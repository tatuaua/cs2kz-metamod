#include "httpmanager.h"
#include "../../hl2sdk-cs2/public/steam/steam_api_common.h"
#include "../../hl2sdk-cs2/public/steam/isteamhttp.h"
#include "../../vendor/nlohmann/json.hpp"

ISteamHTTP *g_http = nullptr;

HTTPManager g_HTTPManager;

#undef strdup

HTTPManager::TrackedRequest::TrackedRequest(HTTPRequestHandle hndl, SteamAPICall_t hCall, std::string strUrl, std::string strText,
											CompletedCallback callback)
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

void HTTPManager::TrackedRequest::OnHTTPRequestCompleted(HTTPRequestCompleted_t *arg, bool bFailed)
{
	uint32 size;
	g_http->GetHTTPResponseBodySize(arg->m_hRequest, &size);

	uint8 *response = new uint8[size + 1];
	g_http->GetHTTPResponseBodyData(arg->m_hRequest, response, size);
	response[size] = 0; // Add null terminator

	json jsonResponse;

	// Pass on response to the custom callback
	jsonResponse = json::parse((char *)response, nullptr, false);
	m_callback(arg->m_hRequest, arg->m_eStatusCode, jsonResponse);

	delete[] response;

	if (g_http)
	{
		g_http->ReleaseHTTPRequest(arg->m_hRequest);
	}

	delete this;
}

void HTTPManager::GET(const char *pszUrl, CompletedCallback callback, std::vector<HTTPHeader> *headers)
{
	GenerateRequest(k_EHTTPMethodGET, pszUrl, "", callback, headers);
}

void HTTPManager::POST(const char *pszUrl, const char *pszText, CompletedCallback callback, std::vector<HTTPHeader> *headers)
{
	GenerateRequest(k_EHTTPMethodPOST, pszUrl, pszText, callback, headers);
}

void HTTPManager::GenerateRequest(EHTTPMethod method, const char *pszUrl, const char *pszText, CompletedCallback callback,
								  std::vector<HTTPHeader> *headers)
{
	auto hReq = g_http->CreateHTTPRequest(method, pszUrl);
	int size = strlen(pszText);

	if (method == k_EHTTPMethodPOST && !g_http->SetHTTPRequestRawPostBody(hReq, "application/json", (uint8 *)pszText, size))
	{
		return;
	}

	if (headers != nullptr)
	{
		for (HTTPHeader header : *headers)
		{
			g_http->SetHTTPRequestHeaderValue(hReq, header.GetName(), header.GetValue());
		}
	}

	SteamAPICall_t hCall;
	g_http->SendHTTPRequest(hReq, &hCall);

	new TrackedRequest(hReq, hCall, pszUrl, pszText, callback);
}