#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <xaudio2.h>
#include "Library.h"
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

class AudioManager {
private:
	struct SoundData {
		std::vector<BYTE> pcm;
		WAVEFORMATEX* waveFormat = nullptr;
		bool isBGM = false;
		std::vector<IXAudio2SourceVoice*> voices;
		size_t nextVoice = 0;
	};

	std::map<std::wstring, std::wstring> sounds;
	std::map<std::wstring, SoundData> loadedSounds;
	std::wstring currentBGM;
	bool loaded;
	bool comInitialized;
	bool mfInitialized;
	IXAudio2* xaudio;
	IXAudio2MasteringVoice* masteringVoice;
	IXAudio2SourceVoice* bgmVoice;
	std::mutex audioMutex;

	static const int POOL_SIZE = 12;

	bool decodeToPCM(const std::wstring& path, SoundData& outData);
	void releaseAllSounds();
	void destroyVoices(SoundData& data);

public:
	AudioManager();
	~AudioManager();

	void init();
	void play(const std::wstring& name);
	void playBGM(const std::wstring& name);
	void stopBGM();
};
