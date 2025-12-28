#pragma once	
#include "Role.h"
#include "Enemy.h"

/// <summary>
/// @brief Barrage class
/// @details Inherited from Role class, represents enemy barrages
/// </summary>
class Barrage : public Role{
private:
	float speed;
	float radian;	
	DWORD t1, t2;
	IMAGE barr1;
	
public:
	Barrage(float _x = 0, float _y = 0);
	~Barrage();
	void draw() override;
	void Normal(std::vector<Enemy*> enemy, Enemy* E);
	
	//* Barrages move by straight lines	
	//* gap: time gap between each round bullet
	//* angle : 
	//* num : number of bullets in one round
	
	void Straight(Enemy* enemy, int gap, float speed, float &angle);
	void Straight(Enemy* enemy, int gap, int x0, int y0, float speed, float &angle, int num);

	void Windmill(Enemy* enemy, int bladeNum);
	void move();
	bool isAlive() const { return alive; }	
	std::vector<Barrage*> barr1List;
	std::vector<Barrage*> barr2List;	
};

