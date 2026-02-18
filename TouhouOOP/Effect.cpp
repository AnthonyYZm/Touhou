#include "Effect.h"

EffectManager::EffectManager() { loaded = false; }

void EffectManager::init() {
	if (loaded) return;
	loadimage(&imgDeadCircle, L"resource/barrage/eff_deadcircle.png");
	loadimage(&imgClear, L"resource/barrage/etama2.png");
	loadimage(&imgReimuSpell, L"resource/other/ReimuSpell.png");
	loadimage(&imgSanaeSpell, L"resource/other/sanaeSpell.png");
	loadimage(&imgSpellName, L"resource/text/spellcardAttack.png");
	loadimage(&imgBarrage, L"resource/barrage/bullet_create.png");
	loadimage(&imgRibbon, L"resource/other/etama3.png");
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
		eff.frameInterval = 0; 
		// (x=0,y=64,width=64,height=64)
		eff.srcX = 64; eff.srcY = 0; eff.srcW = 64; eff.srcH = 64;
		break;

	case EffectType::CLEAR_SMALL:
		eff.imgPtr = &imgClear;
		eff.totalFrames = 3;
		eff.frameInterval = 50; // 每帧 50ms
		// (65,224), (97,224), (158,224)
		eff.srcW = 30; eff.srcH = 30;
		eff.frameOffsets = { {65, 224}, {97, 224}, {158, 224} };
		eff.x -= 15; eff.y -= 15;
		break;

	case EffectType::SPELL_CUTIN:
		eff.imgPtr = isPlayer ? &imgReimuSpell : &imgSanaeSpell;
		eff.totalFrames = 1500; // 持续 1000ms
		eff.frameInterval = 0;
		eff.srcX = 0; eff.srcY = 0;
		eff.srcW = eff.imgPtr->getwidth();
		eff.srcH = eff.imgPtr->getheight();
		eff.y = TopEdge;
		break;
	case EffectType::SPELL_NAME:
		eff.imgPtr = &imgSpellName;
		eff.isPersistent = true; 
		eff.srcW = imgSpellName.getwidth();
		eff.srcH = imgSpellName.getheight();
		eff.scale = 5.0f; 
		// 初始位置设定在右下角
		eff.x = (float)(LeftEdge + WIDTH - 150);
		eff.y = (float)(TopEdge + HEIGHT - 100);
		break;
	case EffectType::CREATE_BARRAGE:
		eff.imgPtr = &imgBarrage;
		eff.totalFrames = 5; 
		eff.frameInterval = 30; 
		eff.srcX = 0; eff.srcY = 0; eff.srcW = 32; eff.srcH = 32;
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
			if (elapsed > 1500) it->active = false; 
			float gw = WIDTH; 
			float startX = LeftEdge - it->srcW; 
			float endX = LeftEdge + gw;      
			float centerX = LeftEdge + gw / 2 - it->srcW / 2; 

			float t = (float)elapsed / 1500.0f; 

			if (t < 0.2f) {
				// 0~0.2s: 快速进场 
				float subT = t / 0.2f; 
				it->x = startX + (centerX - startX) * (1 - pow(1 - subT, 3));
			}
			else if (t < 0.8f) {
				// 0.2~0.8s: 缓慢移动 
				float subT = (t - 0.2f) / 0.6f;
				it->x = centerX + 20.0f * subT;
			}
			else {
				// 0.8~1.0s: 快速离场 
				float subT = (t - 0.8f) / 0.2f;
				float currentCenter = centerX + 20.0f;
				it->x = currentCenter + (endX - currentCenter) * (subT * subT * subT);
			}
			break; 
		}
		case EffectType::SPELL_NAME:
		{
			float t = elapsed / 1000.0f; 
			if (t <= 0.3f) {
				// 前 0.3s：快速收缩
				float subT = t / 0.3f;
				it->scale = 3.0f - (2.0f * subT); 
			}
			else if (t <= 1.0f) {
				// 0.3s~1.0s：从右下移动到右上
				float subT = (t - 0.3f) / 0.7f;
				it->scale = 1.0f;
				float startY = (float)(TopEdge + HEIGHT - 100);
				float endY = (float)(TopEdge + 50);
				it->y = startY + (endY - startY) * subT; 
			}
			else {
				// 1s 之后：静止在右上
				it->scale = 1.0f;
				it->y = (float)(TopEdge + 50);
			}
			break;
		}
		case EffectType::CREATE_BARRAGE:
			if (now - it->lastFrameTime >= it->frameInterval) {
				it->currentFrame++;
				it->lastFrameTime = now;
				if (it->currentFrame >= it->totalFrames) it->active = false;
			}
			break;
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
			long elapsed = now - eff.startTime;
			drawSpellRibbons(elapsed);
			putimagePNG((int)eff.x, (int)eff.y, eff.srcW, eff.srcH, eff.imgPtr, eff.srcX, eff.srcY, eff.srcW, eff.srcH);
		}
		else if (eff.type == EffectType::SPELL_NAME) {
			int drawW = (int)(eff.srcW * eff.scale * 2);
			int drawH = (int)(eff.srcH * eff.scale * 2);
			putimagePNG((int)eff.x - drawW / 2, (int)eff.y - drawH / 2,
				eff.srcW, eff.srcH, eff.imgPtr, 0, 0, drawW, drawH);
		}
		else if (eff.type == EffectType::CREATE_BARRAGE) {
			int currentSrcX = eff.srcX + (eff.currentFrame * eff.srcW);
			float progress = (float)eff.currentFrame / max(1, eff.totalFrames - 1);

			float startScale = 0.5f; 
			float endScale = 2.0f;  

			// 线性插值计算当前比例
			float currentScale = startScale + (endScale - startScale) * progress;

			// 根据比例计算最终绘制在屏幕上的宽高
			int dstW = (int)(eff.srcW * currentScale);
			int dstH = (int)(eff.srcH * currentScale);

			putimagePNG((int)(eff.x - dstW / 2), (int)(eff.y - dstH / 2), eff.srcW, eff.srcH,
				eff.imgPtr,0, 0, dstW, dstH);
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

void EffectManager::drawSpellRibbons(long elapsed) {
	int imgW = imgRibbon.getwidth();
	int imgH = 15;
	if (imgW <= 0) return; // 防止除以0

	int gapX = 200;
	int cycleW = imgW + gapX;
	int count = 12;      // 数量
	int spacing = HEIGHT / (count + 1);

	for (int i = 0; i < count; ++i) {
		float y = (float)(spacing * (i + 1));

		// 方向交替：偶数行向右(1)，奇数行向左(-1)
		int dir = (i % 2 == 0) ? 1 : -1;
		float speed = 0.5f;
		// 计算当前总位移
		float totalMove = elapsed * speed * dir;
		// 使用 fmod 对位移取模，确保 startX 永远在 [-imgW, 0] 之间循环
		float startX = fmod(totalMove, (float)cycleW);
		while (startX > -cycleW) {
			startX -= cycleW;
		}
		// 如果是向右滚动，取模后需要向左平移一个单位以填满左侧边缘
		while (startX > -imgW) {
			startX -= imgW;
		}
		// 从左侧起始点开始，一张接一张画，直到填满屏幕宽度
		for (float x = startX; x < WIDTH; x += cycleW) {
			// y - imgH / 2 是为了让带子中心对齐 y 坐标
			putimagePNG((int)x, (int)y - imgH / 2, imgW, imgH, &imgRibbon, 0, 97, imgW * 2, imgH * 2);
		}
	}
}