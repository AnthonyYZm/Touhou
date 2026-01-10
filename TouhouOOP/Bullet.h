#pragma once
#include "Role.h"
#include "Hero.h"

/// <summary>
/// @brief Bullet class
/// @details Inherited from Role class, represents bullets fired by the hero
/// </summary>
class Bullet : public Role {

	static const int bulletWidth;
	static const int bulletHeight;
	float speed;
	DWORD t1, t2;
	IMAGE bullet1;
	using clock = std::chrono::steady_clock;
	clock::time_point next_fire = clock::now();    
	std::chrono::milliseconds fire_cd{ 100 };

public:
	std::vector<Bullet*> bulletList;

	Bullet(float _x = 0, float _y = 0);
	~Bullet();

	void draw() override;
	void move();
	void createBullet(Hero* hero, int type);
	bool isAlive() const { return alive; }
	void setFire(bool f) { fire = f; }

	static int getBulletWidth() { return bulletWidth; }
	static int getBulletHeight() { return bulletHeight; }
};


