#include "ws2812.h"

#include <string.h>
#include <stdlib.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_heap_caps.h"

#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"

static const char *TAG = "ws2812";

// RMT resolution: 10 MHz (0.1us per tick)
#define RMT_RESOLUTION_HZ 10000000

// WS2812 timing (ticks)
#define T0H 4   // 0.4us
#define T0L 9   // 0.9us

#define T1H 8   // 0.8us
#define T1L 5   // 0.5us

#define RESET_US 80
#define RESET_TICKS (RMT_RESOLUTION_HZ / 1000000 * RESET_US)

static rmt_channel_handle_t s_rmt_chan = NULL;
static rmt_encoder_handle_t s_ws_encoder = NULL;

static uint8_t *s_led_buf = NULL;
static uint32_t s_led_count = 0;
static gpio_num_t s_gpio = -1;

// ---------------- ENCODER STRUCT ------------------

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_handle_t bytes_encoder;
    rmt_encoder_handle_t copy_encoder;
    uint8_t state;
} ws2812_encoder_t;

enum {
    WS_STATE_SEND_DATA = 0,
    WS_STATE_SEND_RESET = 1
};

// -------------- ENCODER API IMPLEMENTATION -------------

static size_t ws2812_encode(
    rmt_encoder_t *encoder,
    rmt_channel_handle_t channel,
    const void *primary_data,
    size_t data_size,
    rmt_encode_state_t *ret_state)
{
    ws2812_encoder_t *enc = __containerof(encoder, ws2812_encoder_t, base);
    size_t encoded = 0;
    rmt_encode_state_t state = 0;

    if (enc->state == WS_STATE_SEND_DATA)
    {
        encoded += enc->bytes_encoder->encode(
            enc->bytes_encoder,
            channel,
            primary_data,
            data_size,
            &state
        );

        if (state & RMT_ENCODING_COMPLETE)
        {
            enc->state = WS_STATE_SEND_RESET;
        }
        if (state & RMT_ENCODING_MEM_FULL)
        {
            *ret_state = state;
            return encoded;
        }
    }

    if (enc->state == WS_STATE_SEND_RESET)
    {
        static const rmt_symbol_word_t reset_symbol = {
            .level0 = 0,
            .duration0 = RESET_TICKS,
            .level1 = 0,
            .duration1 = 0,
        };

        state = 0;
        encoded += enc->copy_encoder->encode(
            enc->copy_encoder,
            channel,
            &reset_symbol,
            sizeof(reset_symbol),
            &state
        );

        if (state & RMT_ENCODING_COMPLETE) {
            enc->state = WS_STATE_SEND_DATA;
            *ret_state = RMT_ENCODING_COMPLETE;
        } else if (state & RMT_ENCODING_MEM_FULL) {
            *ret_state = RMT_ENCODING_MEM_FULL;
        }

        return encoded;
    }

    *ret_state = RMT_ENCODING_COMPLETE;
    return encoded;
}

static esp_err_t ws2812_reset(rmt_encoder_t *encoder)
{
    ws2812_encoder_t *enc = __containerof(encoder, ws2812_encoder_t, base);
    enc->state = WS_STATE_SEND_DATA;

    enc->bytes_encoder->reset(enc->bytes_encoder);
    enc->copy_encoder->reset(enc->copy_encoder);

    return ESP_OK;
}

static esp_err_t ws2812_del(rmt_encoder_t *encoder)
{
    ws2812_encoder_t *enc = __containerof(encoder, ws2812_encoder_t, base);

    if (enc->bytes_encoder)
        enc->bytes_encoder->del(enc->bytes_encoder);

    if (enc->copy_encoder)
        enc->copy_encoder->del(enc->copy_encoder);

    free(enc);

    return ESP_OK;
}

static esp_err_t ws2812_new_encoder(rmt_encoder_handle_t *ret_encoder)
{
    ws2812_encoder_t *enc = calloc(1, sizeof(ws2812_encoder_t));
    if (!enc)
        return ESP_ERR_NO_MEM;

    // base API
    enc->base.encode = ws2812_encode;
    enc->base.reset  = ws2812_reset;
    enc->base.del    = ws2812_del;
    enc->state       = WS_STATE_SEND_DATA;

    // bytes encoder
    rmt_bytes_encoder_config_t bytes_cfg = {
        .bit0 = {.duration0=T0H, .level0=1, .duration1=T0L, .level1=0},
        .bit1 = {.duration0=T1H, .level0=1, .duration1=T1L, .level1=0},
        .flags.msb_first = 1,
    };

    ESP_RETURN_ON_ERROR(
        rmt_new_bytes_encoder(&bytes_cfg, &enc->bytes_encoder),
        TAG, "Failed to create bytes encoder"
    );

    // copy encoder (for reset)
    rmt_copy_encoder_config_t copy_cfg = {};
    ESP_RETURN_ON_ERROR(
        rmt_new_copy_encoder(&copy_cfg, &enc->copy_encoder),
        TAG, "Failed to create copy encoder"
    );

    *ret_encoder = &enc->base;
    return ESP_OK;
}

// --------------------- PUBLIC API ------------------------

esp_err_t ws2812_init(gpio_num_t gpio, uint32_t count)
{
    if (s_rmt_chan != NULL)
        return ESP_OK;

    s_led_count = count;
    s_gpio = gpio;

    size_t buf_size = count * 3;
    s_led_buf = heap_caps_malloc(buf_size, MALLOC_CAP_DEFAULT);
    if (!s_led_buf)
        return ESP_ERR_NO_MEM;
    memset(s_led_buf, 0, buf_size);

    rmt_tx_channel_config_t tx_cfg = {
        .gpio_num = gpio,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
        .flags.with_dma = true,
    };

    ESP_RETURN_ON_ERROR(
        rmt_new_tx_channel(&tx_cfg, &s_rmt_chan),
        TAG, "Cannot create RMT channel"
    );

    ESP_RETURN_ON_ERROR(
        ws2812_new_encoder(&s_ws_encoder),
        TAG, "Cannot create WS2812 encoder"
    );

    ESP_RETURN_ON_ERROR(
        rmt_enable(s_rmt_chan),
        TAG, "Cannot enable RMT"
    );

    ESP_LOGI(TAG, "WS2812 initialized: gpio=%d leds=%lu", gpio, (unsigned long)count);
    return ESP_OK;
}

esp_err_t ws2812_show(void)
{
    if (!s_rmt_chan || !s_ws_encoder)
        return ESP_ERR_INVALID_STATE;

    rmt_transmit_config_t tx_cfg = {
        .loop_count = 0,
    };

    ESP_RETURN_ON_ERROR(
        rmt_transmit(s_rmt_chan, s_ws_encoder, s_led_buf, s_led_count * 3, &tx_cfg),
        TAG, "Transmit error"
    );

    return rmt_tx_wait_all_done(s_rmt_chan, -1);
}

void ws2812_set_pixel(uint32_t i, uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_led_buf || i >= s_led_count) return;

    size_t o = i * 3;
    s_led_buf[o] = g;
    s_led_buf[o+1] = r;
    s_led_buf[o+2] = b;
}

void ws2812_fill(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint32_t i = 0; i < s_led_count; i++)
        ws2812_set_pixel(i, r, g, b);
}

void ws2812_clear(void)
{
    if (s_led_buf)
        memset(s_led_buf, 0, s_led_count * 3);
}

uint32_t ws2812_get_count(void)
{
    return s_led_count;
}
