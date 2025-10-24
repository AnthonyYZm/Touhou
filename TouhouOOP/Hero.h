#ifndef HERO_H
#define HERO_H
#include "Role.h"

class Hero : public Role {

	int row;
	int col;
	int heroWidth;
	int heroHeight;
	int frame;
	int JudgeR;
	float Speed;
	/*th:  hero frame rate 
	* tb : bullet fire rate
	*/
	DWORD th1, th2; 

public:

	Hero(float _x = WIDTH / 2, float _y = HEIGHT / 4 * 3);
	
	IMAGE hero;

	void draw() override;

	void move();
	
	void control(float speed);

	void JudgePoint();	

};
#endif
