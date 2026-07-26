#pragma execution_character_set("utf-8")
#include "RLstate.h"
#include "json.hpp"

using json = nlohmann::json;

std::string RLState::toJsonString() const {
	json state;
	// 字段名和 touhou_env.py 的解析逻辑保持一致，避免协议两端各写各的。
	state["heroX"] = heroX;
	state["heroY"] = heroY;
	state["bossRelativeX"] = bossRelativeX;
	state["bossRelativeY"] = bossRelativeY;

	json bulletX = json::array();
	json bulletY = json::array();
	// 固定输出 MAX_OBSERVE_BULLETS 个槽位，Python 端始终得到 24 维观测。
	for (int i = 0; i < MAX_OBSERVE_BULLETS; ++i) {
		bulletX.push_back(bulletRelativeX[i]);
		bulletY.push_back(bulletRelativeY[i]);
	}

	state["bulletRelativeX"] = bulletX;
	state["bulletRelativeY"] = bulletY;
	return state.dump();
}
