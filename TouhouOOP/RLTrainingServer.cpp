#pragma execution_character_set("utf-8")
#include "RLTrainingServer.h"
#include <iostream>
#include <string>

using json = nlohmann::json;

RLTrainingServer::RLTrainingServer(Hero* hero, EnemyManager* enemies, std::vector<Barrage*>* barrages)
	: udpSocket(INVALID_SOCKET)
	, running(false)
	, environment(hero, enemies, barrages) {
	ZeroMemory(&serverAddr, sizeof(serverAddr));
}

RLTrainingServer::~RLTrainingServer() {
	Close();
}

bool RLTrainingServer::Init(int localPort) {
	if (running.load()) {
		return true;
	}

	// 训练服务是 C++ 侧的“环境服务器”：Python Gym 环境只负责发 reset/step 请求。
	// 这里单独占用 9090，避免复用 AI 演示模式的 target_x/target_y 协议。
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		std::cerr << "[RLTrainingServer] WSAStartup failed: " << result << std::endl;
		return false;
	}

	udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udpSocket == INVALID_SOCKET) {
		std::cerr << "[RLTrainingServer] Socket creation failed: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return false;
	}

	DWORD timeoutMs = 100;
	setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));

	// 使用非长阻塞接收，关闭训练模式时线程最多等待一个短超时周期就能退出。
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons((u_short)localPort);

	if (bind(udpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "[RLTrainingServer] Bind failed on port " << localPort
			<< ": " << WSAGetLastError() << std::endl;
		closesocket(udpSocket);
		udpSocket = INVALID_SOCKET;
		WSACleanup();
		return false;
	}

	running = true;
	serverThread = std::thread(&RLTrainingServer::ServerLoop, this);
	std::cout << "[RLTrainingServer] Listening on UDP port " << localPort << std::endl;
	return true;
}

void RLTrainingServer::Close() {
	if (!running.load() && udpSocket == INVALID_SOCKET) {
		return;
	}

	// 先关闭 socket 再 join 线程，能打断 recvfrom 的等待，避免退出游戏时卡住。
	running = false;
	if (udpSocket != INVALID_SOCKET) {
		SOCKET socketToClose = udpSocket;
		udpSocket = INVALID_SOCKET;
		closesocket(socketToClose);
	}

	if (serverThread.joinable()) {
		serverThread.join();
	}

	WSACleanup();
	std::cout << "[RLTrainingServer] Closed" << std::endl;
}

json RLTrainingServer::BuildStateResponse(float reward, bool done) {
	RLState state = environment.GetObservation();
	json response;
	response["ok"] = true;
	// Python 端期望 state 是 JSON 对象，不是 JSON 字符串，所以这里再 parse 一次。
	response["state"] = json::parse(state.toJsonString());
	response["reward"] = reward;
	response["done"] = done;
	return response;
}

void RLTrainingServer::SendJson(const json& response, const sockaddr_in& clientAddr, int clientLen) {
	if (udpSocket == INVALID_SOCKET) {
		return;
	}

	const std::string payload = response.dump();
	sendto(udpSocket, payload.c_str(), (int)payload.size(), 0,
		(sockaddr*)&clientAddr, clientLen);
}

void RLTrainingServer::ServerLoop() {
	char buffer[8192];

	while (running.load()) {
		sockaddr_in clientAddr;
		int clientLen = sizeof(clientAddr);
		int recvLen = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0,
			(sockaddr*)&clientAddr, &clientLen);

		if (recvLen == SOCKET_ERROR) {
			const int err = WSAGetLastError();
			if (err == WSAETIMEDOUT || err == WSAEWOULDBLOCK) {
				continue;
			}
			if (!running.load()) {
				break;
			}
			std::cerr << "[RLTrainingServer] recvfrom failed: " << err << std::endl;
			continue;
		}

		buffer[recvLen] = '\0';

		try {
			json request = json::parse(buffer);
			const std::string command = request.value("command", "");

			// RLEnvironment 持有游戏对象指针；所有 reset/step 串行执行，避免训练线程并发改场景。
			std::lock_guard<std::mutex> lock(envMutex);
			if (command == "reset") {
				const int difficulty = request.value("difficultyLevel", 1);
				environment.Reset(difficulty);
				SendJson(BuildStateResponse(0.0f, false), clientAddr, clientLen);
			}
			else if (command == "step") {
				// Gym 的一次 step 对应 C++ 环境推进一帧，并立刻回传新观测和奖励。
				const int action = request.value("action", 0);
				environment.Step(action);
				const float reward = environment.CalculateReward();
				const bool done = environment.IsDone();
				SendJson(BuildStateResponse(reward, done), clientAddr, clientLen);
			}
			else {
				json response;
				response["ok"] = false;
				response["error"] = "unknown command";
				SendJson(response, clientAddr, clientLen);
			}
		}
		catch (const std::exception& e) {
			json response;
			response["ok"] = false;
			response["error"] = e.what();
			SendJson(response, clientAddr, clientLen);
		}
	}
}
