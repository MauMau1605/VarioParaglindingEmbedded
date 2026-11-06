/// @file adc_hal.cpp
/// @brief SAADC implementation for battery voltage divider readout.

#include "hal/adc_hal.hpp"
#include "common/config.hpp"
#include <hal/nrf_saadc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>

namespace vario::hal {

static const struct device *const s_adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));
static constexpr uint8_t kAdcChannelId = 3;

static int16_t s_sample_val = 0;

static struct adc_sequence create_adc_sequence() {
  struct adc_sequence seq = {};
  seq.channels = BIT(kAdcChannelId);
  seq.buffer = &s_sample_val;
  seq.buffer_size = sizeof(s_sample_val);
  seq.resolution = 12;
  return seq;
}

Error adc_init() {
  if (!device_is_ready(s_adc_dev)) {
    return Error::kNotInitialized;
  }
  struct adc_channel_cfg channel_cfg = {};
  channel_cfg.gain = ADC_GAIN_1_4;
  channel_cfg.reference = ADC_REF_INTERNAL;
  channel_cfg.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40);
  channel_cfg.channel_id = kAdcChannelId;
  channel_cfg.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput3;
  const int rc = adc_channel_setup(s_adc_dev, &channel_cfg);
  return (rc == 0) ? Error::kOk : Error::kAdcError;
}

static uint16_t raw_to_battery_mv(int32_t raw) {
  if (raw <= 0) {
    return 0;
  }
  // Full scale = 0.6 V / (1/4) = 2400 mV, 12-bit = 4096. Divider = 2x.
  // v_pin_mv = raw * 2400 / 4096, v_bat_mv = v_pin_mv * 2
  uint32_t v_bat_mv = (static_cast<uint32_t>(raw) * 4800UL) / 4096UL;
  v_bat_mv = (v_bat_mv * static_cast<uint32_t>(config::kBatteryCalNum)) /
             static_cast<uint32_t>(config::kBatteryCalDen);
  return static_cast<uint16_t>(v_bat_mv);
}

Error adc_read_battery_mv(uint16_t *battery_mv) {
  if (!device_is_ready(s_adc_dev)) {
    return Error::kNotInitialized;
  }
  if (battery_mv == nullptr) {
    return Error::kInvalidData;
  }

  const struct adc_sequence sequence = create_adc_sequence();
  const int rc = adc_read(s_adc_dev, &sequence);
  if (rc != 0) {
    return Error::kAdcError;
  }
  *battery_mv = raw_to_battery_mv(s_sample_val);
  return Error::kOk;
}

} // namespace vario::hal
