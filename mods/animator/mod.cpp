#include "Animator.hpp"
#include "Vec2.hpp"
#include <deque>
#include <stdexcept>

class Animator : public anim::AnimationManager {

    std::string_view id() const override { return "isd.bardak.animator"; }
    std::string_view brief() const override { return "Animation manager module"; }
    ModVersion version() const override { return ModVersion(0, 0, 1); }

    std::deque<anim::Animation> m_anims;
    anim::AnimatedObjectID m_nextObject = 0;
    anim::SpriteSlotID m_nextSpriteSlot = 0;

    anim::AnimatedObjectID newObject() override {
        return m_nextObject++;
    }

    anim::SpriteSlotID newSpriteSlot() override {
        return m_nextSpriteSlot++;
    }

    anim::Animation *newAnimation() override {
        m_anims.push_back(constructAnimation(m_anims.size()));
        return &m_anims.back();
    }

    const anim::Animation *animationFixUp(anim::AnimationID id) const override {
        if (id >= m_anims.size()) {
            return nullptr;
        }

        return &m_anims[id];
    }
    
    void animationBuilt(anim::Animation *an) override {
        onRegister().emit(an);
    }

    void play(anim::AnimatedObjectID obj, modlib::Vec2f cell, int lyr, anim::AnimationID id) override {
        if (id >= m_anims.size())
            throw std::runtime_error("Unknown animation");
        onPlay().emit(obj, cell, lyr, id);
    }
};

extern "C" Mod* modlib_create(ModManager*) { return new Animator(); }
