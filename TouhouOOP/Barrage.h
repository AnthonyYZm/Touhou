#pragma once	
#include "Role.h"
#include "EnemyManager.h"

/// <summary>
/// @brief Barrage class
/// @details Inherited from Role class, represents enemy barrages
/// </summary>

enum class bType : int {
	down_st,
	windmill_st,
	firework,
	circle_mill,
	wheel,
	pincer_aim,
	random_rain,
	pincer_rain,
	star_fall,
	windmill_switching
};


class Barrage : public Role{
private:
	float speed;
	float rSpeed;
	float radian;	
	float currentAngle;
	float vx, vy;	
	int moveType;
	float omega;
	float radius;
	float centerX, centerY;
	static const int darkGreenWidth;
	static const int darkGreenHeight;
	static int wheelGroup;
	bool outBound;
	DWORD t1, t2;
	static IMAGE barr1, mxtsSpell;
	static int globalGroupID;
	int groupID;

public:

	bool isFriendly;	

	Barrage(float _x = 0, float _y = 0);
	~Barrage();
	void draw() override;
	void move() override;

	void Normal(Enemy& e, float speed);
	void randomRain(float speed);
	void straightMill(Enemy& e, float speed, int omega, int num, int x0, int y0, int dir);
	void wheel(Enemy& e, float speed, float vl, int num, int x0, int y0);
	void fireWork(Enemy& e, float speed, int num, int x0, int y0);
	void circleMill(Enemy& e, float speed, int r, int num, int x0, int y0);
	void pincerAim(Enemy& e, float targetX, float targetY, float speed, int spacing, int pairNum, int x0, int y0);
	void fiveStar(Enemy& e, int currentStep, int totalSteps, float R_orbit, float r_star, float stretchSpeed, float normalAcc, DWORD releaseTime, int starCount, int x0, int y0);
	void directionalMill(Enemy& e, float speed, float baseAngle, int num, int x0, int y0);		
	
	void update();
	void reset();
	void clearBarrage();
	void collision();
	bool isAlive() const { return alive; }	
	std::vector<Barrage*> barrList;

	int getBarrageCount() const { return barrList.size(); }
	static int getDarkGreenWidth() { return darkGreenWidth; }
	static int getDarkGreenHeight() { return darkGreenHeight; }
	void setVx(float _vx) { vx = _vx; }
	void setVy(float _vy) { vy = _vy; }
};

