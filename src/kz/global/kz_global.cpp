#include "kz_global.h"
#include "vendor/nlohmann/json.hpp"

using JSON = nlohmann::json;

extern ISteamHTTP *g_http;

#define KZ_API_BASE_URL "https://staging.cs2.kz/"

internal const char *refreshKey;
internal const char *JWT = "";
internal bool authenticated = false;
internal bool APIStatus = false;

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

	f64 refreshInterval = 10.0f;
	if (V_stricmp(JWT, "") == 0)
	{
		// Do the first request earlier
		refreshInterval = 1.0f;
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

void KZGlobalService::SendPlayerInfo(KZPlayer* player, const char *someInfo) 
{
	std::vector<HTTPHeader> headers;

	headers.push_back(GetJWTHeader());

	g_HTTPManager.POST("https://staging.cs2.kz/players/STEAM_1%3A1%3A161178172/preferences", someInfo, &SendPlayerInfoCallback, &headers, player);
}

void KZGlobalService::SendPlayerInfoCallback(HTTPRequestHandle request, int statusCode, std::string response, bool requestSuccessful,
											 KZPlayer *player)
{
	META_CONPRINTF("\nplayer callback: %s", player->GetController()->m_iszPlayerName());
}

void KZGlobalService::Init()
{
	StartTimer(Heartbeat, true, false);
	refreshKey = KZOptionService::GetOptionStr("refreshKey");
	authenticated = false;
	StartTimer(RefreshJWT, true, false);
}

HTTPHeader KZGlobalService::GetJWTHeader()
{
	char value[2048];
	V_snprintf(value, sizeof(value), "%s%s", "Bearer ", JWT);
	HTTPHeader header("Authorization", value);
	return header;
}
