#include "AudioManager.h"

#include <iostream>
namespace Razel {

	AudioManager::AudioManager()
	{

	}

	bool AudioManager::StartRecording(const std::string& filePath)
	{
		if (isRecording_) {
			std::cerr << "错误: 已经在录音中。" << std::endl;
			return false;
		}

		// 1. 配置编码器
		ma_encoder_config encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 2, 44100);
		if (ma_encoder_init_file(filePath.c_str(), &encoderConfig, &encoder_) != MA_SUCCESS) {
			std::cerr << "错误: 初始化编码器失败。" << std::endl;
			return false;
		}

		// 2. 配置捕获设备
		ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
		deviceConfig.capture.format = encoder_.config.format;
		deviceConfig.capture.channels = encoder_.config.channels;
		deviceConfig.sampleRate = encoder_.config.sampleRate;
		deviceConfig.dataCallback = recording_data_callback;
		deviceConfig.pUserData = this; // 传递 this 指针

		if (ma_device_init(NULL, &deviceConfig, &recordingDevice_) != MA_SUCCESS) {
			std::cerr << "错误: 初始化捕获设备失败。" << std::endl;
			ma_encoder_uninit(&encoder_);
			return false;
		}

		// 3. 启动设备
		if (ma_device_start(&recordingDevice_) != MA_SUCCESS) {
			std::cerr << "错误: 启动录音设备失败。" << std::endl;
			ma_device_uninit(&recordingDevice_);
			ma_encoder_uninit(&encoder_);
			return false;
		}

		isRecording_ = true;
		std::cerr << "开始录音..." << std::endl;
		return true;
	}

	void AudioManager::StopRecording()
	{
		if (!isRecording_) {
			return;
		}
		isRecording_ = false;
		ma_device_uninit(&recordingDevice_);
		ma_encoder_uninit(&encoder_);
		std::cerr << "录音已停止。" << std::endl;
	}

	void AudioManager::recording_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
		AudioManager* pManager = (AudioManager*)pDevice->pUserData;
		pManager->onRecordingData(pInput, frameCount);
		(void)pOutput;
	}

	void AudioManager::onRecordingData(const void* pInput, ma_uint32 frameCount) {
		ma_encoder_write_pcm_frames(&encoder_, pInput, frameCount, NULL);
	}

	// --- 播放实现 ---

	void AudioManager::PlayAudioFile(const std::string& filePath, std::function<void()> onPlaybackFinished) {
		if (isPlaying_) {
			std::cerr << "警告: 正在播放另一个文件，请等待其结束。" << std::endl;
			if (onPlaybackFinished) onPlaybackFinished();
			return;
		}

		// 1. 初始化解码器
		if (ma_decoder_init_file(filePath.c_str(), NULL, &decoder_) != MA_SUCCESS) {
			std::cerr << "错误: 无法加载音频文件 '" << filePath << "'" << std::endl;
			if (onPlaybackFinished) onPlaybackFinished();
			return;
		}

		onPlaybackFinishedCallback_ = onPlaybackFinished;

		// 2. 配置播放设备
		ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
		deviceConfig.playback.format = decoder_.outputFormat;
		deviceConfig.playback.channels = decoder_.outputChannels;
		deviceConfig.sampleRate = decoder_.outputSampleRate;
		deviceConfig.dataCallback = playback_data_callback;
		deviceConfig.stopCallback = playback_stop_callback;
		deviceConfig.pUserData = this;

		if (ma_device_init(NULL, &deviceConfig, &playbackDevice_) != MA_SUCCESS) {
			std::cerr << "错误: 初始化播放设备失败。" << std::endl;
			ma_decoder_uninit(&decoder_);
			if (onPlaybackFinishedCallback_) onPlaybackFinishedCallback_();
			return;
		}

		// 3. 启动设备
		if (ma_device_start(&playbackDevice_) != MA_SUCCESS) {
			std::cerr << "错误: 启动播放设备失败。" << std::endl;
			ma_device_uninit(&playbackDevice_);
			ma_decoder_uninit(&decoder_);
			if (onPlaybackFinishedCallback_) onPlaybackFinishedCallback_();
			return;
		}

		isPlaying_ = true;
		std::cerr << "开始播放 '" << filePath << "'..." << std::endl;
	}

	void AudioManager::OnUpdate()
	{
		if (isEncoderShouldBeUninit)
		{
			ma_device_uninit(&playbackDevice_);
			ma_decoder_uninit(&decoder_);
			isEncoderShouldBeUninit = false;
		}
	}

	void AudioManager::playback_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
		AudioManager* pManager = (AudioManager*)pDevice->pUserData;
		if (pManager == NULL || !pManager->isPlaying_) {
			return;
		}
		
		ma_uint64 framesRead;
		ma_decoder_read_pcm_frames(&pManager->decoder_, pOutput, frameCount, &framesRead);

		// 如果读取的帧数少于请求的帧数，说明音频播放完毕
		if (framesRead < frameCount) {			
			// 设置标志表示播放结束，但不在这里停止设备
			pManager->isPlaying_ = false;
			
			// 直接在这里调用完成回调
			if (pManager->onPlaybackFinishedCallback_) {
				pManager->onPlaybackFinishedCallback_();
				pManager->isEncoderShouldBeUninit = true;
				return;
			}
			return;
		}

		(void)pInput;
	}

	void AudioManager::playback_stop_callback(ma_device* pDevice) {
		AudioManager* pManager = (AudioManager*)pDevice->pUserData;
		if (pManager) {
			pManager->isPlaying_ = false;
			std::cerr << "播放设备已停止。" << std::endl;
		}
	}

	AudioManager::~AudioManager()
	{
		StopRecording();
		//if (isPlaying_) {
		//	ma_device_uninit(&playbackDevice_);
		//	ma_decoder_uninit(&decoder_);
		//}
	}

}