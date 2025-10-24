#ifndef ROLE_H
#define ROLE_H
#include "Library.h"

class Role {
public:

	float x, y;
	int hp;
	bool alive, fire;

	Role(float _x, float _y, int _hp) {
		x = _x;
		y = _y;
		hp = _hp;
		alive = true;
		fire = false;
	}

	Role(float _x, float _y) {
		x = _x;
		y = _y;
	}

	virtual void draw() = 0;
	
	//virtual void move() = 0;

	void putimagePNG(int x, int y, int w, int h, IMAGE* srcImg, int sx, int sy);

	void score(int idx);

};
#endif