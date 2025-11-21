#pragma once
#include "Library.h"
class Screen {

	IMAGE gameBg;
	bool start;	

public:
	Screen();
	void MenuScreen();	
	void setStart(bool s) { start = s; }
	void gameScreen();	
	void gameBackground();
};
