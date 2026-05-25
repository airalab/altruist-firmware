#ifndef __SDS011_H__
#define __SDS011_H__

#include "sensor.h"

constexpr const unsigned long WARMUPTIME_SDS_MS = 15000;  // time needed to "warm up" the sensor before we can take the first measurement
constexpr const unsigned long READINGTIME_SDS_MS = 5000;  // how long we read data from the PM sensors
constexpr const unsigned long SAMPLETIME_SDS_MS = 1000;

enum class PmSensorCmd {
	Start,
	Stop,
	ContinuousMode
};

class SDS011Sensor : public Sensor {

public:
  SDS011Sensor(unsigned long sending_timeout = 1000UL);
  
  bool begin() override;
  bool getIsSDSRunning() { return is_SDS_running; }

private:
    void _fetch(JsonDocument &data) override;
    void sdsUartRecover();
    unsigned long SDS_error_count = 0;
    uint8_t sds_bad_window_streak = 0;
    uint16_t sds_recovery_count = 0;
    uint32_t sds_pm10_sum = 0;
    uint32_t sds_pm25_sum = 0;
    uint32_t sds_val_count = 0;
    uint32_t sds_pm10_max = 0;
    uint32_t sds_pm10_min = 20000;
    uint32_t sds_pm25_max = 0;
    uint32_t sds_pm25_min = 20000;
    String last_p1_str;
    String last_p2_str;
    unsigned long last_measure_time;
    bool is_SDS_running = false;
    String last_value_SDS_version;
    bool checksum_valid(const uint8_t (&data)[8]);
    bool rawcmd(const uint8_t cmd_head1, const uint8_t cmd_head2, const uint8_t cmd_head3);
    bool cmd(PmSensorCmd cmd);
    String version_date();
};

#endif // __SDS011_H__