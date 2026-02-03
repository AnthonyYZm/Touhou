#pragma once
#include <graphics.h>
#include "Role.h"

enum class BGMode {
	NORMAL,
	BOSS_SPELL
};

class BackgroundManager {
private:
	IMAGE bgNormal;
	IMAGE bgSpellBase;  
	IMAGE bgSpellLayer; // (¹ö¶¯²ã)
	IMAGE tips;
	BGMode currentMode;

	float scrollY; 

public:
	BackgroundManager();
	void init();
	void update();
	void draw();
	BGMode getMode() const { return currentMode; }
	void setMode(BGMode mode);
};