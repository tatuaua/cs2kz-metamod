#pragma once
#include "../kz.h"
#include <type_traits>
#include "utils/utils.h"
#include "KeyValues.h"
#include "interfaces/interfaces.h"
#include "filesystem.h"
#include "keyvalues3.h"
#include "utils/eventlisteners.h"

enum class KZPreference : i32
{
	CompactPanel = 0,
	DesiredBeamType,
	JsVolume,
	BeamOffset,
	Language
};

// Define the mapping traits
template<KZPreference P>
struct KZPreferenceTraits;

// Specializations: each provides 'key', 'Type', and 'GetDefault()'
// To add a preference: add an enum member above (before COUNT), then add a specialization below.
template<> struct KZPreferenceTraits<KZPreference::CompactPanel>    { static constexpr const char *key = "compactPanel";    using Type = bool;   static constexpr Type GetDefault() { return false;                       } };
template<> struct KZPreferenceTraits<KZPreference::DesiredBeamType> { static constexpr const char *key = "desiredBeamType"; using Type = i64;    static constexpr Type GetDefault() { return 0;                           } };
template<> struct KZPreferenceTraits<KZPreference::JsVolume>        { static constexpr const char *key = "jsVolume";        using Type = f64;    static constexpr Type GetDefault() { return 0.5;                         } };
template<> struct KZPreferenceTraits<KZPreference::BeamOffset>      { static constexpr const char *key = "beamOffset";      using Type = Vector; static         Type GetDefault() { return Vector(0.0f, 0.0f, 1.75f); } };
template<> struct KZPreferenceTraits<KZPreference::Language> 	  	{ static constexpr const char *key = "language";        using Type = CUtlString; static Type GetDefault() { return "english";                     } };

class KZOptionServiceEventListener
{
public:
	virtual void OnPlayerPreferencesLoaded(KZPlayer *player) {};
	virtual void OnPlayerPreferenceChanged(KZPlayer *player, const char *optionName) {};
};

class KZOptionService : public KZBaseService
{
	using KZBaseService::KZBaseService;

	DECLARE_CLASS_EVENT_LISTENER(KZOptionServiceEventListener);

public:
	static void InitOptions();
	static void Cleanup();
	static const char *GetOptionStr(const char *optionName, const char *defaultValue = "");
	static f64 GetOptionFloat(const char *optionName, f64 defaultValue = 0.0);
	static i64 GetOptionInt(const char *optionName, i64 defaultValue = 0);
	static KeyValues *GetOptionKV(const char *optionName);

private:
	static void LoadDefaultOptions();

private:
	enum
	{
		NONE = 0,
		LOCAL,
		GLOBAL
	} dataState, currentState;

	KeyValues3 prefKV = KeyValues3(KV3_TYPEEX_TABLE, KV3_SUBTYPE_UNSPECIFIED);
	CUtlVector<CUtlString> userSetPrefs; // Track user-modified preferences

public:
	void Reset()
	{
		dataState = NONE;
		currentState = NONE;
		prefKV.SetToEmptyTable();
		userSetPrefs.Purge();
	}

	void InitializeLocalPrefs(CUtlString text);
	void InitializeGlobalPrefs(std::string json);

	void SaveLocalPrefs();

	void SaveGlobalPrefs() {}

	void OnPlayerActive();

	void OnClientDisconnect()
	{
		SaveLocalPrefs();
		SaveGlobalPrefs();
	}

	void GetPreferencesAsJSON(CUtlString *error, CUtlString *output)
	{
		SaveKV3AsJSON(&this->prefKV, error, output);
	}

	template<KZPreference P>
    void SetPreference(typename KZPreferenceTraits<P>::Type value)
    {
        constexpr const char *prefName = KZPreferenceTraits<P>::key;

        if (userSetPrefs.Find(prefName) == userSetPrefs.InvalidIndex())
        {
            userSetPrefs.AddToTail(prefName);
        }

        using T = typename KZPreferenceTraits<P>::Type;

        if constexpr (std::is_same_v<T, bool>) {
            prefKV.FindOrCreateMember(prefName)->SetBool(value);
        } else if constexpr (std::is_same_v<T, i64>) {
            prefKV.FindOrCreateMember(prefName)->SetInt64(value);
        } else if constexpr (std::is_same_v<T, f64>) {
            prefKV.FindOrCreateMember(prefName)->SetDouble(value);
        } else if constexpr (std::is_same_v<T, CUtlString>) {
            prefKV.FindOrCreateMember(prefName)->SetString(value.Get());
        } else if constexpr (std::is_same_v<T, Vector>) {
            prefKV.FindOrCreateMember(prefName)->SetVector(value);
        }

        CALL_FORWARD(eventListeners, OnPlayerPreferenceChanged, this->player, prefName);
    }

	template<KZPreference P>
    typename KZPreferenceTraits<P>::Type GetPreference()
    {
        using T = typename KZPreferenceTraits<P>::Type;
        KeyValues3 *option = prefKV.FindMember(KZPreferenceTraits<P>::key);

        if (!option)
        {
            return KZPreferenceTraits<P>::GetDefault();
        }

        if constexpr (std::is_same_v<T, bool>) {
            return option->GetBool(KZPreferenceTraits<P>::GetDefault());
        } else if constexpr (std::is_same_v<T, i64>) {
            return option->GetInt64(KZPreferenceTraits<P>::GetDefault());
        } else if constexpr (std::is_same_v<T, f64>) {
            return option->GetDouble(KZPreferenceTraits<P>::GetDefault());
        } else if constexpr (std::is_same_v<T, CUtlString>) {
            return option->GetString(KZPreferenceTraits<P>::GetDefault());
        } else if constexpr (std::is_same_v<T, Vector>) {
            return option->GetVector(KZPreferenceTraits<P>::GetDefault());
        } else {
            static_assert(!sizeof(T), "Unsupported KZ preference type");
        }
    }

	// Due to the way keyvalues3.h is written, we can't template these functions.
	void SetPreferenceBool(const char *optionName, bool value)
	{
		if (userSetPrefs.Find(optionName) == userSetPrefs.InvalidIndex())
		{
			userSetPrefs.AddToTail(optionName); // Mark as user-set
		}
		prefKV.FindOrCreateMember(optionName)->SetBool(value);
		CALL_FORWARD(eventListeners, OnPlayerPreferenceChanged, this->player, optionName);
	}

	bool GetPreferenceBool(const char *optionName, bool defaultValue = false)
	{
		KeyValues3 *option = prefKV.FindMember(optionName);
		if (!option)
		{
			return defaultValue;
		}
		return option->GetBool(defaultValue);
	}

	void SetPreferenceFloat(const char *optionName, f64 value)
	{
		if (userSetPrefs.Find(optionName) == userSetPrefs.InvalidIndex())
		{
			userSetPrefs.AddToTail(optionName); // Mark as user-set
		}
		prefKV.FindOrCreateMember(optionName)->SetDouble(value);
		CALL_FORWARD(eventListeners, OnPlayerPreferenceChanged, this->player, optionName);
	}

	f64 GetPreferenceFloat(const char *optionName, f64 defaultValue = 0.0)
	{
		KeyValues3 *option = prefKV.FindMember(optionName);
		if (!option)
		{
			return defaultValue;
		}
		return option->GetDouble(defaultValue);
	}

	void SetPreferenceInt(const char *optionName, i64 value)
	{
		if (userSetPrefs.Find(optionName) == userSetPrefs.InvalidIndex())
		{
			userSetPrefs.AddToTail(optionName); // Mark as user-set
		}
		prefKV.FindOrCreateMember(optionName)->SetInt64(value);
		CALL_FORWARD(eventListeners, OnPlayerPreferenceChanged, this->player, optionName);
	}

	i64 GetPreferenceInt(const char *optionName, i64 defaultValue = 0)
	{
		KeyValues3 *option = prefKV.FindMember(optionName);
		if (!option)
		{
			return defaultValue;
		}
		return option->GetInt64(defaultValue);
	}

	void SetPreferenceStr(const char *optionName, const char *value)
	{
		if (userSetPrefs.Find(optionName) == userSetPrefs.InvalidIndex())
		{
			userSetPrefs.AddToTail(optionName); // Mark as user-set
		}
		prefKV.FindOrCreateMember(optionName)->SetString(value);
		CALL_FORWARD(eventListeners, OnPlayerPreferenceChanged, this->player, optionName);
	}

	const char *GetPreferenceStr(const char *optionName, const char *defaultValue = "")
	{
		KeyValues3 *option = prefKV.FindMember(optionName);
		if (!option)
		{
			return defaultValue;
		}
		// DebugPrintKV3(&prefKV);
		return option->GetString(defaultValue);
	}

	void SetPreferenceVector(const char *optionName, const Vector &value)
	{
		if (userSetPrefs.Find(optionName) == userSetPrefs.InvalidIndex())
		{
			userSetPrefs.AddToTail(optionName); // Mark as user-set
		}
		prefKV.FindOrCreateMember(optionName)->SetVector(value);
		CALL_FORWARD(eventListeners, OnPlayerPreferenceChanged, this->player, optionName);
	}

	Vector GetPreferenceVector(const char *optionName, const Vector &defaultValue = Vector(0.0f, 0.0f, 0.0f))
	{
		KeyValues3 *option = prefKV.FindMember(optionName);
		if (!option)
		{
			return defaultValue;
		}
		return option->GetVector(defaultValue);
	}

	void SetPreferenceTable(const char *optionName, const KeyValues3 &value)
	{
		if (userSetPrefs.Find(optionName) == userSetPrefs.InvalidIndex())
		{
			userSetPrefs.AddToTail(optionName); // Mark as user-set
		}
		KeyValues3 *option = prefKV.FindOrCreateMember(optionName);
		option->SetToEmptyTable();
		*option = value;
		CALL_FORWARD(eventListeners, OnPlayerPreferenceChanged, this->player, optionName);
	}

	void GetPreferenceTable(const char *optionName, KeyValues3 &output, const KeyValues3 &defaultValue = KeyValues3())
	{
		KeyValues3 *option = prefKV.FindMember(optionName);
		if (!option)
		{
			output = defaultValue;
			return;
		}
		output = *option;
	}
};
