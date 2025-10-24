#include "Screen.h"

Screen::Screen() {
	loadimage(&gameBg, L"resource/background/game.png", WIDTH, HEIGHT);
}

void Screen::gameScreen() {
	initgraph(screenWidth, screenHeight);
	setbkcolor(WHITE);
}

void Screen::gameBackground() {
	putimage(LeftEdge, TopEdge, &gameBg);
}

void Screen::MenuScreen() {
	initgraph(screenWidth, screenHeight);
	setbkcolor(WHITE);
}