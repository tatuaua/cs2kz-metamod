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

DEFINE_KZ_LOGGING_CHANNEL(LOG_KZ, "CS2KZ", LV_DETAILED);

namespace
{
	struct LogServiceEntry
	{
		LogService service;
		const char *name;
	};

	constexpr LogServiceEntry kLogServiceTable[] = {
		{LogService::General,    "General"   },
		{LogService::AC,         "AC"        },
		{LogService::DB,         "DB"        },
		{LogService::Global,     "Global"    },
		{LogService::Language,   "Language"  },
		{LogService::MappingAPI, "MappingAPI"},
		{LogService::Misc,       "Misc"      },
		{LogService::Mode,       "Mode"      },
		{LogService::Movement,   "Movement"  },
		{LogService::Option,     "Option"    },
		{LogService::Player,     "Player"    },
		{LogService::Profile,    "Profile"   },
		{LogService::Racing,     "Racing"    },
		{LogService::Recording,  "Recording" },
		{LogService::Replays,    "Replays"   },
		{LogService::Style,      "Style"     },
		{LogService::Timer,      "Timer"     },
		{LogService::Tip,        "Tip"       },
		{LogService::Trigger,    "Trigger"   },
	};
} // namespace

const char *LogServiceName(LogService service)
{
	for (const auto &entry : kLogServiceTable)
	{
		if (entry.service == service)
		{
			return entry.name;
		}
	}
	return "Unknown";
}

static bool ParseLogService(const char *name, LogService &out)
{
	for (const auto &entry : kLogServiceTable)
	{
		if (V_stricmp(name, entry.name) == 0)
		{
			out = entry.service;
			return true;
		}
	}
	return false;
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
	const char *channelName = pChannel ? pChannel->m_Name : "CS2KZ";

	ConColorMsg(color, "[%s] [%s] %s", channelName, level, pMessage);
	size_t msgLen = V_strlen(pMessage);
	bool needsNewline = (msgLen == 0 || pMessage[msgLen - 1] != '\n');
	if (needsNewline)
	{
		ConColorMsg(color, "\n");
	}

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
		Msg("  <service> is a LogService name (e.g. Timer, AC, DB, General).\n");
		return;
	}

	const char *serviceArg = args.Arg(1);
	const char *level = args.Arg(2);

	LogService service;
	if (!ParseLogService(serviceArg, service))
	{
		Msg("[kz_log_test] Unknown service '%s'. See LogService in utils/logging.h for valid names.\n", serviceArg);
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
