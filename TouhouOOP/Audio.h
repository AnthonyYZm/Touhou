#pragma once
#include <string>
#include <map>
#include "Library.h"
#pragma comment(lib, "Winmm.lib")

class AudioManager {
private:
	std::map<std::wstring, std::wstring> sounds;
	std::wstring currentBGM;
	bool loaded;

	static const int POOL_SIZE = 15;
	std::map<std::wstring, int> poolIndex;
	std::vector<std::wstring> pooledSounds;
	std::map<std::wstring, DWORD> lastPlayTime;

public:
	AudioManager();
	~AudioManager();

	void init();
	void play(const std::wstring& name);
	void playBGM(const std::wstring& name);
	void stopBGM();
};
