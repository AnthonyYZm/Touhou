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

	sounds[L"bgm_stage1"] = L"resource/bgm/【東方風神録】～ 神々が恋した幻想郷 ～　.mp3";
	sounds[L"bgm_boss"] = L"resource/bgm/【東方風神録】～ 信仰は儚き人間の為に ～　.mp3";

	// 预加载 
	for (auto const& [alias, path] : sounds) {
		std::wstring cmd = L"open \"" + path + L"\" alias " + alias;
		mciSendString(cmd.c_str(), NULL, 0, NULL);
	}
	loaded = true;
	currentBGM = L"";
}

void AudioManager::playFast(const std::wstring& path) {
	PlaySound(path.c_str(), NULL, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
}

void AudioManager::play(const std::wstring& name) {
	// PlaySound
	if ( name == L"break" || name == L"clear") {
		if (sounds.find(name) != sounds.end()) {
			playFast(sounds[name]);
		}
		return;
	}     

	// MCI
	if (sounds.find(name) == sounds.end()) return;
	std::wstring cmd = L"play " + name + L" from 0";
	mciSendString(cmd.c_str(), NULL, 0, NULL);
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