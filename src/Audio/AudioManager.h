#pragma once

#include <string>
#include <miniaudio.h>

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
        bool IsRecording() const;

    private:
		static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    private:
        ma_context m_Context;
        ma_device m_Device;
        ma_encoder m_Encoder;
        ma_engine m_Engine;

        bool m_IsRecording = false;
    };

}