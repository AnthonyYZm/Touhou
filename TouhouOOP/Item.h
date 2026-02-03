#pragma once
#include "Role.h"

class Item : public Role {
private:
	float vx, vy;
	bool magnet;
	static IMAGE imgItem;
	static bool resLoaded;

public:

	Item(float _x, float _y);
	~Item();

	void draw() override;
	void move() override;

	void setMagnet(bool m) { magnet = m; }
	bool isMagnet() const { return magnet; }

	static int getWidth() { return 16; } 
	static int getHeight() { return 16; }
};
