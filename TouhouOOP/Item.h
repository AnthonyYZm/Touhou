#pragma once
#include "Role.h"

class Item : public Role {
private:
	float vx, vy;
	bool magnet; // 是否被自机吸附
	static IMAGE imgItem;
	static bool resLoaded;

public:

	Item(float _x, float _y);
	~Item();

	void draw() override;
	void move() override;

	// 触发吸附模式
	void setMagnet(bool m) { magnet = m; }
	bool isMagnet() const { return magnet; }

	static int getWidth() { return 16; } // 假设道具大小
	static int getHeight() { return 16; }
};
