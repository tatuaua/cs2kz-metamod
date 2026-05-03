#include <ctime>

#include "common.h"
#include "convar.h"
#include "tier0/logging.h"
#include "filesystem.h"
#include "utils/logging.h"

CConVar<bool> kz_log_to_file("kz_log_to_file", FCVAR_NONE, "Whether to mirror CS2KZ log output to a file in addons/cs2kz/logs.", true);
CConVar<bool> kz_log_new_file_on_startup("kz_log_new_file_on_startup", FCVAR_NONE,
										 "Whether to create a new log file on plugin startup, named with the current datetime.", true);

// Helper to define a CS2KZ logging channel that is automatically tagged with
// KZ_LOG_TAG. KZLoggingListener uses the tag to claim the message, so every
// channel intended for CS2KZ output must go through this macro.
#define DEFINE_KZ_LOGGING_CHANNEL(channel, name, verbosity)                  \
	BEGIN_DEFINE_LOGGING_CHANNEL(channel, name, LCF_DO_NOT_ECHO, verbosity); \
	ADD_LOGGING_CHANNEL_TAG(KZ_LOG_TAG);                                     \
	END_DEFINE_LOGGING_CHANNEL()

DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_GENERAL,    "CS2KZ.General",    LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_AC,         "CS2KZ.AC",         LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_DB,         "CS2KZ.DB",         LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_GLOBAL,     "CS2KZ.Global",     LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_LANGUAGE,   "CS2KZ.Language",   LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_MAPPINGAPI, "CS2KZ.MappingAPI", LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_MISC,       "CS2KZ.Misc",       LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_MODE,       "CS2KZ.Mode",       LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_MOVEMENT,   "CS2KZ.Movement",   LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_OPTION,     "CS2KZ.Option",     LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_PLAYER,     "CS2KZ.Player",     LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_PROFILE,    "CS2KZ.Profile",    LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_RACING,     "CS2KZ.Racing",     LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_RECORDING,  "CS2KZ.Recording",  LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_REPLAYS,    "CS2KZ.Replays",    LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_STYLE,      "CS2KZ.Style",      LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_TIMER,      "CS2KZ.Timer",      LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_TIP,        "CS2KZ.Tip",        LV_DEFAULT);
DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ_TRIGGER,    "CS2KZ.Trigger",    LV_DEFAULT);

// Map LogService enum to logging channels
LoggingChannelID_t GetServiceChannel(LogService service)
{
	switch (service)
	{
		case LogService::General:    return LOG_KZ_GENERAL;
		case LogService::AC:         return LOG_KZ_AC;
		case LogService::DB:         return LOG_KZ_DB;
		case LogService::Global:     return LOG_KZ_GLOBAL;
		case LogService::Language:   return LOG_KZ_LANGUAGE;
		case LogService::MappingAPI: return LOG_KZ_MAPPINGAPI;
		case LogService::Misc:       return LOG_KZ_MISC;
		case LogService::Mode:       return LOG_KZ_MODE;
		case LogService::Movement:   return LOG_KZ_MOVEMENT;
		case LogService::Option:     return LOG_KZ_OPTION;
		case LogService::Player:     return LOG_KZ_PLAYER;
		case LogService::Profile:    return LOG_KZ_PROFILE;
		case LogService::Racing:     return LOG_KZ_RACING;
		case LogService::Recording:  return LOG_KZ_RECORDING;
		case LogService::Replays:    return LOG_KZ_REPLAYS;
		case LogService::Style:      return LOG_KZ_STYLE;
		case LogService::Timer:      return LOG_KZ_TIMER;
		case LogService::Tip:        return LOG_KZ_TIP;
		case LogService::Trigger:    return LOG_KZ_TRIGGER;
	}
	return LOG_KZ_GENERAL;
}

void KZLoggingListener::Log(const LoggingContext_t *pContext, const tchar *pMessage)
{
	if (!LoggingSystem_HasTag(pContext->m_ChannelID, KZ_LOG_TAG))
	{
		return;
	}

	const char *level = "INFO";
	Color color(255, 255, 255, 255);

	if (pContext->m_Severity >= LS_WARNING)
	{
		level = "WARN";
		color = Color(255, 220, 80, 255);
	}
	else if (pContext->m_Severity == LS_DETAILED)
	{
		level = "DEBUG";
		color = Color(160, 160, 160, 255);
	}

	const CLoggingSystem::LoggingChannel_t *pChannel = LoggingSystem_GetChannel(pContext->m_ChannelID);
	const char *channelName = pChannel ? pChannel->m_Name : "CS2KZ.General";

	size_t msgLen = V_strlen(pMessage);
	bool needsNewline = (msgLen == 0 || pMessage[msgLen - 1] != '\n');

	ConColorMsg(color, "[%s] [%s] %s%s", channelName, level, pMessage, needsNewline ? "\n" : "");

	if (m_pFile)
	{
		std::time_t t = std::time(nullptr);
		std::tm tm {};
#ifdef _WIN32
		localtime_s(&tm, &t);
#else
		localtime_r(&t, &tm);
#endif
		char ts[32];
		std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm);
		g_pFullFileSystem->FPrintf(m_pFile, "[%s] [%s] [%s] %s%s", ts, channelName, level, pMessage, needsNewline ? "\n" : "");
		g_pFullFileSystem->Flush(m_pFile);
	}
}

void KZLoggingListener::OpenFile(bool useDatetimeFilename)
{
	if (m_pFile)
	{
		return;
	}
	char dir[1024];
	V_snprintf(dir, sizeof(dir), "%s/addons/cs2kz/logs", g_SMAPI->GetBaseDir());
	V_FixSlashes(dir);
	g_pFullFileSystem->CreateDirHierarchy(dir, nullptr);

	char path[1024];
	if (useDatetimeFilename)
	{
		std::time_t t = std::time(nullptr);
		std::tm tm {};
#ifdef _WIN32
		localtime_s(&tm, &t);
#else
		localtime_r(&t, &tm);
#endif
		char ts[32];
		std::strftime(ts, sizeof(ts), "%Y-%m-%d", &tm);
		V_snprintf(path, sizeof(path), "%s/cs2kz_%s.log", dir, ts);
	}
	else
	{
		V_snprintf(path, sizeof(path), "%s/cs2kz.log", dir);
	}
	V_FixSlashes(path);
	m_pFile = g_pFullFileSystem->Open(path, "a");
}

void KZLoggingListener::CloseFile()
{
	if (m_pFile)
	{
		g_pFullFileSystem->Close(m_pFile);
		m_pFile = nullptr;
	}
}

// TODO: temporary - remove once logging configuration is finalized.
CON_COMMAND_F(kz_log_verbosity, "Set verbosity for all CS2KZ-tagged logging channels. Usage: kz_log_verbosity <off|essential|default|detailed|max>",
			  FCVAR_NONE)
{
	if (args.ArgC() != 2)
	{
		Msg("Usage: kz_log_verbosity <off|essential|default|detailed|max>\n");
		return;
	}

	const char *level = args.Arg(1);
	LoggingVerbosity_t verbosity;
	if (V_stricmp(level, "off") == 0)
	{
		verbosity = LV_OFF;
	}
	else if (V_stricmp(level, "essential") == 0)
	{
		verbosity = LV_ESSENTIAL;
	}
	else if (V_stricmp(level, "default") == 0)
	{
		verbosity = LV_DEFAULT;
	}
	else if (V_stricmp(level, "detailed") == 0)
	{
		verbosity = LV_DETAILED;
	}
	else if (V_stricmp(level, "max") == 0)
	{
		verbosity = LV_MAX;
	}
	else
	{
		Msg("[kz_log_verbosity] Unknown level '%s'. Use one of: off, essential, default, detailed, max.\n", level);
		return;
	}

	LoggingSystem_SetChannelVerbosityByTag(KZ_LOG_TAG, verbosity);
	Msg("[kz_log_verbosity] Set all '%s'-tagged channels to %s.\n", KZ_LOG_TAG, level);
}

// TODO: temporary - remove once logging configuration is finalized.
CON_COMMAND_F(kz_log_test, "Emit a test log message. Usage: kz_log_test <service> <debug|info|warn|error>", FCVAR_NONE)
{
	if (args.ArgC() != 3)
	{
		Msg("Usage: kz_log_test <service> <debug|info|warn|error>\n");
		Msg("  <service> is a service name (e.g. General, Timer, AC, DB).\n");
		return;
	}

	const char *serviceArg = args.Arg(1);
	const char *level = args.Arg(2);

	LogService service = LogService::General;
	if (V_stricmp(serviceArg, "General") == 0)
		service = LogService::General;
	else if (V_stricmp(serviceArg, "AC") == 0)
		service = LogService::AC;
	else if (V_stricmp(serviceArg, "DB") == 0)
		service = LogService::DB;
	else if (V_stricmp(serviceArg, "Global") == 0)
		service = LogService::Global;
	else if (V_stricmp(serviceArg, "Language") == 0)
		service = LogService::Language;
	else if (V_stricmp(serviceArg, "MappingAPI") == 0)
		service = LogService::MappingAPI;
	else if (V_stricmp(serviceArg, "Misc") == 0)
		service = LogService::Misc;
	else if (V_stricmp(serviceArg, "Mode") == 0)
		service = LogService::Mode;
	else if (V_stricmp(serviceArg, "Movement") == 0)
		service = LogService::Movement;
	else if (V_stricmp(serviceArg, "Option") == 0)
		service = LogService::Option;
	else if (V_stricmp(serviceArg, "Player") == 0)
		service = LogService::Player;
	else if (V_stricmp(serviceArg, "Profile") == 0)
		service = LogService::Profile;
	else if (V_stricmp(serviceArg, "Racing") == 0)
		service = LogService::Racing;
	else if (V_stricmp(serviceArg, "Recording") == 0)
		service = LogService::Recording;
	else if (V_stricmp(serviceArg, "Replays") == 0)
		service = LogService::Replays;
	else if (V_stricmp(serviceArg, "Style") == 0)
		service = LogService::Style;
	else if (V_stricmp(serviceArg, "Timer") == 0)
		service = LogService::Timer;
	else if (V_stricmp(serviceArg, "Tip") == 0)
		service = LogService::Tip;
	else if (V_stricmp(serviceArg, "Trigger") == 0)
		service = LogService::Trigger;
	else
	{
		Msg("[kz_log_test] Unknown service '%s'. Valid services: General, AC, DB, Global, Language, MappingAPI, Misc, Mode, Movement, Option, Player, Profile, Racing, Recording, Replays, Style, Timer, Tip, Trigger.\n", serviceArg);
		return;
	}

	if (V_stricmp(level, "debug") == 0)
	{
		KZ_LOG_DEBUG(service, "kz_log_test: debug message\n");
	}
	else if (V_stricmp(level, "info") == 0)
	{
		KZ_LOG_INFO(service, "kz_log_test: info message\n");
	}
	else if (V_stricmp(level, "warn") == 0 || V_stricmp(level, "warning") == 0)
	{
		KZ_LOG_WARN(service, "kz_log_test: warning message\n");
	}
	else if (V_stricmp(level, "error") == 0)
	{
		KZ_LOG_ERROR(service, "kz_log_test: error message\n");
	}
	else
	{
		Msg("[kz_log_test] Unknown level '%s'. Use one of: debug, info, warn, error.\n", level);
	}


}
