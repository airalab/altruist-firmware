#ifndef ALTRUIST_LIBRARY_DIAGNOSTICS_H
#define ALTRUIST_LIBRARY_DIAGNOSTICS_H

#include <Arduino.h>

#if !defined(ALTRUIST_BUILD_DEBUG)
// ESPRobonomicsClient only uses Serial for logs. Do not remap Serial in the
// application: Improv uses the same USB port for functional provisioning frames.
class AltruistSilentLibrarySerial : public Print {
public:
    using Print::write;
    size_t write(uint8_t) override { return 1; }
    size_t write(const uint8_t *, size_t size) override { return size; }
};
static AltruistSilentLibrarySerial altruistSilentLibrarySerial;
#undef Serial
#define Serial altruistSilentLibrarySerial
#endif

#endif
