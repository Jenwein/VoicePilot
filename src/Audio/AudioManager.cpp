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
		// ma_engine_play_sound 是一个高级API，非常简单易用
		// 它会在后台自动处理解码和播放，播放完毕后自动释放资源
		ma_result result = ma_engine_play_sound(&m_Engine, filePath.c_str(), NULL);
		if (result != MA_SUCCESS) {
			std::cerr << "Failed to play audio file: " << filePath << std::endl;
		}
	}

	bool AudioManager::IsRecording() const
	{
		return m_IsRecording;
	}
}