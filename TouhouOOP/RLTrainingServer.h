#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <atomic>
#include <mutex>
#include <thread>
#include "RLEnvironment.h"
#include "json.hpp"

#pragma comment(lib, "ws2_32.lib")

class RLTrainingServer {
private:
	SOCKET udpSocket;
	sockaddr_in serverAddr;
	std::thread serverThread;
	std::atomic<bool> running;
	std::mutex envMutex;
	RLEnvironment environment;

	void ServerLoop();
	nlohmann::json BuildStateResponse(float reward, bool done);
	void SendJson(const nlohmann::json& response, const sockaddr_in& clientAddr, int clientLen);

public:
	RLTrainingServer(Hero* hero, EnemyManager* enemies, std::vector<Barrage*>* barrages);
	~RLTrainingServer();

	bool Init(int localPort = 9090);
	void Close();
	bool IsRunning() const { return running.load(); }
};
