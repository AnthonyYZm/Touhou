#include "Audio.h"
#include <windows.h>
#include <iostream>
#include <mmsystem.h>

AudioManager::AudioManager() { loaded = false; }

AudioManager::~AudioManager() {
	mciSendString(L"close all", NULL, 0, NULL);
}

void AudioManager::init() {
	if (loaded) return;
	sounds[L"fire"] = L"resource/sound/fire.wav";
	sounds[L"hit"] = L"resource/sound/se_damage00.wav";
	sounds[L"break"] = L"resource/sound/se_enep00.wav"; 
	sounds[L"barrage"] = L"resource/sound/barrage_tan2.wav";
	sounds[L"spell"] = L"resource/sound/spell.wav";
	sounds[L"clear"] = L"resource/sound/se_enep00.wav";
	sounds[L"dead"] = L"resource/sound/se_pldead00.wav";
	sounds[L"pickup"] = L"resource/sound/se_item00.wav"; 
	sounds[L"breakBoss"] = L"resource/sound/break_boss.wav"; 

	pooledSounds = { L"barrage", L"break", L"clear", L"hit", L"pickup" };

	sounds[L"bgm_stage1"] = L"resource/bgm/【東方風神録】～ 神々が恋した幻想郷 ～　.mp3";
	sounds[L"bgm_sanae"] = L"resource/bgm/【東方風神録】～ 信仰は儚き人間の為に ～　.mp3";

	// 预加载 
	for (auto const& [name, path] : sounds) {
		// 检查这个音效是否属于高频音效
		bool isPooled = std::find(pooledSounds.begin(), pooledSounds.end(), name) != pooledSounds.end();

		if (isPooled) {
			// 如果是高频音效，加载 POOL_SIZE 次
			for (int i = 0; i < POOL_SIZE; ++i) {
				std::wstring alias = name + L"_" + std::to_wstring(i);
				std::wstring cmd = L"open \"" + path + L"\" alias " + alias;
				mciSendString(cmd.c_str(), NULL, 0, NULL);
			}
			poolIndex[name] = 0; // 初始化索引
			lastPlayTime[name] = 0;
		}
		else {
			// 普通音效只加载一次
			std::wstring cmd = L"open \"" + path + L"\" alias " + name;
			mciSendString(cmd.c_str(), NULL, 0, NULL);
		}
	}

	loaded = true;
	currentBGM = L"";
}


void AudioManager::play(const std::wstring& name) {
	// 安全检查
	if (sounds.find(name) == sounds.end()) return;
	// 音效限流 
	DWORD now = GetTickCount();

	// 针对高频音效进行限流
	if (name == L"barrage" || name == L"break" || name == L"hit") {
		// 设定最小间隔为 40ms，约每秒 25 次
		const int MIN_INTERVAL = 40;

		if (now - lastPlayTime[name] < MIN_INTERVAL) {
			return; 
		}
		lastPlayTime[name] = now;
	}

	// 正常播放其他音效
	bool isPooled = std::find(pooledSounds.begin(), pooledSounds.end(), name) != pooledSounds.end();

	if (isPooled) {
		int idx = poolIndex[name];
		std::wstring alias = name + L"_" + std::to_wstring(idx);
		std::wstring cmd = L"play " + alias + L" from 0";
		mciSendString(cmd.c_str(), NULL, 0, NULL);
		poolIndex[name] = (idx + 1) % POOL_SIZE;
	}
	else {
		std::wstring cmd = L"play " + name + L" from 0";
		mciSendString(cmd.c_str(), NULL, 0, NULL);
	}
}

void AudioManager::playBGM(const std::wstring& name) {
	// 如果请求的 BGM 已经在播放，直接返回
	if (currentBGM == name) return;

	// 如果当前有 BGM 在播放，先停止它
	if (!currentBGM.empty()) {
		std::wstring cmdStop = L"stop " + currentBGM;
		mciSendString(cmdStop.c_str(), NULL, 0, NULL);
	}
	if (sounds.find(name) == sounds.end()) {
		currentBGM = L""; 
		return;
	}

	// BGM 
	std::wstring cmdPlay = L"play " + name + L" repeat";
	mciSendString(cmdPlay.c_str(), NULL, 0, NULL);
	currentBGM = name;
}

void AudioManager::stopBGM() {
	if (!currentBGM.empty()) {
		std::wstring cmd = L"stop " + currentBGM;
		mciSendString(cmd.c_str(), NULL, 0, NULL);
		currentBGM = L"";
	}
}