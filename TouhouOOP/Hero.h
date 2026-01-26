#pragma once
#include "Role.h"

/// <summary>
///	@brief Hero class
/// @details Inherited from Role class, represents the player's character
/// </summary>
class Hero : public Role {

	int row;
	int col;
	static const int heroWidth;
	static const int heroHeight;
	int frame;
	int JudgeR;
	float Speed;
	//th:  hero frame rate 
	//tb : bullet fire rate
	DWORD th1, th2; 

public:

	Hero(float _x = WIDTH / 2, float _y = HEIGHT / 4 * 3);
	
	IMAGE hero;

	void draw() override;
	void move() override;
	void control(float speed);
	void JudgePoint();	

	static int getWidth() { return heroWidth; }
	static int getHeight() { return heroHeight; }
};
