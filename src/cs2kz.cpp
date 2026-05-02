#include "cs2kz.h"

#include <ctime>

#include "entity2/entitysystem.h"
#include "steam/steam_gameserver.h"
#include "filesystem.h"

#include "sdk/cgameresourceserviceserver.h"
#include "utils/utils.h"
#include "utils/hooks.h"
#include "utils/gameconfig.h"

#include "movement/movement.h"
#include "kz/kz.h"
#include "kz/anticheat/kz_anticheat.h"
#include "kz/db/kz_db.h"
#include "kz/hud/kz_hud.h"
#include "kz/mode/kz_mode.h"
#include "kz/spec/kz_spec.h"
#include "kz/goto/kz_goto.h"
#include "kz/style/kz_style.h"
#include "kz/quiet/kz_quiet.h"
#include "kz/tip/kz_tip.h"
#include "kz/option/kz_option.h"
#include "kz/language/kz_language.h"
#include "kz/mappingapi/kz_mappingapi.h"
#include "kz/global/kz_global.h"
#include "kz/beam/kz_beam.h"
#include "kz/pistol/kz_pistol.h"
#include "kz/recording/kz_recording.h"
#include "kz/replays/kz_replaysystem.h"
#include "kz/racing/kz_racing.h"

#include <vendor/MultiAddonManager/public/imultiaddonmanager.h>
#include <vendor/ClientCvarValue/public/iclientcvarvalue.h>
#include <vendor/ixwebsocket/ixwebsocket/IXNetSystem.h>

#include "tier0/memdbgon.h"
KZPlugin g_KZPlugin;

IMultiAddonManager *g_pMultiAddonManager;
IClientCvarValue *g_pClientCvarValue;
CSteamGameServerAPIContext g_steamAPI;

PLUGIN_EXPOSE(KZPlugin, g_KZPlugin);

void KZLoggingListener::Log(const LoggingContext_t *pContext, const tchar *pMessage)
{
	if (pContext->m_ChannelID != LOG_KZ)
	{
		return;
	}

	LogLevel msgLevel;
	if (pContext->m_Severity >= LS_WARNING)
	{
		msgLevel = LOG_LEVEL_WARNING;
	}
	else if (pContext->m_Severity == LS_DETAILED)
	{
		msgLevel = LOG_LEVEL_DEBUG;
	}
	else
	{
		msgLevel = LOG_LEVEL_INFO;
	}

	if (msgLevel > m_logLevel)
	{
		return;
	}

	const char *level = "INFO";
	Color color(255, 255, 255, 255);

	if (msgLevel == LOG_LEVEL_WARNING)
	{
		level = "WARN";
		color = Color(255, 220, 80, 255);
	}
	else if (msgLevel == LOG_LEVEL_DEBUG)
	{
		level = "DEBUG";
		color = Color(160, 160, 160, 255);
	}

	ConColorMsg(color, "[%s] %s", level, pMessage);
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
		g_pFullFileSystem->FPrintf(m_pFile, "[%s] [%s] %s%s", ts, level, pMessage, needsNewline ? "\n" : "");
		g_pFullFileSystem->Flush(m_pFile);
	}
}

void KZLoggingListener::OpenFile()
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
	V_snprintf(path, sizeof(path), "%s/cs2kz.log", dir);
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

KZLoggingListener::LogLevel KZLoggingListener::ParseLogLevel(const char *value, LogLevel defaultLevel)
{
	if (!value || !*value)
	{
		return defaultLevel;
	}
	if (KZ_STREQI(value, "none"))
	{
		return LOG_LEVEL_NONE;
	}
	if (KZ_STREQI(value, "warning"))
	{
		return LOG_LEVEL_WARNING;
	}
	if (KZ_STREQI(value, "info"))
	{
		return LOG_LEVEL_INFO;
	}
	if (KZ_STREQI(value, "debug"))
	{
		return LOG_LEVEL_DEBUG;
	}
	return defaultLevel;
}

bool KZPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	setlocale(LC_ALL, "en_US.utf8");
	PLUGIN_SAVEVARS();

	LoggingSystem_RegisterLoggingListener(&this->loggingListener);

	if (!utils::Initialize(ismm, error, maxlen))
	{
		return false;
	}
	ConVar_Register();
	hooks::Initialize();
	ix::initNetSystem();
	movement::InitDetours();
	KZCheckpointService::Init();
	KZTimerService::Init();
	KZSpecService::Init();
	KZGotoService::Init();
	KZHUDService::Init();
	KZLanguageService::Init();
	KZBeamService::Init();
	KZPistolService::Init();
	KZ::misc::Init();
	KZQuietService::Init();
	KZRecordingService::Init();
	if (!KZ::mode::CheckModeCvars())
	{
		return false;
	}

	ismm->AddListener(this, this);
	KZ::mapapi::Init();
	KZ::mode::InitModeManager();
	KZ::style::InitStyleManager();

	KZ::mode::DisableReplicatedModeCvars();

	KZOptionService::InitOptions();
	if (KZOptionService::GetOptionInt("logToFile", true) != 0)
	{
		this->loggingListener.OpenFile();
	}
	this->loggingListener.m_logLevel =
		KZLoggingListener::ParseLogLevel(KZOptionService::GetOptionStr("logLevel", "info"), KZLoggingListener::LOG_LEVEL_INFO);
	KZTipService::Init();
	KZAnticheatService::Init();
	if (late)
	{
		g_steamAPI.Init();
		g_pKZPlayerManager->OnLateLoad();
		// We need to reset the map for mapping api to properly load in.
		utils::ResetMap();
		KZ::replaysystem::Init();
	}

	// We don't need command filtering for KZ maps.
	CommandLine()->AppendParm("-disable_workshop_command_filtering", "");

	KZ::replaysystem::InitWatcher();
	return true;
}

bool KZPlugin::Unload(char *error, size_t maxlen)
{
	this->unloading = true;
	KZ::misc::UnrestrictTimeLimit();
	KZRecordingService::Shutdown();
	KZRacingService::Cleanup();
	ix::uninitNetSystem();
	hooks::Cleanup();
	KZ::mode::EnableReplicatedModeCvars();
	utils::Cleanup();
	g_pKZModeManager->Cleanup();
	g_pKZStyleManager->Cleanup();
	g_pPlayerManager->Cleanup();
	KZDatabaseService::Cleanup();
	KZGlobalService::Cleanup();
	KZLanguageService::Cleanup();
	KZOptionService::Cleanup();
	KZ::replaysystem::Cleanup();
	KZAnticheatService::CleanupSvCheatsWatcher();
	ConVar_Unregister();
	LoggingSystem_UnregisterLoggingListener(&this->loggingListener);
	this->loggingListener.CloseFile();
	return true;
}

void KZPlugin::AllPluginsLoaded()
{
	KZDatabaseService::Init();
	KZ::mode::LoadModePlugins();
	KZ::style::LoadStylePlugins();
	g_pKZPlayerManager->ResetPlayers();
	this->UpdateSelfMD5();
	g_pMultiAddonManager = (IMultiAddonManager *)g_SMAPI->MetaFactory(MULTIADDONMANAGER_INTERFACE, nullptr, nullptr);
	g_pClientCvarValue = (IClientCvarValue *)g_SMAPI->MetaFactory(CLIENTCVARVALUE_INTERFACE, nullptr, nullptr);
}

void KZPlugin::AddonInit()
{
	static_persist bool addonLoaded;
	if (g_pMultiAddonManager != nullptr && !addonLoaded)
	{
		addonLoaded = g_pMultiAddonManager->AddAddon(KZLanguageService::GetBaseAddon(), true);
		CConVarRef<bool> mm_cache_clients_with_addons("mm_cache_clients_with_addons");
		CConVarRef<float> mm_cache_clients_duration("mm_cache_clients_duration");
		mm_cache_clients_with_addons.Set(true);
		mm_cache_clients_duration.Set(30.0f);
	}
}

bool KZPlugin::IsAddonMounted()
{
	if (g_pMultiAddonManager != nullptr)
	{
		return g_pMultiAddonManager->IsAddonMounted(KZLanguageService::GetBaseAddon(), true);
	}
	return false;
}

bool KZPlugin::Pause(char *error, size_t maxlen)
{
	return true;
}

bool KZPlugin::Unpause(char *error, size_t maxlen)
{
	return true;
}

void *KZPlugin::OnMetamodQuery(const char *iface, int *ret)
{
	if (strcmp(iface, KZ_MODE_MANAGER_INTERFACE) == 0)
	{
		*ret = META_IFACE_OK;
		return g_pKZModeManager;
	}
	else if (strcmp(iface, KZ_STYLE_MANAGER_INTERFACE) == 0)
	{
		*ret = META_IFACE_OK;
		return g_pKZStyleManager;
	}
	else if (strcmp(iface, KZ_UTILS_INTERFACE) == 0)
	{
		*ret = META_IFACE_OK;
		return g_pKZUtils;
	}
	else if (strcmp(iface, KZ_MAPPING_INTERFACE) == 0)
	{
		*ret = META_IFACE_OK;
		return g_pMappingApi;
	}
	*ret = META_IFACE_FAILED;

	return NULL;
}

void KZPlugin::UpdateSelfMD5()
{
	ISmmPluginManager *pluginManager = (ISmmPluginManager *)g_SMAPI->MetaFactory(MMIFACE_PLMANAGER, nullptr, nullptr);
	const char *path;
	pluginManager->Query(g_PLID, &path, nullptr, nullptr);
	g_pKZUtils->GetFileMD5(path, this->md5, sizeof(this->md5));
}

CGameEntitySystem *GameEntitySystem()
{
	return interfaces::pGameResourceServiceServer->GetGameEntitySystem();
}
