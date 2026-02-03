#pragma once
#include <string>
#include <map>
#pragma comment(lib, "Winmm.lib")

class AudioManager {
private:
	std::map<std::wstring, std::wstring> sounds;
	std::wstring currentBGM;
	bool loaded;

public:
	AudioManager();
	~AudioManager();

	void init();
	void play(const std::wstring& name);
	void playFast(const std::wstring& path);
	void playBGM(const std::wstring& name);
	void stopBGM();
};
