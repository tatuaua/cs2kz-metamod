/*
 * Credit to CS2Fixes: https://github.com/Source2ZE/CS2Fixes/blob/40a7f3d9f479aeb8f1d0a5acb61d615c07176e43/src/httpmanager.cpp
 */

#include "httpmanager.h"
#include "../../hl2sdk-cs2/public/steam/steam_api_common.h"
#include "../../hl2sdk-cs2/public/steam/isteamhttp.h"

ISteamHTTP *g_http = nullptr;

CSteamGameServerAPIContext g_steamAPI;

HTTPManager g_HTTPManager;

#undef strdup

HTTPManager::TrackedRequest::TrackedRequest(HTTPRequestHandle hndl, SteamAPICall_t hCall, std::string strUrl, std::string strText,
											CallbackFn callback)
{
	m_hHTTPReq = hndl;
	m_CallResult.SetGameserverFlag();
	m_CallResult.Set(hCall, this, &TrackedRequest::OnHTTPRequestCompleted);

	m_strUrl = strUrl;
	m_strText = strText;
	m_callback = callback;

	g_HTTPManager.m_PendingRequests.push_back(this);
}

HTTPManager::TrackedRequest::~TrackedRequest()
{
	for (auto e = g_HTTPManager.m_PendingRequests.begin(); e != g_HTTPManager.m_PendingRequests.end(); ++e)
	{
		if (*e == this)
		{
			g_HTTPManager.m_PendingRequests.erase(e);
			break;
		}
	}
}

HTTPManager::TrackedPlayerRequest::TrackedPlayerRequest(HTTPRequestHandle hndl, SteamAPICall_t hCall, std::string strUrl, std::string strText,
											PlayerCallbackFn callback, KZPlayer *player)
{
	m_hHTTPReq = hndl;
	m_CallResult.SetGameserverFlag();
	m_CallResult.Set(hCall, this, &TrackedPlayerRequest::OnHTTPRequestCompleted);

	m_strUrl = strUrl;
	m_strText = strText;
	m_callback = callback;

	m_player = player;

	g_HTTPManager.m_PendingPlayerRequests.push_back(this);
}

HTTPManager::TrackedPlayerRequest::~TrackedPlayerRequest()
{
	for (auto e = g_HTTPManager.m_PendingPlayerRequests.begin(); e != g_HTTPManager.m_PendingPlayerRequests.end(); ++e)
	{
		if (*e == this)
		{
			g_HTTPManager.m_PendingPlayerRequests.erase(e);
			break;
		}
	}
}

void HTTPManager::TrackedRequest::OnHTTPRequestCompleted(HTTPRequestCompleted_t *arg, bool bFailed)
{
	uint32 size;
	g_http->GetHTTPResponseBodySize(arg->m_hRequest, &size);

	uint8 *response = new uint8[size + 1];
	g_http->GetHTTPResponseBodyData(arg->m_hRequest, response, size);
	response[size] = 0; // Add null terminator

	m_callback(arg->m_hRequest, arg->m_eStatusCode, (char *)response, arg->m_bRequestSuccessful);

	delete[] response;

	g_http->ReleaseHTTPRequest(arg->m_hRequest);
	
	delete this;
}

void HTTPManager::TrackedPlayerRequest::OnHTTPRequestCompleted(HTTPRequestCompleted_t *arg, bool bFailed)
{
	uint32 size;
	g_http->GetHTTPResponseBodySize(arg->m_hRequest, &size);

	uint8 *response = new uint8[size + 1];
	g_http->GetHTTPResponseBodyData(arg->m_hRequest, response, size);
	response[size] = 0; // Add null terminator

	m_callback(arg->m_hRequest, arg->m_eStatusCode, (char *)response, arg->m_bRequestSuccessful, m_player);

	delete[] response;

	g_http->ReleaseHTTPRequest(arg->m_hRequest);

	delete this;
}

void HTTPManager::GET(const char *pszUrl, CallbackFn callback, std::vector<HTTPHeader> *headers)
{
	GenerateRequest(k_EHTTPMethodGET, pszUrl, "", callback, headers);
}

void HTTPManager::POST(const char *pszUrl, const char *pszText, CallbackFn callback, std::vector<HTTPHeader> *headers)
{
	GenerateRequest(k_EHTTPMethodPOST, pszUrl, pszText, callback, headers);
}

void HTTPManager::PUT(const char *pszUrl, const char *pszText, CallbackFn callback, std::vector<HTTPHeader> *headers)
{
	GenerateRequest(k_EHTTPMethodPUT, pszUrl, pszText, callback, headers);
}

void HTTPManager::PATCH(const char *pszUrl, const char *pszText, CallbackFn callback, std::vector<HTTPHeader> *headers)
{
	GenerateRequest(k_EHTTPMethodPATCH, pszUrl, pszText, callback, headers);
}

void HTTPManager::HTTP_DELETE(const char *pszUrl, CallbackFn callback, std::vector<HTTPHeader> *headers)
{
	GenerateRequest(k_EHTTPMethodDELETE, pszUrl, "", callback, headers);
}

void HTTPManager::GET(const char *pszUrl, PlayerCallbackFn callback, std::vector<HTTPHeader> *headers, KZPlayer *player)
{
	GeneratePlayerRequest(k_EHTTPMethodGET, pszUrl, "", callback, headers, player);
}

void HTTPManager::POST(const char *pszUrl, const char *pszText, PlayerCallbackFn callback, std::vector<HTTPHeader> *headers, KZPlayer *player)
{
	GeneratePlayerRequest(k_EHTTPMethodPOST, pszUrl, pszText, callback, headers, player);
}

void HTTPManager::PUT(const char *pszUrl, const char *pszText, PlayerCallbackFn callback, std::vector<HTTPHeader> *headers, KZPlayer *player)
{
	GeneratePlayerRequest(k_EHTTPMethodPUT, pszUrl, pszText, callback, headers, player);
}

void HTTPManager::PATCH(const char *pszUrl, const char *pszText, PlayerCallbackFn callback, std::vector<HTTPHeader> *headers, KZPlayer *player)
{
	GeneratePlayerRequest(k_EHTTPMethodPATCH, pszUrl, pszText, callback, headers, player);
}

void HTTPManager::HTTP_DELETE(const char *pszUrl, PlayerCallbackFn callback, std::vector<HTTPHeader> *headers, KZPlayer *player)
{
	GeneratePlayerRequest(k_EHTTPMethodDELETE, pszUrl, "", callback, headers, player);
}


void HTTPManager::GenerateRequest(EHTTPMethod method, const char *pszUrl, const char *pszText, CallbackFn callback, std::vector<HTTPHeader> *headers)
{
	if (!g_http)
	{
		g_steamAPI.Init();
		g_http = g_steamAPI.SteamHTTP();
	}

	auto hReq = g_http->CreateHTTPRequest(method, pszUrl);
	int size = strlen(pszText);

	if ((method == k_EHTTPMethodPOST || method == k_EHTTPMethodPUT || method == k_EHTTPMethodPATCH)
		&& !g_http->SetHTTPRequestRawPostBody(hReq, "application/json", (uint8 *)pszText, size))
	{
		META_CONPRINTF("\nFailed to set request body\n");
		return;
	}

	if (headers != nullptr)
	{
		for (HTTPHeader header : *headers)
		{
			g_http->SetHTTPRequestHeaderValue(hReq, header.m_strName.c_str(), header.m_strValue.c_str());
		}
	}

	SteamAPICall_t hCall;
	if(!g_http->SendHTTPRequest(hReq, &hCall))
	{
		META_CONPRINTF("\nFailed to send request\n");
	}

	new TrackedRequest(hReq, hCall, pszUrl, pszText, callback);
}

void HTTPManager::GeneratePlayerRequest(EHTTPMethod method, const char *pszUrl, const char *pszText, PlayerCallbackFn callback,
										std::vector<HTTPHeader> *headers, KZPlayer *player)
{
	if (!g_http)
	{
		g_steamAPI.Init();
		g_http = g_steamAPI.SteamHTTP();
	}

	auto hReq = g_http->CreateHTTPRequest(method, pszUrl);
	int size = strlen(pszText);

	if ((method == k_EHTTPMethodPOST || method == k_EHTTPMethodPUT || method == k_EHTTPMethodPATCH)
		&& !g_http->SetHTTPRequestRawPostBody(hReq, "application/json", (uint8 *)pszText, size))
	{
		META_CONPRINTF("\nFailed to set request body\n");
		return;
	}

	if (headers != nullptr)
	{
		for (HTTPHeader header : *headers)
		{
			g_http->SetHTTPRequestHeaderValue(hReq, header.m_strName.c_str(), header.m_strValue.c_str());
		}
	}

	SteamAPICall_t hCall;
	if (!g_http->SendHTTPRequest(hReq, &hCall))
	{
		META_CONPRINTF("\nFailed to send request\n");
	}

	new TrackedPlayerRequest(hReq, hCall, pszUrl, pszText, callback, player);
}
