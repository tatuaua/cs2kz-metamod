#pragma once
/*
 * Credit to CS2Fixes: https://github.com/Source2ZE/CS2Fixes/blob/40a7f3d9f479aeb8f1d0a5acb61d615c07176e43/src/httpmanager.h
 */

#include "../kz/kz.h"
#include "../cs2kz.h"
#undef snprintf
#include <steam/steam_gameserver.h>

class HTTPManager;
extern HTTPManager g_HTTPManager;

#define CallbackFn std::function<void(HTTPRequestHandle, int, std::string, bool)>
#define PlayerCallbackFn std::function<void(HTTPRequestHandle, int, std::string, bool, KZPlayer *player)>

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

	void GET(const char *pszUrl, PlayerCallbackFn callback, std::vector<HTTPHeader> *headers = nullptr, KZPlayer *player = nullptr);
	void POST(const char *pszUrl, const char *pszText, PlayerCallbackFn callback, std::vector<HTTPHeader> *headers = nullptr,
			  KZPlayer *player = nullptr);
	void PUT(const char *pszUrl, const char *pszText, PlayerCallbackFn callback, std::vector<HTTPHeader> *headers = nullptr,
			 KZPlayer *player = nullptr);
	void PATCH(const char *pszUrl, const char *pszText, PlayerCallbackFn callback, std::vector<HTTPHeader> *headers = nullptr,
			   KZPlayer *player = nullptr);
	// DELETE is a macro
	void HTTP_DELETE(const char *pszUrl, PlayerCallbackFn callback, std::vector<HTTPHeader> *headers = nullptr, KZPlayer *player = nullptr);

	bool HasAnyPendingRequests() const
	{
		return m_PendingRequests.size() > 0 && m_PendingPlayerRequests.size() > 0;
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

	class TrackedPlayerRequest
	{
	public:
		TrackedPlayerRequest(const TrackedPlayerRequest &req) = delete;
		TrackedPlayerRequest(HTTPRequestHandle hndl, SteamAPICall_t hCall, std::string strUrl, std::string strText, PlayerCallbackFn callback, KZPlayer *player);
		~TrackedPlayerRequest();

	private:
		void OnHTTPRequestCompleted(HTTPRequestCompleted_t *arg, bool bFailed);

		HTTPRequestHandle m_hHTTPReq;
		CCallResult<TrackedPlayerRequest, HTTPRequestCompleted_t> m_CallResult;
		std::string m_strUrl;
		std::string m_strText;
		PlayerCallbackFn m_callback;
		KZPlayer *m_player;
	};

private:
	std::vector<HTTPManager::TrackedRequest *> m_PendingRequests;
	std::vector<HTTPManager::TrackedPlayerRequest *> m_PendingPlayerRequests;
	void GenerateRequest(EHTTPMethod method, const char *pszUrl, const char *pszText, CallbackFn callback, std::vector<HTTPHeader> *headers);
	void GeneratePlayerRequest(EHTTPMethod method, const char *pszUrl, const char *pszText, PlayerCallbackFn callback,
							   std::vector<HTTPHeader> *headers, KZPlayer *player);
};
