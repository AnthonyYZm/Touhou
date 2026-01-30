#include "Audio.h"
#include <windows.h>
#include <iostream>
#include <mmsystem.h>

AudioManager::AudioManager() { loaded = false; }

AudioManager::~AudioManager() {
	// 关闭所有打开的音频设备
	mciSendString(L"close all", NULL, 0, NULL);
}

void AudioManager::init() {
	if (loaded) return;

	// 在这里注册你的音效文件路径和别名
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
	sounds[L"bgm_boss"] = L"resource/sound/信仰は儚き人間の為に (信仰是为了虚幻之人) - 上海爱莉丝幻乐团.mp3";

	// 预加载 (打开设备)
	for (auto const& [alias, path] : sounds) {
		std::wstring cmd = L"open \"" + path + L"\" alias " + alias;
		mciSendString(cmd.c_str(), NULL, 0, NULL);
	}
	loaded = true;
	currentBGM = L"";
}

void AudioManager::playFast(const std::wstring& path) {
	// SND_ASYNC: 异步播放（不阻塞主线程）
	// SND_FILENAME: 从文件播放
	// SND_NODEFAULT: 找不到文件时不播放默认系统音
	// 注意：PlaySound 同一时间只能播一个声音，后来的会覆盖先来的。
	// 这对于密集音效（如机枪）可能听起来会吞音，但绝不会卡顿。
	PlaySound(path.c_str(), NULL, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
}

void AudioManager::play(const std::wstring& name) {
	// 针对容易卡顿的高频音效，改用 PlaySound
	if ( name == L"break" || name == L"clear") {
		if (sounds.find(name) != sounds.end()) {
			playFast(sounds[name]);
		}
		return;
	}

	// 其他长音效（如 spell, bgm）继续用 MCI
	if (sounds.find(name) == sounds.end()) return;
	std::wstring cmd = L"play " + name + L" from 0";
	mciSendString(cmd.c_str(), NULL, 0, NULL);
}

void AudioManager::playBGM(const std::wstring& name) {
	// 1. 如果请求的 BGM 已经在播放，直接返回，避免重头开始
	if (currentBGM == name) return;

	// 2. 如果当前有 BGM 在播放，先停止它
	if (!currentBGM.empty()) {
		std::wstring cmdStop = L"stop " + currentBGM;
		mciSendString(cmdStop.c_str(), NULL, 0, NULL);
	}

	// 3. 检查新 BGM 是否存在
	if (sounds.find(name) == sounds.end()) {
		currentBGM = L""; // 找不到就置空
		return;
	}

	// 4. 播放新 BGM (加上 repeat 关键字实现循环)
	std::wstring cmdPlay = L"play " + name + L" repeat";
	mciSendString(cmdPlay.c_str(), NULL, 0, NULL);

	// 5. 更新记录
	currentBGM = name;
}

void AudioManager::stopBGM() {
	if (!currentBGM.empty()) {
		std::wstring cmd = L"stop " + currentBGM;
		mciSendString(cmd.c_str(), NULL, 0, NULL);
		currentBGM = L"";
	}
}