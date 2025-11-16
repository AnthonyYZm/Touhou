#ifndef BARRAGE_H
#define BARRAGE_H	
#include "Role.h"
#include "Enemy.h"

class Barrage : public Role{
	int aliveBarr1;
	int speed;
	DWORD t1, t2;
public:
	Barrage(float _x = 0, float _y = 0);
	void draw() override;
	void createBarrage(std::vector<Barrage>& barr, Enemy* enemy);
	void move();

	bool isAlive() { return alive; }	
};
#endif
