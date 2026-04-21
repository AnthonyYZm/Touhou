#include "AIController.h"
#include "Library.h"
#include <iostream>
#include <cmath>

AIController::AIController()
	: udpSocket(INVALID_SOCKET)
	, pythonAddrSet(false)
	, running(false)
	, safeRadius(80.0f)       // 默认危险半径 80 像素
	, repulsionWeight(0.8f)   // 避障权重 80%
	, intentWeight(0.2f)      // 意图权重 20%
{
}

AIController::~AIController() {
	Close();
}

bool AIController::Init(int localPort) {
	// 初始化 Winsock
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		std::cerr << "WSAStartup failed: " << result << std::endl;
		return false;
	}

	// 创建 UDP Socket
	udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udpSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed" << std::endl;
		WSACleanup();
		return false;
	}

	// 设置为非阻塞模式
	u_long mode = 1;
	ioctlsocket(udpSocket, FIONBIO, &mode);

	// 绑定本地端口
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(localPort);

	if (bind(udpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
		closesocket(udpSocket);
		WSACleanup();
		return false;
	}

	pythonAddr.sin_family = AF_INET;
	pythonAddr.sin_port = htons(8080);
	inet_pton(AF_INET, "127.0.0.1", &pythonAddr.sin_addr);
	pythonAddrSet = true;

	// 启动接收线程
	running = true;
	recvThread = std::thread(&AIController::ReceiveThread, this);

	std::cout << "[AIController] 初始化成功，监听端口: " << localPort << std::endl;
	return true;
}

void AIController::Close() {
	if (running) {
		running = false;
		if (recvThread.joinable()) {
			recvThread.join();
		}
	}

	if (udpSocket != INVALID_SOCKET) {
		closesocket(udpSocket);
		udpSocket = INVALID_SOCKET;
	}

	WSACleanup();
	std::cout << "[AIController] 已关闭" << std::endl;
}

void AIController::ReceiveThread() {
	char buffer[4096];
	sockaddr_in fromAddr;
	int fromLen = sizeof(fromAddr);

	while (running) {
		// 非阻塞接收
		int recvLen = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0,
			(sockaddr*)&fromAddr, &fromLen);

		if (recvLen > 0) {
			buffer[recvLen] = '\0';

			// 记录 Python 服务器地址（用于后续发送）
			if (!pythonAddrSet) {
				pythonAddr = fromAddr;
				pythonAddrSet = true;
				std::cout << "[AIController] Python 服务器地址已记录" << std::endl;
			}

			// 解析 JSON 数据
			try {
				json j = json::parse(buffer);

				// 提取控制指令 (改为读取目标坐标)
				AICommand cmd;
				cmd.targetX = j.value("target_x", 375.0f);
				cmd.targetY = j.value("target_y", 800.0f);
				cmd.fire = j.value("fire", 1);

				// 线程安全地更新最新指令
				{
					std::lock_guard<std::mutex> lock(commandMutex);
					latestCommand = cmd;
				}
			}
			catch (json::parse_error& e) {
				std::cerr << "[AIController] JSON 解析错误: " << e.what() << std::endl;
			}
		}

		// 短暂休眠，避免 CPU 占用过高
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void AIController::SendGameState(const Hero& hero, const std::vector<Enemy*>& enemies,
	const std::vector<Barrage*>& bullets) {

	if (!pythonAddrSet) {
		return;  // Python 服务器地址未知，无法发送
	}

	// 构建 JSON 游戏状态
	json state;
	state["hero"] = {
		{"x", hero.x},
		{"y", hero.y},
		{"hp", hero.hp},
		{"alive", hero.alive}
	};

	// 敌人信息
	json enemyArray = json::array();
	for (const auto& enemy : enemies) {
		if (enemy && enemy->isAlive()) {
			enemyArray.push_back({
				{"x", enemy->x},
				{"y", enemy->y},
				{"hp", enemy->hp}
			});
		}
	}
	state["enemies"] = enemyArray;

	// 子弹信息（只发送前 50 颗，避免数据过大）
	json bulletArray = json::array();
	int count = 0;
	for (const auto& bullet : bullets) {
		if (bullet && bullet->isAlive() && count < 50) {
			bulletArray.push_back({
				{"x", bullet->x},
				{"y", bullet->y}
			});
			count++;
		}
	}
	state["bullets"] = bulletArray;

	// 转换为字符串并发送
	std::string stateStr = state.dump();
	sendto(udpSocket, stateStr.c_str(), (int)stateStr.length(), 0,
		(sockaddr*)&pythonAddr, sizeof(pythonAddr));
}

void AIController::CalculateRepulsion(const Hero& hero, const std::vector<Barrage*>& bullets,
	float& repulseX, float& repulseY) {

	repulseX = 0.0f;
	repulseY = 0.0f;

	// 遍历所有子弹，计算斥力
	for (const auto& bullet : bullets) {
		if (!bullet || !bullet->isAlive()) continue;

		// 计算自机到子弹的距离
		float dx = hero.x - bullet->x;
		float dy = hero.y - bullet->y;
		float distance = std::sqrt(dx * dx + dy * dy);

		// 如果在危险半径内，产生斥力
		if (distance < safeRadius && distance > 1.0f) {
			// 归一化方向向量（从子弹指向自机）
			float dirX = dx / distance;
			float dirY = dy / distance;

			// 斥力大小：距离越近，斥力越大（使用平方反比）
			float repulsionMagnitude = (safeRadius - distance) / safeRadius;
			repulsionMagnitude = repulsionMagnitude * repulsionMagnitude;  // 平方增强

			// 累加斥力向量
			repulseX += dirX * repulsionMagnitude;
			repulseY += dirY * repulsionMagnitude;
		}
	}

	// 归一化斥力向量（如果有斥力）
	float repulseMag = std::sqrt(repulseX * repulseX + repulseY * repulseY);
	if (repulseMag > 0.01f) {
		repulseX /= repulseMag;
		repulseY /= repulseMag;
	}
}

bool AIController::UpdateHeroControl(Hero& hero, const std::vector<Barrage*>& bullets) {
	// 1. 获取最新的宏观 AI 指令
	AICommand cmd;
	{
		std::lock_guard<std::mutex> lock(commandMutex);
		cmd = latestCommand;
	}

	float intentDx = cmd.targetX - hero.x;
	float intentDy = cmd.targetY - hero.y;
	float distToTarget = std::sqrt(intentDx * intentDx + intentDy * intentDy);

	// 只有距离目标大于 2 像素时才产生宏观推力，防止在目标点剧烈鬼畜抖动
	if (distToTarget > 2.0f) {
		intentDx /= distToTarget; // 归一化
		intentDy /= distToTarget;
	}
	else {
		intentDx = 0.0f;
		intentDy = 0.0f;
	}

	// 2. 计算弹幕产生的原始斥力
	float repulseX = 0.0f, repulseY = 0.0f;
	CalculateRepulsion(hero, bullets, repulseX, repulseY);

	// 3. 边界斥力逻辑
	float wallRepulseX = 0.0f, wallRepulseY = 0.0f;
	float wallMargin = 50.0f; // 边缘感应范围 (像素)

	if (hero.x < LeftEdge + wallMargin) {
		float dist = (hero.x - LeftEdge) < 1.0f ? 1.0f : (hero.x - LeftEdge);
		wallRepulseX += (wallMargin - dist) / wallMargin;
	}
	if (hero.x > Right - Hero::getWidth() - wallMargin) {
		float dist = (Right - Hero::getWidth() - hero.x) < 1.0f ? 1.0f : (Right - Hero::getWidth() - hero.x);
		wallRepulseX -= (wallMargin - dist) / wallMargin;
	}
	if (hero.y < TopEdge + wallMargin) {
		float dist = (hero.y - TopEdge) < 1.0f ? 1.0f : (hero.y - TopEdge);
		wallRepulseY += (wallMargin - dist) / wallMargin;
	}
	if (hero.y > Bottom - Hero::getHeight() - wallMargin) {
		float dist = (Bottom - Hero::getHeight() - hero.y) < 1.0f ? 1.0f : (Bottom - Hero::getHeight() - hero.y);
		wallRepulseY -= (wallMargin - dist) / wallMargin;
	}

	// 4. 方向分量过滤 (如果已经贴边，屏蔽掉推向边界的弹幕斥力分量)
	if (hero.x <= LeftEdge + 10.0f && repulseX < 0) repulseX = 0;
	if (hero.x >= Right - Hero::getWidth() - 10.0f && repulseX > 0) repulseX = 0;
	if (hero.y <= TopEdge + 10.0f && repulseY < 0) repulseY = 0;
	if (hero.y >= Bottom - Hero::getHeight() - 10.0f && repulseY > 0) repulseY = 0;

	// 5. 混合三种力：把原先的 cmd.dx 换成计算出来的 intentDx
	float finalDx = intentDx * intentWeight + repulseX * repulsionWeight + wallRepulseX * 0.6f;
	float finalDy = intentDy * intentWeight + repulseY * repulsionWeight + wallRepulseY * 0.6f;

	// 6. 向量归一化
	float magnitude = std::sqrt(finalDx * finalDx + finalDy * finalDy);
	if (magnitude > 0.01f) {
		finalDx /= magnitude;
		finalDy /= magnitude;
	}

	// 7. 应用移动
	float speed = 4.0f;
	hero.x += finalDx * speed;
	hero.y += finalDy * speed;

	// 8. 物理硬限制
	if (hero.x < LeftEdge) hero.x = (float)LeftEdge;
	if (hero.x > Right - Hero::getWidth()) hero.x = (float)(Right - Hero::getWidth());
	if (hero.y < TopEdge) hero.y = (float)TopEdge;
	if (hero.y > Bottom - Hero::getHeight()) hero.y = (float)(Bottom - Hero::getHeight());

	hero.fire = (cmd.fire == 1);

	return false;
}
