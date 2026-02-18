#pragma once
#include "Role.h"
#include "Audio.h"

class Hero : public Role {

	int row;
	int col;
	static const int heroWidth;
	static const int heroHeight;
	int frame;
	float Speed;
	DWORD th1, th2; 

	int lives;
	int power;
	static long long score;	

	int bombCount;
	int maxBomb;

public:

	int JudgeR;
	bool invincible;
	DWORD invincibleEnd;

	Hero(float _x = Right / 2 - heroWidth / 2, float _y = HEIGHT / 4 * 3 + TopEdge);
	
	IMAGE hero;

	void draw() override;
	void move() override;
	void control(float speed);
	void JudgePoint();	

	void hit(); 
	void addPower(int p);
	static void addScore(int s);
	bool tryUseBomb();
	void addBomb(int b);

	bool isInvincible() const { return invincible; }
	int getPower() const { return power; }
	int getLives() const { return lives; }
	int getBombs() const { return bombCount; }
	long long getScore() const { return score; }
	static int getWidth() { return heroWidth; }
	static int getHeight() { return heroHeight; }
};
