#pragma once
#include <string>
#include <map>
#pragma comment(lib, "Winmm.lib")

// 使用 mciSendString 需要包含此库
#pragma comment(lib, "Winmm.lib")

class AudioManager {
private:
	// 存储音效别名对应的文件路径
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
