#include "Effect.h"
//#include "Game.h" // 获取边界常量

EffectManager::EffectManager() { loaded = false; }

void EffectManager::init() {
	if (loaded) return;
	// 1. 加载资源
	loadimage(&imgDeadCircle, L"resource/barrage/eff_deadcircle.png");
	loadimage(&imgClear, L"resource/barrage/etama2.png");
	loadimage(&imgReimuSpell, L"resource/other/ReimuSpell.png");
	loadimage(&imgSanaeSpell, L"resource/other/sanaeSpell.png");
	loadimage(&imgSpellName, L"resource/text/spellcardAttack.png");
	loaded = true;
}

void EffectManager::spawn(EffectType type, float x, float y, bool isPlayer) {
	EffectInstance eff;
	eff.type = type;
	eff.x = x; eff.y = y;
	eff.currentFrame = 0;
	eff.startTime = GetTickCount();
	eff.lastFrameTime = eff.startTime;
	eff.active = true;
	eff.isPlayer = isPlayer;

	switch (type) {
	case EffectType::EXPLOSION:
		eff.imgPtr = &imgDeadCircle;
		eff.totalFrames = 15; // 动画持续帧数 (约300ms)
		eff.frameInterval = 0; // 不使用帧间隔，使用时间插值
		// 剪裁区域 (x=0,y=64,width=64,height=64)
		eff.srcX = 64; eff.srcY = 0; eff.srcW = 64; eff.srcH = 64;
		break;

	case EffectType::CLEAR_SMALL:
		eff.imgPtr = &imgClear;
		eff.totalFrames = 3;
		eff.frameInterval = 50; // 每帧 50ms
		// 记录三张剪切图的坐标
		// (65,224), (97,224), (158,224) 宽高均为 30
		eff.srcW = 30; eff.srcH = 30;
		eff.frameOffsets = { {65, 224}, {97, 224}, {158, 224} };
		// 修正中心位置
		eff.x -= 15; eff.y -= 15;
		break;

	case EffectType::SPELL_CUTIN:
		eff.imgPtr = isPlayer ? &imgReimuSpell : &imgSanaeSpell;
		eff.totalFrames = 1000; // 持续 1000ms
		eff.frameInterval = 0;
		// 假设立绘图是完整的，不需要剪裁
		eff.srcX = 0; eff.srcY = 0;
		eff.srcW = eff.imgPtr->getwidth();
		eff.srcH = eff.imgPtr->getheight();
		// 初始位置设置在屏幕左外侧
		eff.y = TopEdge; // 这里的y可以根据立绘高度调整
		break;
	case EffectType::SPELL_NAME:
		eff.imgPtr = &imgSpellName;
		eff.isPersistent = true; // 标记为常驻
		eff.srcW = imgSpellName.getwidth();
		eff.srcH = imgSpellName.getheight();
		eff.scale = 5.0f; // 初始很大
		// 初始位置设定在右下角
		eff.x = (float)(LeftEdge + WIDTH - 150);
		eff.y = (float)(TopEdge + HEIGHT - 100);
		break;
	}
	effects.push_back(eff);
}

void EffectManager::update() {
	DWORD now = GetTickCount();
	for (auto it = effects.begin(); it != effects.end(); ) {
		if (!it->active) {
			it = effects.erase(it); continue;
		}

		long elapsed = now - it->startTime;

		switch (it->type) {
		case EffectType::EXPLOSION:
			// 播放完毕自动销毁 (20帧 * 16ms ≈ 320ms)
			if (elapsed > 300) it->active = false;
			break;

		case EffectType::CLEAR_SMALL:
			if (now - it->lastFrameTime >= it->frameInterval) {
				it->currentFrame++;
				it->lastFrameTime = now;
				if (it->currentFrame >= it->totalFrames) it->active = false;
			}
			break;

		case EffectType::SPELL_CUTIN:
		{
			if (elapsed > 1000) it->active = false; // 1秒后结束

			// 计算移动逻辑 (Left -> Center -> Right)
			// 游戏区宽度
			float gw = WIDTH; // 需在头文件定义
			float startX = LeftEdge - it->srcW; // 左侧屏幕外
			float endX = LeftEdge + gw;         // 右侧屏幕外
			float centerX = LeftEdge + gw / 2 - it->srcW / 2; // 中间

			float t = (float)elapsed / 1000.0f; // 0.0 ~ 1.0

			if (t < 0.2f) {
				// 0~0.2s: 快速进场 (Left -> Center)
				float subT = t / 0.2f; // 归一化 0~1
				// 使用简单的插值或 EaseOut
				it->x = startX + (centerX - startX) * (1 - pow(1 - subT, 3));
			}
			else if (t < 0.8f) {
				// 0.2~0.8s: 缓慢移动 (Center 稍微动一点点，营造悬停感)
				float subT = (t - 0.2f) / 0.6f;
				it->x = centerX + 20.0f * subT; // 慢慢向右漂移 20px
			}
			else {
				// 0.8~1.0s: 快速离场 (Center -> Right)
				float subT = (t - 0.8f) / 0.2f;
				float currentCenter = centerX + 20.0f;
				it->x = currentCenter + (endX - currentCenter) * (subT * subT * subT);
			}
			break; 
		}
		case EffectType::SPELL_NAME:
		{
			float t = elapsed / 1000.0f; // 0.0 ~ 1.0 对应 1秒动画期

			if (t <= 0.3f) {
				// 1. 前 0.3s：快速收缩
				float subT = t / 0.3f;
				it->scale = 3.0f - (2.0f * subT); // 3.0 -> 1.0
			}
			else if (t <= 1.0f) {
				// 2. 0.3s~1.0s：从右下移动到右上
				float subT = (t - 0.3f) / 0.7f;
				it->scale = 1.0f;
				float startY = (float)(TopEdge + HEIGHT - 100);
				float endY = (float)(TopEdge + 50);
				it->y = startY + (endY - startY) * subT; // 线性移动
			}
			else {
				// 3. 1s 之后：静止在右上
				it->scale = 1.0f;
				it->y = (float)(TopEdge + 50);
			}
			break;
		}
		}
		++it;
	}
}

void EffectManager::draw() {
	DWORD now = GetTickCount();

	for (const auto& eff : effects) {
		if (!eff.active) continue;

		if (eff.type == EffectType::EXPLOSION) {
			// 从小到大缩放动画
			long elapsed = now - eff.startTime;
			float progress = elapsed / 300.0f;
			if (progress > 1.0f) progress = 1.0f;

			// Scale: 0.2 -> 1.5
			float scale = 0.2f + progress * 2.5f;

			int drawW = (int)(eff.srcW * scale);
			int drawH = (int)(eff.srcH * scale);
			// 保持中心对齐
			int drawX = (int)(eff.x - drawW / 2);
			int drawY = (int)(eff.y - drawH / 2);
			putimagePNG(drawX, drawY, eff.srcW, eff.srcH, eff.imgPtr, eff.srcX, eff.srcY, drawW, drawH);
		}
		else if (eff.type == EffectType::CLEAR_SMALL) {
			POINT p = eff.frameOffsets[eff.currentFrame];
			putimagePNG((int)eff.x, (int)eff.y, eff.srcW, eff.srcH, eff.imgPtr, p.x, p.y, eff.srcW, eff.srcH);
		}
		else if (eff.type == EffectType::SPELL_CUTIN) {
			// 直接绘制，x 已在 update 中计算好
			putimagePNG((int)eff.x, (int)eff.y, eff.srcW, eff.srcH, eff.imgPtr, eff.srcX, eff.srcY, eff.srcW, eff.srcH);
		}
		else if (eff.type == EffectType::SPELL_NAME) {
			int drawW = (int)(eff.srcW * eff.scale * 2);
			int drawH = (int)(eff.srcH * eff.scale * 2);
			// 中心点对齐绘制，防止缩放时位置乱跳
			putimagePNG((int)eff.x - drawW / 2, (int)eff.y - drawH / 2,
				eff.srcW, eff.srcH, eff.imgPtr, 0, 0, drawW, drawH);
		}
	}
}

void EffectManager::clearSpellName() {
	for (auto& eff : effects) {
		if (eff.type == EffectType::SPELL_NAME) {
			eff.active = false;
		}
	}
}