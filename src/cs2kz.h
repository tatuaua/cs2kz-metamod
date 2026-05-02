#pragma once

#include <string_view>

#include "common.h"
#include "version_gen.h"

class KZPlugin;
extern KZPlugin g_KZPlugin;

class KZLoggingListener : public ILoggingListener
{
public:
	enum LogLevel : int
	{
		LOG_LEVEL_NONE = 0,
		LOG_LEVEL_WARNING,
		LOG_LEVEL_INFO,
		LOG_LEVEL_DEBUG,
	};

	void Log(const LoggingContext_t *pContext, const tchar *pMessage) override;
	void OpenFile();
	void CloseFile();

	static LogLevel ParseLogLevel(const char *value, LogLevel defaultLevel = LOG_LEVEL_INFO);

	LogLevel m_logLevel = LOG_LEVEL_INFO;

private:
	FILE *m_pFile = nullptr;
};

class KZPlugin : public ISmmPlugin, public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	bool Unload(char *error, size_t maxlen);
	bool Pause(char *error, size_t maxlen);
	bool Unpause(char *error, size_t maxlen);
	void AddonInit();
	bool IsAddonMounted();
	void AllPluginsLoaded();

public:
	const char *GetAuthor()
	{
		return PLUGIN_AUTHOR;
	}

	const char *GetName()
	{
		return PLUGIN_DISPLAY_NAME;
	}

	const char *GetDescription()
	{
		return PLUGIN_DESCRIPTION;
	}

	const char *GetURL()
	{
		return PLUGIN_URL;
	}

	const char *GetLicense()
	{
		return PLUGIN_LICENSE;
	}

	const char *GetVersion()
	{
		return PLUGIN_FULL_VERSION;
	}

	const char *GetDate()
	{
		return __DATE__;
	}

	const char *GetLogTag()
	{
		return PLUGIN_LOGTAG;
	}

	virtual void *OnMetamodQuery(const char *iface, int *ret) override;

	bool simulatingPhysics = false;
	CGlobalVars serverGlobals;
	bool unloading = false;
	KZLoggingListener loggingListener;

private:
	void UpdateSelfMD5();
	char md5[33];

public:
	std::string_view GetMD5()
	{
		return md5;
	}
};
