#ifndef __MSG_FORMATTER_H__
#define __MSG_FORMATTER_H__

#include <ArduinoJson.h>
#include <Robonomics.h>

void formatRobonomicsString(JsonDocument &data, String &datalog_data);
void addTimeAndSign(const String &data, String &signature, Robonomics *robonomics);

#endif // __MSG_FORMATTER_H__