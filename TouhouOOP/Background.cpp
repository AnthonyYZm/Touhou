#include "Background.h"


BackgroundManager::BackgroundManager() {
	scrollY = 0.0f;
	currentMode = BGMode::NORMAL;
}

void BackgroundManager::init() {
	loadimage(&bgNormal, L"resource/background/game.png", WIDTH, HEIGHT);
	loadimage(&bgSpellBase, L"resource/background/cdbg05a.png", WIDTH, HEIGHT);
	loadimage(&bgSpellLayer, L"resource/background/cdbg05b.png", WIDTH, WIDTH);
	loadimage(&tips, L"resource/background/tips.png");
}

void BackgroundManager::setMode(BGMode mode) {
	currentMode = mode;
}

void BackgroundManager::update() {
	// ÏòÏÂ¹ö¶¯
	float scrollSpeed = (currentMode == BGMode::BOSS_SPELL) ? 2.0f : 1.0f; // ·û¿¨½×¶Î¿ÉÒÔ¹ö¿ìµã
	scrollY += scrollSpeed;

	int currentBGHeight = (currentMode == BGMode::BOSS_SPELL) ? bgSpellLayer.getheight() : HEIGHT;

	if (scrollY >= currentBGHeight) {
		scrollY = 1;
	}
}


void TileDraw(IMAGE* img, float scrollY, int alpha = 255) {
	int imgW = img->getwidth();
	int imgH = img->getheight();

	for (int x = LeftEdge; x < LeftEdge + WIDTH; x += imgW) {
		for (int y = TopEdge - (imgH - (int)scrollY); y < TopEdge + HEIGHT; y += imgH) {
			putimagePNG(x, y, imgW, imgH, img, 0, 0, imgW, imgH);
		}
	}
}

void BackgroundManager::draw() {
	putimage(LeftEdge + WIDTH + 30, TopEdge, &tips);
	HRGN rgn = CreateRectRgn(LeftEdge, TopEdge, LeftEdge + WIDTH, TopEdge + HEIGHT);
	setcliprgn(rgn);
	if (currentMode == BGMode::NORMAL) {
		//ÆÕÍ¨±³¾°¹ö¶¯Âß¼­
		int offset = (int)scrollY;
		putimage(LeftEdge, TopEdge + offset, &bgNormal);
		putimage(LeftEdge, TopEdge + offset - HEIGHT, &bgNormal);
	}
	else if (currentMode == BGMode::BOSS_SPELL) {
		// µ×²ã
		putimage(LeftEdge, TopEdge, &bgSpellBase);
		// ¶¥²ã
		TileDraw(&bgSpellLayer, scrollY);
	}
	setcliprgn(NULL);
	DeleteObject(rgn);
}