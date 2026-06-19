#ifndef VXGE_AUDIO_HPP
#define VXGE_AUDIO_HPP

#include <filesystem>

namespace fs = std::filesystem;

namespace VX {
    constexpr float AUDIO_PAN_LEFT = -1.0f;
    constexpr float AUDIO_PAN_CENTER = 0.0f;
    constexpr float AUDIO_PAN_RIGHT = 1.0f;

    class VXAudio {
    public:
        VXAudio() = default;
        explicit VXAudio(const fs::path& path);
        ~VXAudio();

        VXAudio(const VXAudio&) = delete;
        VXAudio& operator=(const VXAudio&) = delete;
        VXAudio(VXAudio&&) noexcept;
        VXAudio& operator=(VXAudio&&) noexcept;

        void Play();
        void Stop();
        void Pause();
        void Resume();

        void SetVolume(float volume); // 0.0 - 1.0
        float GetVolume() const;

        void SetLoop(bool loop);
        bool IsLooping() const;

        void SetPan(float pan); // -1.0 left, 0.0 center, 1.0 right
        float GetPan() const;

        bool IsPlaying() const;
        bool IsPaused() const;

        void Destroy();

    private:
        void* m_impl = nullptr;
    };
}

#endif //VXGE_AUDIO_HPP
