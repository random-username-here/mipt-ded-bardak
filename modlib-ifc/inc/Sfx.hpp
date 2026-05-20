#pragma once

#include "AssetManager.hpp"
#include "Event.hpp"
#include "Vec2.hpp"
#include "modlib_mod.hpp"

namespace sfx {

using SoundID = modlib::SoundID;

struct Cue {
    SoundID id{};
    modlib::Vec2f pos{};

    // negative for `SoundAsset` defaults
    float volume = -1.0f;
    float pitch = -1.0f;
    float delaySeconds = -1.0f;

    int priority = -69420;
};

class SoundManager : public Mod {
    Event<const Cue &> m_onPlay;

public:
    virtual void play(Cue cue) = 0;

    Event<const Cue &> &onPlay()
    {
        return m_onPlay;
    }

    ~SoundManager() override = default;
};

} // namespace sfx
