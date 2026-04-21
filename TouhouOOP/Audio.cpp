#include "Audio.h"
#include <iostream>
#include <mmsystem.h>

#if 0
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

	sounds[L"bgm_stage1"] = L"resource/bgm/stage1.mp3";
	sounds[L"bgm_sanae"] = L"resource/bgm/sanae.mp3";

	// 初始化音频资源
	for (auto const& [name, path] : sounds) {
		// 判断当前音效是否使用对象池
		bool isPooled = std::find(pooledSounds.begin(), pooledSounds.end(), name) != pooledSounds.end();

		if (isPooled) {
			// 为池化音效预先打开 POOL_SIZE 个实例
			for (int i = 0; i < POOL_SIZE; ++i) {
				std::wstring alias = name + L"_" + std::to_wstring(i);
				std::wstring cmd = L"open \"" + path + L"\" alias " + alias;
				mciSendString(cmd.c_str(), NULL, 0, NULL);
			}
			poolIndex[name] = 0; // 轮转播放索引
			lastPlayTime[name] = 0;
		}
		else {
			// 非池化音效只保留一个实例
			std::wstring cmd = L"open \"" + path + L"\" alias " + name;
			mciSendString(cmd.c_str(), NULL, 0, NULL);
		}
	}

	loaded = true;
	currentBGM = L"";
}


void AudioManager::play(const std::wstring& name) {
	// 未注册音效直接返回
	if (sounds.find(name) == sounds.end()) return;
	// 获取当前时间戳
	DWORD now = GetTickCount();

	// 高频音效加入节流，避免过度叠加
	if (name == L"barrage" || name == L"break" || name == L"hit") {
		// 最短间隔 40ms（约 25 次/秒）
		const int MIN_INTERVAL = 40;

		if (now - lastPlayTime[name] < MIN_INTERVAL) {
			return; 
		}
		lastPlayTime[name] = now;
	}

	// 根据是否池化选择播放策略
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
	// 相同 BGM 不重复播放，避免重置进度
	if (currentBGM == name) return;

	// 若已有 BGM，先停止当前曲目
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
#endif

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <filesystem>

AudioManager::AudioManager()
	: loaded(false),
	comInitialized(false),
	mfInitialized(false),
	xaudio(nullptr),
	masteringVoice(nullptr),
	bgmVoice(nullptr) {
}

AudioManager::~AudioManager() {
	std::lock_guard<std::mutex> lock(audioMutex);
	releaseAllSounds();
}

void AudioManager::destroyVoices(SoundData& data) {
	for (auto* voice : data.voices) {
		if (voice != nullptr) {
			voice->Stop(0);
			voice->DestroyVoice();
		}
	}
	data.voices.clear();
}

void AudioManager::releaseAllSounds() {
	stopBGM();

	for (auto& [_, data] : loadedSounds) {
		destroyVoices(data);
		if (data.waveFormat != nullptr) {
			CoTaskMemFree(data.waveFormat);
			data.waveFormat = nullptr;
		}
	}
	loadedSounds.clear();

	if (masteringVoice != nullptr) {
		masteringVoice->DestroyVoice();
		masteringVoice = nullptr;
	}
	if (xaudio != nullptr) {
		xaudio->Release();
		xaudio = nullptr;
	}
	if (mfInitialized) {
		MFShutdown();
		mfInitialized = false;
	}
	if (comInitialized) {
		CoUninitialize();
		comInitialized = false;
	}

	loaded = false;
	currentBGM.clear();
}

bool AudioManager::decodeToPCM(const std::wstring& path, SoundData& outData) {
	// 使用 Media Foundation 将压缩音频流解码为 PCM，并缓存到内存中供 XAudio2 播放
	IMFSourceReader* reader = nullptr;
	IMFMediaType* mediaType = nullptr;
	IMFMediaType* outputType = nullptr;

	HRESULT hr = MFCreateSourceReaderFromURL(path.c_str(), nullptr, &reader);
	if (FAILED(hr)) return false;

	hr = MFCreateMediaType(&mediaType);
	if (SUCCEEDED(hr)) hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	if (SUCCEEDED(hr)) hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	if (SUCCEEDED(hr)) hr = reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, mediaType);
	if (SUCCEEDED(hr)) hr = reader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &outputType);
	if (SUCCEEDED(hr)) {
		UINT32 formatSize = 0;
		hr = MFCreateWaveFormatExFromMFMediaType(outputType, &outData.waveFormat, &formatSize, MFWaveFormatExConvertFlag_Normal);
	}
	if (FAILED(hr) || outData.waveFormat == nullptr) {
		if (outputType) outputType->Release();
		if (mediaType) mediaType->Release();
		if (reader) reader->Release();
		return false;
	}

	outData.pcm.clear();
	// 逐帧读取样本并拼接到 PCM 缓冲，避免频繁磁盘 I/O 导致播放抖动
	while (true) {
		DWORD flags = 0;
		IMFSample* sample = nullptr;
		hr = reader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &sample);
		if (FAILED(hr)) break;
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
			if (sample) sample->Release();
			break;
		}
		if (sample != nullptr) {
			IMFMediaBuffer* buffer = nullptr;
			if (SUCCEEDED(sample->ConvertToContiguousBuffer(&buffer))) {
				BYTE* data = nullptr;
				DWORD maxLen = 0;
				DWORD curLen = 0;
				if (SUCCEEDED(buffer->Lock(&data, &maxLen, &curLen)) && curLen > 0) {
					size_t oldSize = outData.pcm.size();
					outData.pcm.resize(oldSize + curLen);
					memcpy(outData.pcm.data() + oldSize, data, curLen);
					buffer->Unlock();
				}
				buffer->Release();
			}
			sample->Release();
		}
	}

	if (outputType) outputType->Release();
	if (mediaType) mediaType->Release();
	if (reader) reader->Release();
	return !outData.pcm.empty();
}

void AudioManager::init() {
	std::lock_guard<std::mutex> lock(audioMutex);
	if (loaded) return;

	// 初始化 COM + Media Foundation + XAudio2，任一步失败则保持未加载状态
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (SUCCEEDED(hr)) comInitialized = true;
	else if (hr != RPC_E_CHANGED_MODE) return;

	hr = MFStartup(MF_VERSION);
	if (FAILED(hr)) return;
	mfInitialized = true;

	hr = XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr)) return;
	hr = xaudio->CreateMasteringVoice(&masteringVoice);
	if (FAILED(hr)) return;

	sounds[L"fire"] = L"resource/sound/fire.wav";
	sounds[L"hit"] = L"resource/sound/se_damage00.wav";
	sounds[L"break"] = L"resource/sound/se_enep00.wav";
	sounds[L"barrage"] = L"resource/sound/barrage_tan2.wav";
	sounds[L"spell"] = L"resource/sound/spell.wav";
	sounds[L"clear"] = L"resource/sound/se_enep00.wav";
	sounds[L"dead"] = L"resource/sound/se_pldead00.wav";
	sounds[L"pickup"] = L"resource/sound/se_item00.wav";
	sounds[L"breakBoss"] = L"resource/sound/break_boss.wav";

	// 扫描 BGM 目录并建立曲目映射
	std::vector<std::wstring> discoveredBGM;
	try {
		for (const auto& entry : std::filesystem::directory_iterator(L"resource/bgm")) {
			if (!entry.is_regular_file()) continue;
			std::wstring ext = entry.path().extension().wstring();
			std::transform(ext.begin(), ext.end(), ext.begin(), towlower);
			if (ext == L".mp3" || ext == L".wav" || ext == L".ogg") {
				discoveredBGM.push_back(entry.path().wstring());
			}
		}
	}
	catch (...) {}

	if (!discoveredBGM.empty()) {
		std::sort(discoveredBGM.begin(), discoveredBGM.end());

		// 默认兜底：按排序选择前两首
		size_t stage1Index = 0;
		size_t sanaeIndex = (discoveredBGM.size() > 1) ? 1 : 0;

		// 精确匹配文件名：stage1.mp3 与 sanae.mp3
		for (size_t i = 0; i < discoveredBGM.size(); ++i) {
			std::wstring filename = std::filesystem::path(discoveredBGM[i]).filename().wstring();
			std::transform(filename.begin(), filename.end(), filename.begin(), towlower);

			if (filename == L"stage1.mp3") {
				stage1Index = i;
			}
			else if (filename == L"sanae.mp3") {
				sanaeIndex = i;
			}
		}

		sounds[L"bgm_stage1"] = discoveredBGM[stage1Index];
		sounds[L"bgm_sanae"] = discoveredBGM[sanaeIndex];
	}

	for (const auto& [name, path] : sounds) {
		SoundData data;
		data.isBGM = (name.rfind(L"bgm_", 0) == 0);
		if (!decodeToPCM(path, data)) continue;

		if (!data.isBGM) {
			// SFX 预创建多个 SourceVoice 做对象池，支持高频短音效并发播放
			for (int i = 0; i < POOL_SIZE; ++i) {
				IXAudio2SourceVoice* voice = nullptr;
				if (SUCCEEDED(xaudio->CreateSourceVoice(&voice, data.waveFormat))) {
					data.voices.push_back(voice);
				}
			}
		}
		loadedSounds.emplace(name, std::move(data));
	}

	loaded = true;
	currentBGM.clear();
}

void AudioManager::play(const std::wstring& name) {
	std::lock_guard<std::mutex> lock(audioMutex);
	if (!loaded) return;

	auto it = loadedSounds.find(name);
	if (it == loadedSounds.end() || it->second.isBGM || it->second.voices.empty()) return;

	SoundData& data = it->second;
	// 轮询选择下一个可用 voice，减少重复触发时的覆盖和截断
	IXAudio2SourceVoice* voice = data.voices[data.nextVoice];
	data.nextVoice = (data.nextVoice + 1) % data.voices.size();

	XAUDIO2_BUFFER buffer = {};
	buffer.AudioBytes = static_cast<UINT32>(data.pcm.size());
	buffer.pAudioData = data.pcm.data();
	buffer.Flags = XAUDIO2_END_OF_STREAM;

	voice->Stop(0);
	voice->FlushSourceBuffers();
	if (SUCCEEDED(voice->SubmitSourceBuffer(&buffer))) {
		voice->Start(0);
	}
}

void AudioManager::playBGM(const std::wstring& name) {
	std::lock_guard<std::mutex> lock(audioMutex);
	if (!loaded) return;
	if (currentBGM == name) return;

	stopBGM();
	// BGM 切换入口由 Game::handleBGM 控制，此处仅负责实际播放

	auto it = loadedSounds.find(name);
	if (it == loadedSounds.end() || !it->second.isBGM) return;

	SoundData& data = it->second;
	if (FAILED(xaudio->CreateSourceVoice(&bgmVoice, data.waveFormat))) return;

	XAUDIO2_BUFFER buffer = {};
	buffer.AudioBytes = static_cast<UINT32>(data.pcm.size());
	buffer.pAudioData = data.pcm.data();
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	// 设置为无限循环，直到外部显式调用 stopBGM

	if (SUCCEEDED(bgmVoice->SubmitSourceBuffer(&buffer)) && SUCCEEDED(bgmVoice->Start(0))) {
		currentBGM = name;
	}
}

void AudioManager::stopBGM() {
	if (bgmVoice != nullptr) {
		// 先停止并清空缓冲，再销毁 voice，确保下次切歌从头开始
		bgmVoice->Stop(0);
		bgmVoice->FlushSourceBuffers();
		bgmVoice->DestroyVoice();
		bgmVoice = nullptr;
	}
	currentBGM.clear();
}