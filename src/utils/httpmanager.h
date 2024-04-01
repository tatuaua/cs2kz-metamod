#pragma once
/*
 * Credit to CS2Fixes: https://github.com/Source2ZE/CS2Fixes/blob/40a7f3d9f479aeb8f1d0a5acb61d615c07176e43/src/httpmanager.h
 */

#include "../cs2kz.h"
#undef snprintf
#include <steam/steam_gameserver.h>

class HTTPManager;
extern HTTPManager g_HTTPManager;

#define CallbackFn std::function<void(HTTPRequestHandle, int, std::string)>

class HTTPHeader
{
public:
	HTTPHeader(std::string strName, std::string strValue)
	{
		m_strName = strName;
		m_strValue = strValue;
	}

	std::string m_strName;
	std::string m_strValue;
};

class HTTPManager
{
public:
	void GET(const char *pszUrl, CallbackFn callback, std::vector<HTTPHeader> *headers = nullptr);
	void POST(const char *pszUrl, const char *pszText, CallbackFn callback, std::vector<HTTPHeader> *headers = nullptr);
	void PUT(const char *pszUrl, const char *pszText, CallbackFn callback, std::vector<HTTPHeader> *headers = nullptr);
	void PATCH(const char *pszUrl, const char *pszText, CallbackFn callback, std::vector<HTTPHeader> *headers = nullptr);
	// DELETE is a macro
	void HTTP_DELETE(const char *pszUrl, CallbackFn callback, std::vector<HTTPHeader> *headers = nullptr);

	bool HasAnyPendingRequests() const
	{
		return m_PendingRequests.size() > 0;
	}

private:
	class TrackedRequest
	{
	public:
		TrackedRequest(const TrackedRequest &req) = delete;
		TrackedRequest(HTTPRequestHandle hndl, SteamAPICall_t hCall, std::string strUrl, std::string strText, CallbackFn callback);
		~TrackedRequest();

	private:
		void OnHTTPRequestCompleted(HTTPRequestCompleted_t *arg, bool bFailed);

		HTTPRequestHandle m_hHTTPReq;
		CCallResult<TrackedRequest, HTTPRequestCompleted_t> m_CallResult;
		std::string m_strUrl;
		std::string m_strText;
		CallbackFn m_callback;
	};

private:
	std::vector<HTTPManager::TrackedRequest *> m_PendingRequests;
	void GenerateRequest(EHTTPMethod method, const char *pszUrl, const char *pszText, CallbackFn callback, std::vector<HTTPHeader> *headers);
};
