#pragma once

#include "tier0/logging.h"

// Common tag applied to every CS2KZ-owned logging channel.
// KZLoggingListener uses this to filter which channels it processes.
#define KZ_LOG_TAG "CS2KZ"

// Single CS2KZ logging channel. Service distinction is encoded directly into
// the message via a "[Service] " prefix appended by the KZ_LOG_* macros.
DECLARE_LOGGING_CHANNEL(LOG_KZ);

// Logical subsystems that can tag log messages. Add new entries here when
// introducing a new service so every call site is discoverable and
// refactorable.
enum class LogService
{
	General,
	AC,
	DB,
	Global,
	Language,
	MappingAPI,
	Misc,
	Mode,
	Movement,
	Option,
	Player,
	Profile,
	Racing,
	Recording,
	Replays,
	Style,
	Timer,
	Tip,
	Trigger,
};

const char *LogServiceName(LogService service);

// Logging macros. The first argument is a LogService enumerator; its name is
// prepended to the message as "[Service] " so a single channel can carry
// per-subsystem context.
//
//     KZ_LOG_INFO (LogService::Timer, "started timer for %s",  name);
//     KZ_LOG_DEBUG(LogService::AC,    "subtick check failed: %d", flags);
//     KZ_LOG_WARN (LogService::DB,    "query took %.2fs", seconds);
//     KZ_LOG_ERROR(LogService::Misc,  "unrecoverable: %s", err);
//
// Variadic args are not evaluated when the channel is below the configured
// verbosity, so debug messages are essentially free in release configurations.
#define KZ_LOG_INFO(service, fmt, ...)  Log_Msg(LOG_KZ, "[%s] " fmt, LogServiceName(service), ##__VA_ARGS__)
#define KZ_LOG_DEBUG(service, fmt, ...) InternalMsg(LOG_KZ, LS_DETAILED, "[%s] " fmt, LogServiceName(service), ##__VA_ARGS__)
#define KZ_LOG_WARN(service, fmt, ...)  Log_Warning(LOG_KZ, "[%s] " fmt, LogServiceName(service), ##__VA_ARGS__)
#define KZ_LOG_ERROR(service, fmt, ...) Log_Error(LOG_KZ, "[%s] " fmt, LogServiceName(service), ##__VA_ARGS__)

#include "filesystem.h"

// Logging listener that claims every channel tagged with KZ_LOG_TAG, prints
// it to the console with a [CS2KZ.<Service>] [<LEVEL>] prefix and optionally
// mirrors output to a log file.
class KZLoggingListener : public ILoggingListener
{
public:
	void Log(const LoggingContext_t *pContext, const tchar *pMessage) override;
	void OpenFile();
	void CloseFile();

private:
	FileHandle_t m_pFile = nullptr;
};
