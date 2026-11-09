#ifndef ALTRUIST_DIAGNOSTICS_H
#define ALTRUIST_DIAGNOSTICS_H

// Apply before framework/library headers, including when custom build flags
// request maximum verbosity in a release build.
#if !defined(ALTRUIST_BUILD_DEBUG)
#undef CORE_DEBUG_LEVEL
#define CORE_DEBUG_LEVEL 0
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL 0
#endif

// Keep diagnostic expressions (including argument evaluation) out of release.
#if defined(ALTRUIST_BUILD_DEBUG)
#define ALTRUIST_DIAGNOSTIC(...) do { __VA_ARGS__; } while (0)
#else
#define ALTRUIST_DIAGNOSTIC(...) do {} while (0)
#endif

#endif
