#include "kz_global.h"
#include "vendor/nlohmann/json.hpp"

using JSON = nlohmann::json;

extern ISteamHTTP *g_http;

#define KZ_API_BASE_URL "https://staging.cs2.kz/"

internal const char *refreshKey;
internal const char *JWT;
internal bool authenticated;
internal bool APIStatus;

f64 KZGlobalService::Heartbeat()
{
	g_HTTPManager.GET(KZ_API_BASE_URL, &HeartbeatCallback);
	return 5.0f;
}

void KZGlobalService::HeartbeatCallback(HTTPRequestHandle request, int statusCode, std::string response, bool requestSuccessful)
{
	if (statusCode >= 200 && statusCode <= 299)
	{
		APIStatus = true;
	}
	else
	{
		APIStatus = false;
	}
}

f64 KZGlobalService::RefreshJWT()
{
	char url[256];
	V_snprintf(url, sizeof(url), "%s%s", KZ_API_BASE_URL, "servers/key");

	JSON requestBody;
	requestBody["refresh_key"] = refreshKey;
	requestBody["plugin_version"] = "1.2.3"; //g_KZPlugin.GetVersion()

	f64 refreshInterval = 900.0f;
	if (!JWT)
	{
		// Do the first request a minute ahead
		refreshInterval = 840.0f;
	}

	g_HTTPManager.POST(url, requestBody.dump().c_str(), &RefreshJWTCallback);
	return refreshInterval;
}

void KZGlobalService::RefreshJWTCallback(HTTPRequestHandle request, int statusCode, std::string response, bool requestSuccessful)
{
	if (statusCode >= 200 && statusCode <= 299)
	{
		JWT = response.c_str();
		authenticated = true;
	}
	else
	{
		authenticated = false;
		if (statusCode == 401)
		{
			// Do something here
			META_CONPRINTF("\nRefresh key is invalid\n");
		}
		else
		{
			META_CONPRINTF("\nSomething unexpexted: %s\n", response.c_str());
		}
	}
}

void KZGlobalService::Init()
{
	StartTimer(Heartbeat, true, false);
	refreshKey = KZOptionService::GetOptionStr("refreshKey");
	authenticated = false;
	StartTimer(RefreshJWT, true, false);
}
