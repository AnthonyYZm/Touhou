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
	// 无论什么模式，背景都向下滚动
	float scrollSpeed = (currentMode == BGMode::BOSS_SPELL) ? 2.0f : 1.0f; // 符卡阶段可以滚快点
	scrollY += scrollSpeed;

	// 获取当前背景的高度进行取模，防止 scrollY 无限增加
	int currentBGHeight = (currentMode == BGMode::BOSS_SPELL) ? bgSpellLayer.getheight() : HEIGHT;

	if (scrollY >= currentBGHeight) {
		scrollY = 1;
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
	putimage(LeftEdge + WIDTH + 30, TopEdge, &tips);
	HRGN rgn = CreateRectRgn(LeftEdge, TopEdge, LeftEdge + WIDTH, TopEdge + HEIGHT);
	setcliprgn(rgn);
	if (currentMode == BGMode::NORMAL) {
		// --- 普通背景无缝滚动逻辑 ---
		int offset = (int)scrollY;

		// 1. 绘制第一张图（下方）
		putimage(LeftEdge, TopEdge + offset, &bgNormal);

		// 2. 绘制第二张图（上方，填补顶部的空缺）
		putimage(LeftEdge, TopEdge + offset - HEIGHT, &bgNormal);
	}
	else if (currentMode == BGMode::BOSS_SPELL) {
		// 1. 底层 (不滚动)
		putimage(LeftEdge, TopEdge, &bgSpellBase);

		// 2. 顶层 (滚动 + 平铺)
		TileDraw(&bgSpellLayer, scrollY);
	}
	setcliprgn(NULL);
	DeleteObject(rgn);
}