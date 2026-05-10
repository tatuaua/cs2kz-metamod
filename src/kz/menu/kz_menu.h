
/*
#pragma once
#include "../kz.h"

#define KZ_MENU_ON_STRING "<font color=#44ff44>ON</font>"
#define KZ_MENU_OFF_STRING "<font color=#888888>OFF</font>"
#define KZ_MENU_ITEM_FORMAT "<font color=#aaaaaa>  %s: %s</font><br>"
#define KZ_MENU_SELECTED_ITEM_FORMAT "<font color=#ffffff> %s: %s</font><br>"

class KZMenuService : public KZBaseService
{
	using KZBaseService::KZBaseService;

private:
	bool menuOpen {};
	i32 selectedItem {};

public:
	virtual void Reset() override;
	static void Init();

	void OpenMenu();
	void CloseMenu();
	void ToggleMenu();

	bool IsMenuOpen()
	{
		return this->menuOpen;
	}

	void MoveUp();
	void MoveDown();
	void SelectCurrent();

	// Called every physics tick: reads newly-pressed buttons and renders the menu.
	void OnPhysicsSimulate();
	void DrawMenu();
};
*/