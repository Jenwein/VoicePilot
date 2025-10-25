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

		bool StartRecording(const std::string& filePath);
		void StopRecording();
		void PlayAudioFile(const std::string& filePath);
		void PlayAudioFile(const std::string& filePath, std::function<void()> onPlaybackFinished);
		bool IsRecording() const;

	private:
		static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
		static void sound_end_callback(void* pUserData, ma_sound* pSound);
	private:
		ma_context m_Context;
		ma_device m_Device;
		ma_encoder m_Encoder;
		ma_engine m_Engine;

		bool m_IsRecording = false;
		std::function<void()> m_OnPlaybackFinished;
	};

}