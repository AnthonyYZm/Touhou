#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include "json.hpp"
#include "Hero.h"
#include "Enemy.h"
#include "Barrage.h"

#pragma comment(lib, "ws2_32.lib")

using json = nlohmann::json;

// AI 控制指令结构体
struct AICommand {
	float targetX; // 目标X坐标
	float targetY; // 目标Y坐标
	int fire;      // 是否开火 (0 或 1)

	// 默认出生时前往屏幕中下方安全区 (X=375, Y=800)
	AICommand() : targetX(375.0f), targetY(800.0f), fire(0) {}
};

class AIController {
private:
	// 网络相关成员
	SOCKET udpSocket;
	sockaddr_in serverAddr;
	sockaddr_in pythonAddr;
	bool pythonAddrSet;

	// 线程相关成员
	std::thread recvThread;
	std::atomic<bool> running;
	std::mutex commandMutex;
	AICommand latestCommand;  // 最新接收到的 AI 指令

	// 避障算法参数
	float safeRadius;         // 危险半径（像素）
	float repulsionWeight;    // 斥力权重（0.0 到 1.0）
	float intentWeight;       // 意图权重（0.0 到 1.0）

	// 接收线程函数
	void ReceiveThread();

	// 计算斥力向量（局部避障算法）
	void CalculateRepulsion(const Hero& hero, const std::vector<Barrage*>& bullets,
		float& repulseX, float& repulseY);

public:
	AIController();
	~AIController();

	// 初始化网络（绑定端口）
	bool Init(int localPort = 9090);

	// 关闭网络和线程
	void Close();

	// 发送游戏状态到 Python 服务器
	void SendGameState(const Hero& hero, const std::vector<Enemy*>& enemies,
		const std::vector<Barrage*>& bullets);

	// 更新自机控制（混合 AI 意图和局部避障）
	bool UpdateHeroControl(Hero& hero, const std::vector<Barrage*>& bullets);

	// 设置避障参数
	void SetAvoidanceParams(float radius, float repWeight, float intWeight) {
		safeRadius = radius;
		repulsionWeight = repWeight;
		intentWeight = intWeight;
	}
};
