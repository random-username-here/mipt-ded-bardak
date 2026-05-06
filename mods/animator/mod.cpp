#include "Animator.hpp"
#include "Vec2.hpp"
#include <stdexcept>

class Animator : public anim::AnimationManager {

    std::string_view id() const override { return "isd.bardak.animator"; }
    std::string_view brief() const override { return "Animation manager module"; }
    ModVersion version() const override { return ModVersion(0, 0, 1); }

    std::vector<anim::Animation> m_anims;

    anim::Animation *newAnimation() override {
        m_anims.push_back(constructAnimation(m_anims.size()));
        return &m_anims.back();
    }
    
    void animationBuilt(anim::Animation *an) override {
        onRegister().emit(an);
    }

    void play(anim::AnimatedObjectID obj, anim::Vec2f cell, int lyr, anim::AnimationID id) override {
        if (id >= m_anims.size())
            throw std::runtime_error("Unknown animation");
        onPlay().emit(obj, cell, lyr, id);
    }
};

extern "C" Mod* modlib_create(ModManager*) { return new Animator(); }
