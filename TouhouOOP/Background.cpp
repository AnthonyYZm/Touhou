#include "Background.h"
#include "Game.h"

BackgroundManager::BackgroundManager() {
	scrollY = 0.0f;
	currentMode = BGMode::NORMAL;
}

void BackgroundManager::init() {
	loadimage(&bgNormal, L"resource/background/game.png", WIDTH, HEIGHT);
	loadimage(&bgSpellBase, L"resource/background/cdbg05a.png", WIDTH, HEIGHT);
	loadimage(&bgSpellLayer, L"resource/background/cdbg05b.png", WIDTH, WIDTH);
}

void BackgroundManager::setMode(BGMode mode) {
	currentMode = mode;
}

void BackgroundManager::update() {
	if (currentMode == BGMode::BOSS_SPELL) {
		// 滚动逻辑
		scrollY += 2.0f; // 滚动速度
		if (scrollY >= bgSpellLayer.getheight()) {
			scrollY = 0;
		}
	}
}

// 辅助函数：平铺半透明绘制
// 注意：这需要 putimagePNG 支持 Alpha 混合，如果 cdbg05b.png 本身是带 Alpha 通道的 PNG，则直接用 putimagePNG。
// 如果 cdbg05b 是不透明图片但需要做成半透明效果，EasyX 需要额外处理。
// 这里假设图片本身带透明度或者你希望它以半透明覆盖。
// 如果原图不透，需要在加载时处理 alpha，或者修改绘制函数。
// 假设 cdbg05b.png 是带透明度的纹理。
void TileDraw(IMAGE* img, float scrollY, int alpha = 255) {
	int imgW = img->getwidth();
	int imgH = img->getheight();

	// 在游戏区域内平铺
	// GameWidth 和 GameHeight 需要在 Game.h 定义
	for (int x = LeftEdge; x < LeftEdge + WIDTH; x += imgW) {
		for (int y = TopEdge - (imgH - (int)scrollY); y < TopEdge + HEIGHT; y += imgH) {
			// 计算裁剪，确保不出界 (这里为了简化，直接画，依赖 putimagePNG 内部裁剪或屏幕裁剪)
			// 为了半透明，这里应该传递 alpha 参数给 putimagePNG (如果支持)
			// 假设素材已经是半透明 png
			putimagePNG(x, y, imgW, imgH, img, 0, 0, imgW, imgH);
		}
	}
}

void BackgroundManager::draw() {
	if (currentMode == BGMode::NORMAL) {
		// 普通背景，不滚动
		putimage(LeftEdge, TopEdge, &bgNormal);
	}
	else if (currentMode == BGMode::BOSS_SPELL) {
		// 1. 底层 (不滚动)
		putimage(LeftEdge, TopEdge, &bgSpellBase);

		// 2. 顶层 (滚动 + 平铺)
		// 假设 cdbg05b 是一张小图，需要平铺满屏幕
		TileDraw(&bgSpellLayer, scrollY);
	}
}