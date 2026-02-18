#pragma once
#include "Role.h" 

enum class EffectType {
	EXPLOSION,      // 敌人爆炸 
	CLEAR_SMALL,    // 清弹特效 
	SPELL_CUTIN,    // 符卡立绘 
	SPELL_NAME,
	CREATE_BARRAGE
};


struct EffectInstance {
	EffectType type;
	float x, y;
	int currentFrame;
	int totalFrames;
	DWORD startTime;     
	DWORD lastFrameTime; 
	int frameInterval;    
	IMAGE* imgPtr;        
	int srcX, srcY, srcW, srcH;
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
	IMAGE imgBarrage;      // 弹幕生成特效
	IMAGE imgRibbon;
	bool loaded;

	void drawSpellRibbons(long elapsed);

public:
	EffectManager();
	void init();
	void spawn(EffectType type, float x, float y, bool isPlayer = true);
	void clearSpellName();
	void update();
	void draw();
};