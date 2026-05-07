#pragma once

#include "Animator.hpp"
#include "AssetManager.hpp"
#include "person_controller.hpp"

#include <cmath>
#include <string>
#include <string_view>
#include <utility>

class PersonAnimator {
    static constexpr float kTilePixels = 40.0f;
    static constexpr float kMoveSeconds = 0.18f;
    static constexpr float kAttackFrameSeconds = 0.05f;

    static constexpr anim::SpriteID kBodySprite = 0;
    static constexpr anim::SpriteID kSlashSprite = 1;

    static constexpr std::string_view kIdleSprite = "p.idle";
    static constexpr std::string_view kRunDownSprite = "p.run.d";
    static constexpr std::string_view kRunUpSprite = "p.run.u";
    static constexpr std::string_view kRunLeftSprite = "p.run.l";
    static constexpr std::string_view kRunRightSprite = "p.run.r";

    static constexpr std::string_view kSlash1Sprite = "slash.1";
    static constexpr std::string_view kSlash2Sprite = "slash.2";
    static constexpr std::string_view kSlash3Sprite = "slash.3";
    static constexpr std::string_view kSlash4Sprite = "slash.4";

    PersonCtl *m_ctl = nullptr;
    anim::AnimationManager *m_anim = nullptr;
    modlib::AssetManager *m_assets = nullptr;

public:
    PersonAnimator(PersonCtl *ctl, anim::AnimationManager *anim, modlib::AssetManager *assets)
        : m_ctl(ctl)
        , m_anim(anim)
        , m_assets(assets)
    {
        registerAssets();

        m_ctl->person()->EvAttack.subscribe(
            [this](Person::Damage targetID) {
                animateAttack(targetID);
            }
        );

        m_ctl->person()->EvEntityMoved.subscribe(
            [this](modlib::Vec2i delta) {
                animateMove(delta);
            }
        );

        animateIdle();
    }

private:
    void animateIdle() {
        if (!m_anim || !m_ctl || !m_ctl->person()) return;

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetImageStep>(kBodySprite, kIdleSprite);
        animation->addStep<anim::PosStep>(0.0f, 0.0f, kBodySprite, modlib::Vec2f(0.0f, 0.0f));
        animation->finishBuild();

        m_anim->play(objectID(), currentPixelPosition(), 0, animation->id());
    }

    void animateMove(modlib::Vec2i delta) {
        if (!m_anim || !m_ctl || !m_ctl->person()) return;

        const modlib::Vec2f oldPosition = pixelPosition(m_ctl->person()->getPosition() - delta);
        const modlib::Vec2f to(
            static_cast<float>(delta.x) * kTilePixels,
            static_cast<float>(delta.y) * kTilePixels
        );

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetImageStep>(kBodySprite, runSprite(delta));
        animation->addStep<anim::PosStep>(kMoveSeconds, kMoveSeconds, kBodySprite, to);
        animation->addStep<anim::SetImageStep>(kBodySprite, kIdleSprite);
        animation->finishBuild();

        m_anim->play(objectID(), oldPosition, 0, animation->id());
    }

    void animateAttack(Person::Damage targetID) {
        if (!m_anim || !m_ctl || !m_ctl->person()) return;

        const modlib::Vec2f slashOffset = attackOffset(targetID);

        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetImageStep>(kBodySprite, kIdleSprite);
        animation->addStep<anim::PosStep>(0.0f, 0.0f, kSlashSprite, slashOffset);

        animation->addStep<anim::SetImageStep>(kSlashSprite, kSlash1Sprite);
        animation->addStep<anim::Step>(kAttackFrameSeconds, kAttackFrameSeconds);
        animation->addStep<anim::SetImageStep>(kSlashSprite, kSlash2Sprite);
        animation->addStep<anim::Step>(kAttackFrameSeconds, kAttackFrameSeconds);
        animation->addStep<anim::SetImageStep>(kSlashSprite, kSlash3Sprite);
        animation->addStep<anim::Step>(kAttackFrameSeconds, kAttackFrameSeconds);
        animation->addStep<anim::SetImageStep>(kSlashSprite, kSlash4Sprite);
        animation->addStep<anim::Step>(kAttackFrameSeconds, kAttackFrameSeconds);
        animation->addStep<anim::DelSpriteStep>(kSlashSprite);
        animation->finishBuild();

        m_anim->play(objectID(), currentPixelPosition(), 1, animation->id());
    }

    void registerAssets() {
        if (!m_assets) return;

        registerSprite(kIdleSprite, "assets/rogue_down.png", 16, 16, kTilePixels, kTilePixels);
        registerSprite(kRunDownSprite, "assets/rogue_run_down.png", 16, 16, kTilePixels, kTilePixels);
        registerSprite(kRunUpSprite, "assets/rogue_run_up.png", 16, 16, kTilePixels, kTilePixels);
        registerSprite(kRunLeftSprite, "assets/rogue_run_left.png", 16, 16, kTilePixels, kTilePixels);
        registerSprite(kRunRightSprite, "assets/rogue_run_right.png", 16, 16, kTilePixels, kTilePixels);

        registerSprite(kSlash1Sprite, "assets/slash_01.png", 32, 32, 48, 48);
        registerSprite(kSlash2Sprite, "assets/slash_02.png", 32, 32, 48, 48);
        registerSprite(kSlash3Sprite, "assets/slash_03.png", 32, 32, 48, 48);
        registerSprite(kSlash4Sprite, "assets/slash_04.png", 32, 32, 48, 48);
    }

    void registerSprite(
        std::string_view id,
        std::string file,
        float sourceW,
        float sourceH,
        float drawW,
        float drawH
    ) {
        const modlib::SpriteID spriteId(id);
        if (m_assets->sprite(spriteId)) return;

        modlib::SpriteAsset sprite;
        sprite.id = spriteId;
        sprite.file = std::move(file);
        sprite.clip = modlib::Rectf{0, 0, sourceW, sourceH};
        sprite.size = modlib::Vec2f{drawW, drawH};

        m_assets->registerSprite(sprite);
    }

    anim::AnimatedObjectID objectID() const {
        return static_cast<anim::AnimatedObjectID>(m_ctl->person()->getID());
    }

    modlib::Vec2f currentPixelPosition() const {
        return pixelPosition(m_ctl->person()->getPosition());
    }

    static modlib::Vec2f pixelPosition(modlib::Vec2i cell) {
        return modlib::Vec2f(
            static_cast<float>(cell.x) * kTilePixels,
            static_cast<float>(cell.y) * kTilePixels
        );
    }

    std::string_view runSprite(modlib::Vec2i delta) const {
        if (std::abs(delta.x) > std::abs(delta.y)) {
            return delta.x < 0 ? kRunLeftSprite : kRunRightSprite;
        }

        return delta.y < 0 ? kRunUpSprite : kRunDownSprite;
    }

    modlib::Vec2f attackOffset(Person::Damage targetID) const {
        modlib::Vec2i delta(1, 0);

        if (auto *target = m_ctl->map()->getEntity(targetID)) {
            const modlib::Vec2i raw = target->getPosition() - m_ctl->person()->getPosition();
            delta.x = raw.x == 0 ? 0 : (raw.x < 0 ? -1 : 1);
            delta.y = raw.y == 0 ? 0 : (raw.y < 0 ? -1 : 1);

            if (delta.x == 0 && delta.y == 0) {
                delta.x = 1;
            }
        }

        return modlib::Vec2f(
            static_cast<float>(delta.x) * kTilePixels,
            static_cast<float>(delta.y) * kTilePixels
        );
    }
};
