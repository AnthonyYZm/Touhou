#pragma once
#define WIN32_LEAN_AND_MEAN
#include "Library.h"
#include "Role.h"
#include "Hero.h"
#include "Bullet.h"
#include "Enemy.h"
#include "Barrage.h"
#include "EnemyManager.h"
#include "Boss.h"
#include "Item.h"
#include "Audio.h"
#include "Effect.h"
#include "Background.h"
#include "AIController.h"
#include <imm.h>
#pragma comment(lib, "imm32.lib")
#include <functional>
#include <unordered_map>

// 游戏状态枚举
enum class GameState {
	MAIN_MENU,    // 主菜单
	NORMAL_PLAY,  // 普通模式
	AI_DEMO       // AI演示模式
};

// 弹幕处理函数的统一签名
// 参数：(任务引用, 敌人引用, x, y, centerX, centerY, 子弹样式, 当前速度, 当前时间戳)
using BarrageHandler = std::function<void(BarrageTask&, Enemy&, int, int, int, int, BulletStyle, float, DWORD)>;

// 为 enum class bType 提供哈希支持，使其可作为 unordered_map 的键
struct bTypeHash {
	std::size_t operator()(bType t) const noexcept {
		return std::hash<int>()(static_cast<int>(t));
	}
};

class Game {

	static int bulletLevel;
	bool enemyFire;
	bool wait;
	EnemyManager E;
	Hero Hero;
	Barrage Barr;
	Bullet B;

	std::queue<waveData> waveQueue;
	std::vector<SpawnEvent> currentWave;
	waveData nextWave;
	std::vector<Item*> items;
	std::vector<Barrage*> spellBarrages;
	bool isSpellActive;
	float spellRadius;
	float spellAngle;

	DWORD waitStart;
	bool isFullScreen = false;
	std::wstring currentBGM;

	// 状态机相关
	GameState currentState;

	// 主菜单：上一帧起缓存的鼠标客户区坐标（避免依赖消息队列导致悬停闪烁）
	int menuCursorX = -1;
	int menuCursorY = -1;

	// AI 控制器
	AIController* aiController;
	HANDLE aiProcessHandle = NULL;
	std::unordered_map<bType, BarrageHandler, bTypeHash> barrageHandlers; // 弹幕分发表

	// 离屏缓冲区（固定游戏分辨率，用于缩放输出）
	HDC     offDC   = nullptr;
	HBITMAP offBmp  = nullptr;
	HBITMAP offOld  = nullptr;

	// 将窗口坐标映射到游戏逻辑坐标（用于菜单鼠标判断）
	void mapWindowToGame(int winX, int winY, int& gameX, int& gameY);
	// 每帧将离屏缓冲等比缩放输出到窗口
	void presentScaled();

public:

	static AudioManager Audio;
	static EffectManager Effects;
	static BackgroundManager BG;

	Game();
	~Game();

	void Touhou();
	void InitNormalLevels();    
	void InitAILevel();
	void HandleRound();
	void updateBoss();
	void Bullets();	
	void HeroControl();
	void UpdateItems();
	void CheckCollision();
	void Barrages();
	void InitBarrageHandlers(); // 初始化弹幕分发表
	void Enemies();
	void CastSpellCard();
	void UpdateSpellCard();
	void ClearSpellBarrages();
	void handleBGM();
	void adjustWindow();
	void drawUI();

	// 主菜单相关方法
	void DrawMainMenu();      // 绘制主菜单界面
	void HandleMenuInput();   // 处理菜单输入
	void InitNormalPlay();    // 初始化普通模式
	void InitAIDemo();        // 初始化AI演示模式
	void HandleAIDemo();      // AI演示模式主逻辑
	
	// Åö×²¼ì²â
	bool checkCircleCollide(float x1, float y1, float r1, float x2, float y2, float r2) {
		float dx = x1 - x2;
		float dy = y1 - y2;
		float distSq = dx * dx + dy * dy;
		float rSum = r1 + r2;
		return distSq < (rSum * rSum);
	}
	
	void DrawDebug();
	
};

