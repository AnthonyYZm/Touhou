#pragma once

#include "Role.h"
#include "Hero.h"	
#include "Bullet.h"
#include "Screen.h"
#include "Enemy.h"
#include "Barrage.h"
#include "EnemyManager.h"
#include "Boss.h"
#include "Item.h"
#include "Audio.h"     
#include "Effect.h"    
#include "Background.h"

class Game {

	static int bulletLevel;
	bool enemyFire;
	bool wait;
	Screen Scr;
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
	

public:

	static AudioManager Audio;
	static EffectManager Effects;
	static BackgroundManager BG;

	Game();
	~Game();

	void Touhou();
	void InitLevels();
	void HandleRound();
	void updateBoss();
	void Bullets();	
	void HeroControl();	
	void UpdateItems();
	void CheckCollision();
	void Barrages();
	void Enemies();
	void CastSpellCard();   
	void UpdateSpellCard(); 
	void ClearSpellBarrages(); 
	void handleBGM();
	
	bool checkCircleCollide(float x1, float y1, float r1, float x2, float y2, float r2) {
		float dx = x1 - x2;
		float dy = y1 - y2;
		float distSq = dx * dx + dy * dy;
		float rSum = r1 + r2;
		return distSq < (rSum * rSum);
	}
	
	void DrawDebug();
};

