#pragma once
#include <graphics.h>

enum class BGMode {
	NORMAL,
	BOSS_SPELL
};

class BackgroundManager {
private:
	IMAGE bgNormal;
	IMAGE bgSpellBase;  // cdbg05a
	IMAGE bgSpellLayer; // cdbg05b (¹ö¶¯²ã)

	BGMode currentMode;

	float scrollY; // ¹ö¶¯²ãµÄYÆ«ÒÆ

public:
	BackgroundManager();
	void init();
	void update();
	void draw();

	void setMode(BGMode mode);
};