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
	pincer_aim,
	random_rain,
	pincer_rain,
	star_fall,
	windmill_switching,
	windmill
};

enum class BulletStyle {
	GREEN_, 
	BLUE_,
	RED_,
	RICE_BULE,
	RICE_RED,
	BLUE_BIG    
};

struct BulletRect {
	int x, y, w, h;
};;

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
	static const int riceWidth;
	static const int riceHeight;
	static int wheelGroup;
	bool outBound;
	DWORD t1, t2;
	static IMAGE mxtsSpell;
	static IMAGE imgGreen;
	static IMAGE imgBlue;
	static IMAGE imgRiceBlue;
	static IMAGE imgRiceRed;
	static IMAGE imgBlueBig;
	static IMAGE imgRed;
	IMAGE cacheImg;
	bool isRotated;
	static int globalGroupID;
	int groupID;
	BulletStyle style;	

public:

	bool isFriendly;	

	Barrage(float _x = 0, float _y = 0, BulletStyle _style = BulletStyle::GREEN_);
	~Barrage();
	void draw() override;
	void move() override;

	void Normal(Enemy& e, float speed, BulletStyle s);
	void randomRain(float speed, BulletStyle s);
	void straightMill(Enemy& e, float speed, int omega, int num, int x0, int y0, int dir, BulletStyle s = BulletStyle::GREEN_);
	void straightMill2(Enemy& e, float speed, int omega, int num, int x0, int y0, int dir, BulletStyle s = BulletStyle::GREEN_);
	void fireWork(Enemy& e, float speed, int num, int x0, int y0, BulletStyle s = BulletStyle::GREEN_);
	void circleMill(Enemy& e, float speed, int r, int num, int x0, int y0, BulletStyle s = BulletStyle::GREEN_);
	void pincerAim(Enemy& e, float targetX, float targetY, float speed, int spacing, int pairNum, int x0, int y0, BulletStyle s = BulletStyle::GREEN_);
	void fiveStar(Enemy& e, int currentStep, int totalSteps, float R_orbit, float r_star, float stretchSpeed, float normalAcc, 
		DWORD releaseTime, int starCount, int x0, int y0, BulletStyle s = BulletStyle::GREEN_);
	void directionalMill(Enemy& e, float speed, float baseAngle, int num, int x0, int y0, int dir, BulletStyle s = BulletStyle::GREEN_);
	
	void update();
	void reset();
	void clearBarrage();
	void collision();
	bool isAlive() const { return alive; }	
	void calRotation();
	std::vector<Barrage*> barrList;

	int getBarrageCount() const { return (int)barrList.size(); }
	BulletStyle getStyle() const { return style; }
	int getWidth() const;
	int getHeight() const;
	static BulletRect getRect(BulletStyle s);
	void setVx(float _vx) { vx = _vx; }
	void setVy(float _vy) { vy = _vy; }
};

