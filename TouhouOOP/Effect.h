#pragma once
#include "Role.h" 

enum class EffectType {
	EXPLOSION,      // 敌人爆炸 (从小到大)
	CLEAR_SMALL,    // 清弹特效 (序列帧)
	SPELL_CUTIN,    // 符卡立绘 (移动)
	SPELL_NAME
};


struct EffectInstance {
	EffectType type;
	float x, y;
	int currentFrame;
	int totalFrames;
	DWORD startTime;      // 开始时间 (用于计算动画进度)
	DWORD lastFrameTime;  // 上一帧时间
	int frameInterval;    // 帧间隔
	IMAGE* imgPtr;        // 贴图指针
	// 剪裁参数 (源图上的坐标)
	int srcX, srcY, srcW, srcH;
	// 针对序列帧，记录每帧的偏移
	std::vector<POINT> frameOffsets;
	// 符卡立绘专用
	bool isPlayer;
	bool active;
	bool isPersistent; 
	float scale;       
};
class EffectManager {
private:
	std::vector<EffectInstance> effects;
	IMAGE imgDeadCircle;  // 敌人死亡
	IMAGE imgClear;       // 清弹
	IMAGE imgReimuSpell;  // 自机符卡
	IMAGE imgSanaeSpell;  // 敌人符卡
	IMAGE imgSpellName;
	bool loaded;

public:
	EffectManager();
	void init();

	// 通用生成接口
	// isPlayer 参数仅对 SPELL_CUTIN 有效
	void spawn(EffectType type, float x, float y, bool isPlayer = true);
	void clearSpellName();
	void update();
	void draw();
};