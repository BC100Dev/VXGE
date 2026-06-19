#include <VXGE/VXAudio.hpp>
#include <VXGE/VXError.hpp>

#include <SDL3/SDL.h>
#include <utility>
#include <cstring>
#include <vector>

namespace VX {
    struct VXAudioImpl {
        SDL_AudioStream* stream = nullptr;
        SDL_AudioDeviceID device = 0;
        SDL_AudioSpec spec = {};
        uint8_t* buffer = nullptr;
        uint32_t length = 0;
        uint32_t position = 0;
        float volume = 1.0f;
        float pan = 0.0f;
        bool loop = false;
        bool playing = false;
        bool paused = false;
    };

    static void SDLCALL audioCallback(void* userdata, SDL_AudioStream* stream,
                                      int additional_amount, int) {
        auto* impl = static_cast<VXAudioImpl*>(userdata);
        if (!impl->playing || impl->paused) return;

        int remaining = additional_amount;
        while (remaining > 0) {
            int available = static_cast<int>(impl->length) - static_cast<int>(impl->position);
            if (available <= 0) {
                if (impl->loop) {
                    impl->position = 0;
                    available = static_cast<int>(impl->length);
                } else {
                    impl->playing = false;
                    return;
                }
            }

            int toWrite = remaining < available ? remaining : available;
            auto samples = reinterpret_cast<int16_t*>(impl->buffer + impl->position);
            int sampleCount = toWrite / sizeof(int16_t);
            int channels = impl->spec.channels;

            std::vector<int16_t> mixed(sampleCount);
            for (int i = 0; i < sampleCount; i++) {
                float sample = samples[i] / 32768.0f;
                float vol = impl->volume;
                if (channels == 2) {
                    if (i % 2 == 0) vol *= (1.0f - impl->pan) * 0.5f + 0.5f;
                    else vol *= (1.0f + impl->pan) * 0.5f + 0.5f;
                }
                sample *= vol;
                if (sample > 1.0f) sample = 1.0f;
                if (sample < -1.0f) sample = -1.0f;
                mixed[i] = static_cast<int16_t>(sample * 32767.0f);
            }

            SDL_PutAudioStreamData(stream, mixed.data(), toWrite);
            impl->position += toWrite;
            remaining -= toWrite;
        }
    }

    VXAudio::VXAudio(const fs::path& path) {
        auto* impl = new VXAudioImpl();

        if (!SDL_LoadWAV(path.string().c_str(), &impl->spec,
                         &impl->buffer, &impl->length)) {
            SetLastError(VXError("VXAudio: Failed to load: " + path.string()));
            delete impl;
            return;
        }

        impl->device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &impl->spec);
        if (!impl->device) {
            SetLastError(VXError("VXAudio: Failed to open audio device"));
            SDL_free(impl->buffer);
            delete impl;
            return;
        }

        impl->stream = SDL_CreateAudioStream(&impl->spec, &impl->spec);
        if (!impl->stream) {
            SetLastError(VXError("VXAudio: Failed to create stream"));
            SDL_CloseAudioDevice(impl->device);
            SDL_free(impl->buffer);
            delete impl;
            return;
        }

        SDL_SetAudioStreamGetCallback(impl->stream, audioCallback, impl);
        SDL_BindAudioStream(impl->device, impl->stream);
        m_impl = impl;
    }

    VXAudio::~VXAudio() { Destroy(); }

    VXAudio::VXAudio(VXAudio&& other) noexcept : m_impl(other.m_impl) {
        other.m_impl = nullptr;
    }

    VXAudio& VXAudio::operator=(VXAudio&& other) noexcept {
        if (this != &other) {
            Destroy();
            m_impl = other.m_impl;
            other.m_impl = nullptr;
        }
        return *this;
    }

    void VXAudio::Play() {
        if (!m_impl) return;
        auto* impl = static_cast<VXAudioImpl*>(m_impl);
        impl->position = 0;
        impl->playing = true;
        impl->paused = false;
    }

    void VXAudio::Stop() {
        if (!m_impl) return;
        auto* impl = static_cast<VXAudioImpl*>(m_impl);
        impl->playing = false;
        impl->paused = false;
        impl->position = 0;
    }

    void VXAudio::Pause() {
        if (!m_impl) return;
        static_cast<VXAudioImpl*>(m_impl)->paused = true;
    }

    void VXAudio::Resume() {
        if (!m_impl) return;
        static_cast<VXAudioImpl*>(m_impl)->paused = false;
    }

    void VXAudio::SetVolume(float volume) {
        if (!m_impl) return;
        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;
        static_cast<VXAudioImpl*>(m_impl)->volume = volume;
    }

    void VXAudio::SetLoop(bool loop) {
        if (!m_impl) return;
        static_cast<VXAudioImpl*>(m_impl)->loop = loop;
    }

    void VXAudio::SetPan(float pan) {
        if (!m_impl) return;
        if (pan < -1.0f) pan = -1.0f;
        if (pan > 1.0f) pan = 1.0f;
        static_cast<VXAudioImpl*>(m_impl)->pan = pan;
    }

    float VXAudio::GetVolume() const {
        if (!m_impl) return 0.0f;
        return static_cast<VXAudioImpl*>(m_impl)->volume;
    }

    bool VXAudio::IsLooping() const {
        if (!m_impl) return false;
        return static_cast<VXAudioImpl*>(m_impl)->loop;
    }

    float VXAudio::GetPan() const {
        if (!m_impl) return 0.0f;
        return static_cast<VXAudioImpl*>(m_impl)->pan;
    }

    bool VXAudio::IsPlaying() const {
        if (!m_impl) return false;
        auto* impl = static_cast<VXAudioImpl*>(m_impl);
        return impl->playing && !impl->paused;
    }

    bool VXAudio::IsPaused() const {
        if (!m_impl) return false;
        return static_cast<VXAudioImpl*>(m_impl)->paused;
    }

    void VXAudio::Destroy() {
        if (!m_impl) return;
        auto* impl = static_cast<VXAudioImpl*>(m_impl);

        if (impl->stream) {
            SDL_DestroyAudioStream(impl->stream);
            impl->stream = nullptr;
        }

        if (impl->device) {
            SDL_CloseAudioDevice(impl->device);
            impl->device = 0;
        }

        if (impl->buffer) {
            SDL_free(impl->buffer);
            impl->buffer = nullptr;
        }
        delete impl;
        m_impl = nullptr;
    }
}
