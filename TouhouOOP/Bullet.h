#ifndef BULLET_H
#define BULLET_H
#include "Role.h"

class Bullet : public Role {

	float bulletX;
	float bulletY;
	int bulletWidth;
	int bulletHeight;
	int speed;
	

public:

	bool Alive;

	Bullet(float _x = 0, float _y = 0);

	void draw() override;

	void move();

	float getBulletX() { return bulletX; }

	float getBulletY() { return bulletY; }

	bool isAlive() { return alive; }	

};


#endif
