#pragma once

#include <string>
#include <miniaudio.h>
#include <functional>
namespace Razel
{
	class AudioManager
	{
	public:
		AudioManager();
		~AudioManager();

		AudioManager(const AudioManager&) = delete;
		AudioManager& operator=(const AudioManager&) = delete;

		bool StartRecording(const std::string& filePath);
		void StopRecording();
		void PlayAudioFile(const std::string& filePath, std::function<void()> onPlaybackFinished);
	
		void OnUpdate();
	private:
		// 静态回调函数，作为 miniaudio 的接口
		static void recording_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
		static void playback_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
		static void playback_stop_callback(ma_device* pDevice);

		// 成员函数，处理实际的回调逻辑
		void onRecordingData(const void* pInput, ma_uint32 frameCount);
		// 移除这两个不再使用的方法
		// void onPlaybackData(void* pOutput, ma_uint32 frameCount);
		// void onPlaybackFinishedInternal();
	private:
		// --- 录音相关成员 ---
		bool isRecording_ = false;
		ma_encoder encoder_;
		ma_device recordingDevice_;

		// --- 播放相关成员 ---
		bool isPlaying_ = false;
		bool isEncoderShouldBeUninit = false;
		ma_decoder decoder_;
		ma_device playbackDevice_;
		std::function<void()> onPlaybackFinishedCallback_;
	};

}