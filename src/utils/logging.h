#pragma once

#include "convar.h"
#include "tier0/logging.h"

extern CConVar<bool> kz_log_to_file;
extern CConVar<bool> kz_log_new_file_on_startup;

// Common tag applied to every CS2KZ-owned logging channel.
// KZLoggingListener uses this to filter which channels it processes.
#define KZ_LOG_TAG "CS2KZ"

// Separate logging channels for each service, plus helpers.
DECLARE_LOGGING_CHANNEL(LOG_KZ_GENERAL);
DECLARE_LOGGING_CHANNEL(LOG_KZ_AC);
DECLARE_LOGGING_CHANNEL(LOG_KZ_DB);
DECLARE_LOGGING_CHANNEL(LOG_KZ_GLOBAL);
DECLARE_LOGGING_CHANNEL(LOG_KZ_LANGUAGE);
DECLARE_LOGGING_CHANNEL(LOG_KZ_MAPPINGAPI);
DECLARE_LOGGING_CHANNEL(LOG_KZ_MISC);
DECLARE_LOGGING_CHANNEL(LOG_KZ_MODE);
DECLARE_LOGGING_CHANNEL(LOG_KZ_MOVEMENT);
DECLARE_LOGGING_CHANNEL(LOG_KZ_OPTION);
DECLARE_LOGGING_CHANNEL(LOG_KZ_PLAYER);
DECLARE_LOGGING_CHANNEL(LOG_KZ_PROFILE);
DECLARE_LOGGING_CHANNEL(LOG_KZ_RACING);
DECLARE_LOGGING_CHANNEL(LOG_KZ_RECORDING);
DECLARE_LOGGING_CHANNEL(LOG_KZ_REPLAYS);
DECLARE_LOGGING_CHANNEL(LOG_KZ_STYLE);
DECLARE_LOGGING_CHANNEL(LOG_KZ_TIMER);
DECLARE_LOGGING_CHANNEL(LOG_KZ_TIP);
DECLARE_LOGGING_CHANNEL(LOG_KZ_TRIGGER);

// Enum for service selection - maps to individual channels
enum class LogChannel
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

// Get the logging channel for a given service.
LoggingChannelID_t GetServiceChannel(LogChannel service);

// Logging macros dispatch to the appropriate service channel.
// The KZLoggingListener extracts the service name from the channel name (e.g. "CS2KZ.Timer")
// and includes it as a "[Service]" prefix in the output.
//
//     KZ_LOG_INFO (LogChannel::Timer, "started timer for %s", name);
//     KZ_LOG_DEBUG(LogChannel::AC,    "subtick check failed: %d", flags);
//     KZ_LOG_WARN (LogChannel::DB,    "query took %.2fs", seconds);
//     KZ_LOG_ERROR(LogChannel::Misc,  "unrecoverable: %s", err);
//
// Variadic args are not evaluated when the channel is below the configured
// verbosity, so debug messages are essentially free in release configurations.
#define KZ_LOG_INFO(service, fmt, ...)  Log_Msg(GetServiceChannel(service), fmt, ##__VA_ARGS__)
#define KZ_LOG_DEBUG(service, fmt, ...) InternalMsg(GetServiceChannel(service), LS_DETAILED, fmt, ##__VA_ARGS__)
#define KZ_LOG_WARN(service, fmt, ...)  Log_Warning(GetServiceChannel(service), fmt, ##__VA_ARGS__)
#define KZ_LOG_ERROR(service, fmt, ...) Log_Error(GetServiceChannel(service), fmt, ##__VA_ARGS__)

#include "filesystem.h"

// Logging listener that claims every channel tagged with KZ_LOG_TAG, prints
// it to the console with a [CS2KZ.<Service>] [<LEVEL>] prefix and optionally
// mirrors output to a log file.
class KZLoggingListener : public ILoggingListener
{
public:
	void Log(const LoggingContext_t *pContext, const tchar *pMessage) override;
	void OpenFile(bool useDatetimeFilename);
	void CloseFile();

private:
	FileHandle_t m_pFile = nullptr;
};
