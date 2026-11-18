#pragma once
#include <stdint.h>
#include <stddef.h>
using esp_err_t = int;
using i2s_port_t = int;
using i2s_mode_t = int;
using i2s_comm_format_t = int;
constexpr int ESP_OK = 0, I2S_NUM_0 = 0, I2S_MODE_MASTER = 1, I2S_MODE_RX = 2;
constexpr int I2S_BITS_PER_SAMPLE_32BIT = 32, I2S_CHANNEL_FMT_ONLY_LEFT = 0;
constexpr int I2S_COMM_FORMAT_I2S = 1, I2S_COMM_FORMAT_I2S_MSB = 2, ESP_INTR_FLAG_LEVEL1 = 1;
constexpr unsigned portMAX_DELAY = 0xffffffff;
struct i2s_config_t {
  int mode, sample_rate, bits_per_sample, channel_format, communication_format;
  int intr_alloc_flags, dma_buf_count, dma_buf_len;
  bool use_apll;
};
struct i2s_pin_config_t { int bck_io_num, ws_io_num, data_out_num, data_in_num; };
extern int installError, pinError, startError, readError, samplePattern;
extern size_t readBytes;
inline int i2s_driver_install(int, const i2s_config_t *, int, void *) { return installError; }
inline int i2s_set_pin(int, const i2s_pin_config_t *) { return pinError; }
inline int i2s_start(int) { return startError; }
inline int i2s_stop(int) { return 0; }
inline int i2s_driver_uninstall(int) { return 0; }
inline int i2s_read(int, void *buffer, size_t size, size_t *read, unsigned) {
  auto *samples = static_cast<int32_t *>(buffer);
  for (size_t i = 0; i < size / sizeof(int32_t); ++i)
    samples[i] = samplePattern == 0 ? 0 : samplePattern == 1 ? 256 :
      samplePattern == 2 ? (i % 2 ? 512 : -512) : (i % 2 ? 2147483392 : INT32_MIN);
  *read = readBytes;
  return readError;
}
