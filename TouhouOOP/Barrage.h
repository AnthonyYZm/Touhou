#pragma once	
#include "Role.h"
#include "EnemyManager.h"


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
	windmill,
	curved_petal,
	brake_aim_ring,
	mirror_sine_wall,
	orbit_release
};

// 子弹运动状态。保留原有直线、环绕和延迟加速表现，并为高级弹幕提供明确语义。
enum class BulletMotion {
	LINEAR,
	ORBIT,
	EXPANDING_ORBIT,
	DELAYED_ACCELERATION,
	CURVE_THEN_STRAIGHT,
	BRAKE_THEN_AIM,
	ORBIT_THEN_RELEASE,
	SINE_WAVE
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
	float vx, vy;	
	BulletMotion motion;
	// 新运动的生命周期状态：时间均以 bornTime 为基准，避免 TickCount 回绕比较问题。
	DWORD bornTime;
	DWORD switchDelay;
	int motionPhase;
	float motionAngle;
	float motionSpeed;
	float turnRate;
	float deceleration;
	float targetX, targetY;
	float originX, originY;
	float travelled;
	float waveAmplitude;
	float waveFrequency;
	float releaseSpeed;
	float radiusGrowth;
	float radius;
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
	bool appendBullet(Barrage* bullet);

public:

	bool isFriendly;	

	Barrage(float _x = 0, float _y = 0, BulletStyle _style = BulletStyle::GREEN_);
	~Barrage();
	void draw() override;
	void move() override;

	void Normal(Enemy& e, float speed, BulletStyle s);
	void randomRain(float speed, BulletStyle s);
	void straightMill(Enemy& e, float speed, float baseAngle, int num, int x0, int y0, int dir, BulletStyle s = BulletStyle::GREEN_);
	void straightMill2(Enemy& e, float speed, float baseAngle, int num, int x0, int y0, int dir, BulletStyle s = BulletStyle::GREEN_);
	void fireWork(Enemy& e, float speed, int num, int x0, int y0, BulletStyle s = BulletStyle::GREEN_);
	void circleMill(Enemy& e, float speed, int r, int num, int x0, int y0, BulletStyle s = BulletStyle::GREEN_);
	void pincerAim(Enemy& e, float targetX, float targetY, float speed, int spacing, int pairNum, int x0, int y0, BulletStyle s = BulletStyle::GREEN_);
	void fiveStar(Enemy& e, int currentStep, int totalSteps, float R_orbit, float r_star, float stretchSpeed, float normalAcc, 
		DWORD releaseTime, int starCount, int x0, int y0, BulletStyle s = BulletStyle::GREEN_);
	void directionalMill(Enemy& e, float speed, float baseAngle, int num, int x0, int y0, int dir, BulletStyle s = BulletStyle::GREEN_);
	// 曲线花瓣、定向刹停环、镜像正弦风墙和环绕切向释放。
	void curvedPetal(Enemy& e, float speed, float baseAngle, int num, DWORD curveTime, int dir, float turnRate, int x0, int y0, BulletStyle s = BulletStyle::GREEN_);
	void brakeAimRing(Enemy& e, float targetX, float targetY, float speed, float baseAngle, int num, DWORD waitTime, float deceleration, int x0, int y0, BulletStyle s = BulletStyle::GREEN_);
	void mirrorSineWall(Enemy& e, float speed, float frequency, int perSide, float amplitude, int dir, float downwardDrift, int x0, int y0, BulletStyle s = BulletStyle::GREEN_);
	void orbitRelease(Enemy& e, float speed, float angularSpeed, int num, DWORD orbitTime, int dir, float radiusGrowth, int x0, int y0, BulletStyle s = BulletStyle::GREEN_);
	
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

