#include "AudioManager.h"

#include <iostream>
namespace Razel {

	// 回调函数，当录音设备捕获到数据时，miniaudio会调用此函数
	// pDevice->pUserData 指向我们的 AudioManager 实例
	void AudioManager::data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
	{
		AudioManager* pAudioManager = (AudioManager*)pDevice->pUserData;
		if (pAudioManager == nullptr) {
			return;
		}

		ma_encoder_write_pcm_frames(&pAudioManager->m_Encoder, pInput, frameCount, NULL);

		(void)pOutput; // 未使用输出参数
	}

	void AudioManager::sound_end_callback(void* pUserData, ma_sound* pSound)
	{
		// pUserData 指向我们的 AudioManager 实例
		AudioManager* pAudioManager = (AudioManager*)pUserData;
		if (pAudioManager && pAudioManager->m_OnPlaybackFinished)
		{
			std::cout << "[AudioManager] Playback finished. Invoking callback." << std::endl;
			pAudioManager->m_OnPlaybackFinished(); // 调用C++回调
			pAudioManager->m_OnPlaybackFinished = nullptr; // 清理回调，防止重复调用
		}

		// 释放 ma_sound 资源
		ma_sound_uninit(pSound);
		delete pSound; // 释放我们自己分配的内存
	}

	AudioManager::AudioManager()
	{
		// 初始化音频引擎，用于播放声音
		ma_result result = ma_engine_init(NULL, &m_Engine);
		if (result != MA_SUCCESS) {
			std::cerr << "Failed to initialize audio engine." << std::endl;
			// 在实际应用中，这里应该有更健壮的错误处理
		}
	}

	AudioManager::~AudioManager()
	{
		// 确保在对象销毁时停止录音并释放资源
		if (m_IsRecording) {
			StopRecording();
		}
		// 卸载音频引擎
		ma_engine_uninit(&m_Engine);
	}

	bool AudioManager::StartRecording(const std::string& filePath)
	{
		if (m_IsRecording) {
			std::cerr << "Already recording. Please stop the current recording first." << std::endl;
			return false;
		}

		// 1. 配置编码器 (Encoder) - 用于将PCM音频数据写入WAV文件
		ma_encoder_config encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_s16, 1, 22050);
		if (ma_encoder_init_file(filePath.c_str(), &encoderConfig, &m_Encoder) != MA_SUCCESS) {
			std::cerr << "Failed to initialize encoder for file: " << filePath << std::endl;
			return false;
		}

		// 2. 配置录音设备 (Device)
		ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
		deviceConfig.capture.format = m_Encoder.config.format; // 捕获格式与编码器一致
		deviceConfig.capture.channels = m_Encoder.config.channels; // 捕获通道数与编码器一致
		deviceConfig.sampleRate = m_Encoder.config.sampleRate; // 捕获采样率与编码器一致
		deviceConfig.dataCallback = data_callback; // 设置数据回调函数
		deviceConfig.pUserData = this; // 将当前实例指针传递给回调函数

		if (ma_device_init(NULL, &deviceConfig, &m_Device) != MA_SUCCESS) {
			std::cerr << "Failed to initialize capture device." << std::endl;
			ma_encoder_uninit(&m_Encoder); // 初始化失败时，清理已初始化的编码器
			return false;
		}

		// 3. 启动设备
		if (ma_device_start(&m_Device) != MA_SUCCESS) {
			std::cerr << "Failed to start capture device." << std::endl;
			ma_device_uninit(&m_Device);
			ma_encoder_uninit(&m_Encoder);
			return false;
		}

		m_IsRecording = true;
		std::cout << "Started recording to " << filePath << std::endl;
		return true;
	}

	void AudioManager::StopRecording()
	{
		if (!m_IsRecording) {
			return;
		}

		// 停止并卸载设备和编码器
		ma_device_uninit(&m_Device);
		ma_encoder_uninit(&m_Encoder);

		m_IsRecording = false;
		std::cout << "Stopped recording." << std::endl;
	}

	void AudioManager::PlayAudioFile(const std::string& filePath)
	{
		// 异步播放
		ma_result result = ma_engine_play_sound(&m_Engine, filePath.c_str(), NULL);
		if (result != MA_SUCCESS) {
			std::cerr << "Failed to play audio file: " << filePath << std::endl;
		}
	}

	void AudioManager::PlayAudioFile(const std::string& filePath, std::function<void()> onPlaybackFinished)
	{
		m_OnPlaybackFinished = onPlaybackFinished;

		// 我们需要在堆上为 ma_sound 分配内存，因为它将在回调中被异步释放
		ma_sound* pSound = new ma_sound;

		ma_result result = ma_sound_init_from_file(&m_Engine, filePath.c_str(), 0, NULL, NULL, pSound);
		if (result != MA_SUCCESS) {
			std::cerr << "Failed to load audio file: " << filePath << std::endl;
			m_OnPlaybackFinished = nullptr; // 加载失败，清理回调
			delete pSound;
			return;
		}

		// 设置播放结束的回调
		ma_sound_set_end_callback(pSound, sound_end_callback, this);

		// 开始播放
		if (ma_sound_start(pSound) != MA_SUCCESS)
		{
			std::cerr << "Failed to start audio playback for file: " << filePath << std::endl;
			ma_sound_uninit(pSound); // 播放失败，立即清理
			delete pSound;
			m_OnPlaybackFinished = nullptr;
		}
	}

	bool AudioManager::IsRecording() const
	{
		return m_IsRecording;
	}
}