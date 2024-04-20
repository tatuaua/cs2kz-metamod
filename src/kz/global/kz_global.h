#pragma once
#include "../kz.h"
#include "utils/httpmanager.h"
#include "steam/steam_api_common.h"
#include "steam/isteamhttp.h"
#include "utils/ctimer.h"
#include "../option/kz_option.h"

class KZGlobalService : public KZBaseService
{
	using KZBaseService::KZBaseService;

public:
	static_global void Init();
	static_global void SendPlayerInfo(KZPlayer *player, const char *someInfo);

private:
	static f64 Heartbeat();
	static void HeartbeatCallback(HTTPRequestHandle request, int statusCode, std::string response, bool requestSuccessful);
	static f64 RefreshJWT();
	static void RefreshJWTCallback(HTTPRequestHandle request, int statusCode, std::string response, bool requestSuccessful);
	static HTTPHeader GetJWTHeader();
	static void SendPlayerInfoCallback(HTTPRequestHandle request, int statusCode, std::string response, bool requestSuccessful,
											  KZPlayer *player);
};
