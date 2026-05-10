// kz_option_test.cpp — chat commands for testing the typed KZPreference API.
// Usage in chat (prefix with ! or /):
//   kz_pref_compactpanel            — toggle compact panel
//   kz_pref_beamtype [0-3]          — get or set desiredBeamType (int)
//   kz_pref_jsvolume [0.0-2.0]      — get or set jsVolume (float)
//   kz_pref_beamoffset [x] [y] [z]  — get or set beamOffset (vector)

#include "src/common.h"
#include "kz/kz.h"
#include "kz/option/kz_option.h"
#include "utils/simplecmds.h"
#include "utils/utils.h"

// ── bool: compact panel ──────────────────────────────────────────────────────

SCMD(kz_pref_compactpanel, SCFL_PLAYER | SCFL_PREFERENCE)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	bool current = player->optionService->GetPreference<KZPreference::CompactPanel>();
	player->optionService->SetPreference<KZPreference::CompactPanel>(!current);
	bool readback = player->optionService->GetPreference<KZPreference::CompactPanel>();
	Msg("[KZPref] compactPanel: %s -> %s\n", current ? "true" : "false", readback ? "true" : "false");
	return MRES_SUPERCEDE;
}

// ── int: desiredBeamType ─────────────────────────────────────────────────────

SCMD(kz_pref_beamtype, SCFL_PLAYER | SCFL_PREFERENCE)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	i64 val = (i64)V_StringToInt32(args->Arg(1), 0);
	player->optionService->SetPreference<KZPreference::DesiredBeamType>(val);
	i64 readback = player->optionService->GetPreference<KZPreference::DesiredBeamType>();
	Msg("[KZPref] desiredBeamType: set=%lld readback=%lld\n", val, readback);
	return MRES_SUPERCEDE;
}

// ── float: jsVolume ──────────────────────────────────────────────────────────

SCMD(kz_pref_jsvolume, SCFL_PLAYER | SCFL_PREFERENCE)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	f64 val = utils::StringToFloat(args->Arg(1));
	player->optionService->SetPreference<KZPreference::JsVolume>(val);
	f64 readback = player->optionService->GetPreference<KZPreference::JsVolume>();
	Msg("[KZPref] jsVolume: set=%f readback=%f\n", val, readback);
	return MRES_SUPERCEDE;  
}

// ── vector: beamOffset ───────────────────────────────────────────────────────

SCMD(kz_pref_beamoffset, SCFL_PLAYER | SCFL_PREFERENCE)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	Vector val(
		(f32)utils::StringToFloat(args->Arg(1)),
		(f32)utils::StringToFloat(args->Arg(2)),
		(f32)utils::StringToFloat(args->Arg(3))
	);
	player->optionService->SetPreference<KZPreference::BeamOffset>(val);
	Vector readback = player->optionService->GetPreference<KZPreference::BeamOffset>();
	Msg("[KZPref] beamOffset: set=(%.2f,%.2f,%.2f) readback=(%.2f,%.2f,%.2f)\n",
		val.x, val.y, val.z, readback.x, readback.y, readback.z);
	return MRES_SUPERCEDE;
}

// ── string: language ─────────────────────────────────────────────────────────

SCMD(kz_pref_language, SCFL_PLAYER | SCFL_PREFERENCE)
{
    KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
    CUtlString val(args->Arg(1));
    player->optionService->SetPreference<KZPreference::Language>(val);
    CUtlString readback = player->optionService->GetPreference<KZPreference::Language>();
    Msg("[KZPref] language: set=%s readback=%s\n", val.Get(), readback.Get());
    return MRES_SUPERCEDE;
}
