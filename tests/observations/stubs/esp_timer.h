#pragma once
#include <stdint.h>
extern uint64_t fakeNow;
inline int64_t esp_timer_get_time() { return fakeNow * 1000; }
