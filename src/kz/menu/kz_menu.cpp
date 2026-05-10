
/*
#include "../kz.h"
#include "kz_menu.h"
#include "cs2kz.h"
#include "utils/simplecmds.h"
#include "kz/option/kz_option.h"
#include "kz/hud/kz_hud.h"
#include "kz/quiet/kz_quiet.h"

#include "tier0/memdbgon.h"



void KZMenuService::Init()
{
}

void KZMenuService::Reset()
{
	this->menuOpen    = false;
	this->selectedItem = 0;
}

void KZMenuService::OpenMenu()
{
	this->menuOpen = true;
}

void KZMenuService::CloseMenu()
{
	this->menuOpen = false;
	utils::PrintHTMLCentre(this->player->GetController(), "#SFUI_EmptyString");
}

void KZMenuService::ToggleMenu()
{
	if (this->menuOpen)
	{
		this->CloseMenu();
	}
	else
	{
		this->OpenMenu();
	}
}

void KZMenuService::MoveUp()
{
	if (!this->menuOpen)
	{
		return;
	}
	this->selectedItem = (this->selectedItem - 1 + (i32)KZPreference::COUNT) % (i32)KZPreference::COUNT;
}

void KZMenuService::MoveDown()
{
	if (!this->menuOpen)
	{
		return;
	}
	this->selectedItem = (this->selectedItem + 1) % (i32)KZPreference::COUNT;
}

void KZMenuService::SelectCurrent()
{
	if (!this->menuOpen)
	{
		return;
	}

	KZPreference pref = (KZPreference)this->selectedItem;
	bool current = this->player->optionService->GetPreferenceBool(pref);
	this->player->optionService->SetPreferenceBool(pref, !current);

	if (pref == KZPreference::HideWeapon)
	{
		this->player->quietService->ToggleHideWeapon();
	}
}

void KZMenuService::DrawMenu()
{
	if (!this->menuOpen)
    {
		return;
    }

	char buf[4096];
	char *p = buf;
	char *end = buf + sizeof(buf);

	for (i32 i = 0; i < (i32)KZPreference::COUNT && p < end - 128; i++)
	{
		KZPreference pref = (KZPreference)i;
		bool value = this->player->optionService->GetPreferenceBool(pref);
		const char *valStr = value ? KZ_MENU_ON_STRING : KZ_MENU_OFF_STRING;
		bool sel = (i == this->selectedItem);

		if (sel)
		{
			p += V_snprintf(p, end - p, KZ_MENU_SELECTED_ITEM_FORMAT,
							KZPreferenceInfo[i].key, valStr);
		}
		else
		{
			p += V_snprintf(p, end - p, KZ_MENU_ITEM_FORMAT,
							KZPreferenceInfo[i].key, valStr);
		}
	}

	this->player->PrintHTMLCentre(false, false, "%s", buf);
}

void KZMenuService::OnPhysicsSimulate()
{
	if (!this->menuOpen)
		return;

	if (this->player->IsButtonNewlyPressed(IN_FORWARD))
	{
		this->MoveUp();
	}
	if (this->player->IsButtonNewlyPressed(IN_BACK))
	{
		this->MoveDown();
	}
	if (this->player->IsButtonNewlyPressed(IN_USE))
	{
		this->SelectCurrent();
	}
	if (this->player->IsButtonNewlyPressed(IN_ATTACK2))
	{
		this->CloseMenu();
	}
}

SCMD(kz_menu, SCFL_PREFERENCE)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	player->menuService->ToggleMenu();
	return MRES_SUPERCEDE;
}
*/