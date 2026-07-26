#pragma once
#include <vector>
#include <string>
#include "Hero.h"
#include "Barrage.h"
#include "Enemy.h"

// 定义固定观察数量，防止神经网络输入长度动态变化
#define MAX_OBSERVE_BULLETS 10 

struct RLState {
	float heroX;
	float heroY;
	float bossRelativeX;
	float bossRelativeY;
	float bulletRelativeX[MAX_OBSERVE_BULLETS];
	float bulletRelativeY[MAX_OBSERVE_BULLETS];

	// 清空状态
	void clear() {
		heroX = 0; heroY = 0;
		bossRelativeX = 0; bossRelativeY = 0;
		for (int i = 0; i < MAX_OBSERVE_BULLETS; ++i) {
			bulletRelativeX[i] = 0;
			bulletRelativeY[i] = 0;
		}
	}

	// 将当前状态打包成 JSON 字符串，准备通过 UDP 发给 Python
	std::string toJsonString() const;
};
