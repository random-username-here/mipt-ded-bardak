#pragma once
#include "Animator.hpp"
#include "BmServerModule.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "Timer.hpp"
#include "Vec2.hpp"
#include "modlib_manager.hpp"

#include <cassert>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>

namespace bombs {

constexpr float kTilePixels = 16.0f;

namespace bomb_sprite {

struct Config {
    static constexpr modlib::Vec2f kSize = {kTilePixels, kTilePixels};
    static constexpr modlib::Vec2f kBombAnimSize = {16, 20};
};

constexpr int kObjectLayer = 1;
constexpr int kZ = 0;
static constexpr float kFlashSeconds = 0.08f;

} // namespace bomb_sprite

namespace bomb_assets {

static const modlib::SpriteAsset Bomb = {
    .id = modlib::SpriteID("d.bomb"),
    .file = ASSETS_DIR "/units/bomber/all.png",
    .clip = {56, 268, 16, 20},
    .size = {kTilePixels * 16.f / 20.f, kTilePixels},
};

#define FIRE_FRAME(idx)                                                                     \
{                                                                                           \
    .id = modlib::SpriteID("d.fire" #idx),                                                  \
    .file = ASSETS_DIR "/units/bomb/explode.png",                                           \
    .clip =                                                                                 \
    {                                                                                       \
        bomb_sprite::Config::kBombAnimSize.x * (idx),                                       \
        0,                                                                                  \
        bomb_sprite::Config::kBombAnimSize.x,                                               \
        bomb_sprite::Config::kBombAnimSize.y                                                \
    },                                                                                      \
    .size = bomb_sprite::Config::kSize,                                                     \
},
static const modlib::SpriteAsset Fires[8] = {
FIRE_FRAME(0)
FIRE_FRAME(1)
FIRE_FRAME(2)
FIRE_FRAME(3)
FIRE_FRAME(4)
FIRE_FRAME(5)
FIRE_FRAME(6)
FIRE_FRAME(7)
};
#undef FIRE_FRAME

} // namespace bomb_assets

modlib::Vec2f pixelPosition(modlib::Vec2i cell)
{
    return modlib::Vec2f(cell.x * kTilePixels, cell.y * kTilePixels);
}

static const int64_t kBombDamage = 50;
static constexpr size_t kBombRange = 5;

class Bomb final : public virtual modlib::Entity, public virtual EC::Stats::Attack {
public:
    Bomb(modlib::Tile *tile)
        : modlib::Entity("bomb", tile), EC::Stats::Attack(kBombDamage)
    {}
};

class BombAnimator {
    Bomb                   *m_bomb   = nullptr;
    anim::AnimationManager *m_anim   = nullptr;
    modlib::AssetManager   *m_assets = nullptr;
    anim::SpriteSlotID      m_slot   = 0;
    anim::AnimatedObjectID m_object;

    struct {
        anim::AnimationID idle  = anim::NO_ANIMATION;
        anim::AnimationID fire  = anim::NO_ANIMATION;
    } m_anims;

public:
    BombAnimator(Bomb *bomb, anim::AnimationManager *anim, modlib::AssetManager *assets)
        : m_bomb(bomb), m_anim(anim), m_assets(assets)
    {
        assert(m_bomb);
        assert(m_anim);
        assert(m_assets);

        m_object = m_anim->newObject();
        m_slot = m_anim->newSpriteSlot();

        registerAssets();
        buildAnimations();
        animateIdle();

        m_bomb->EvAttack.subscribe([this](EC::Entity::ID){
            animateFire();
        });
    }

    void registerAssets()
    {
        m_assets->registerSprite(bomb_assets::Bomb);

        for (size_t i = 0; i < sizeof(bomb_assets::Fires) / sizeof(bomb_assets::Fires[0]); i++) {
            m_assets->registerSprite(bomb_assets::Fires[i]);
        }
    }

    void buildAnimations()
    {
        m_anims.idle  = buildIdleAnimation();
        m_anims.fire  = buildFireAnimation();
    }

    void animateIdle()
    {
        m_anim->play(m_object, pixelPosition(m_bomb->getPosition()), bomb_sprite::kObjectLayer, m_anims.idle);
    }

    void animateFire()
    {
        auto pos = m_bomb->getPosition();
        m_anim->play(m_object, pixelPosition({pos.x, pos.y}), bomb_sprite::kObjectLayer, m_anims.fire);
    }

    anim::AnimationID buildIdleAnimation()
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_slot, bomb_assets::Bomb.id, bomb_sprite::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildFireAnimation()
    {
        auto *animation = m_anim->newAnimation();

        for (size_t i = 0; i < sizeof(bomb_assets::Fires) / sizeof(bomb_assets::Fires[0]); i++) {
            animation->addStep<anim::SetAssetStep>(m_slot, bomb_assets::Fires[i].id, bomb_sprite::kZ);
            animation->addStep<anim::Step>(0.1, 0.1);
        }
		animation->addStep<anim::DelSpriteStep>(m_slot);
        animation->finishBuild();
        return animation->id();
    }
};

struct BombEntry {
    std::unique_ptr<Bomb> bomb;
    std::unique_ptr<BombAnimator> animator;
};

static const EC::Entity::ID INVALID_BOMB_ID = 0;

static const int kBombTimeOut = 5;

class BombsModule final : public Mod {
    modlib::Level          *m_map    = nullptr;
    modlib::Timer          *m_timer  = nullptr;
    anim::AnimationManager *m_anim   = nullptr;
    modlib::AssetManager   *m_assets = nullptr;
    std::unordered_map<modlib::Entity::ID, BombEntry> m_bombs;
    std::unordered_set<modlib::Entity::ID> m_pendingRemoval;

    bool m_spawned = false;


public:
    std::string_view id()    const override { return "dinichthys.bardak.bombs"; }
    std::string_view brief() const override { return "Static blocking bombs"; }
    ModVersion version()     const override { return ModVersion(0, 0, 1); }

    void onResolveDeps(ModManager *mm) override
    {
        m_map    = mm->requireAnyOfType<modlib::Level>         ("bombs need Map");
        m_timer  = mm->requireAnyOfType<modlib::Timer>         ("bombs need Timer");
        m_anim   = mm->requireAnyOfType<anim::AnimationManager>("bombs need Animator");
        m_assets = mm->requireAnyOfType<modlib::AssetManager>  ("bombs need AssetManager");
    }

    void onDepsResolved(ModManager *) override {}

    modlib::Entity::ID spawnBomb(modlib::Vec2i pos, modlib::Entity::ID owner) {
        modlib::Tile *tile = m_map->getTile(pos);
        if (tile == nullptr || tile->getType() == modlib::Tile::BasicTypes::WALL) {
            return INVALID_BOMB_ID;
        }

        for (const auto &[id, entity] : tile->getEntityList()) {
            (void)id;

            if (entity != nullptr) {
                return INVALID_BOMB_ID;
            }
        }

        auto bomb = std::make_unique<Bomb>(tile);
        const modlib::Entity::ID id = m_map->newEntity(bomb.get(), tile);

        m_timer->setTimer(kBombTimeOut, [this, id, owner]() { explodeBomb(id, owner); }, modlib::Timer::Stage::ON_UPDATE_DONE);

        auto animator = std::make_unique<BombAnimator>(bomb.get(), m_anim, m_assets);
        m_bombs.emplace(id, BombEntry{std::move(bomb), std::move(animator)});

        return id;
    }

    void explodeBomb(modlib::Entity::ID id, modlib::Entity::ID owner) {
        // m_bombs.at(id).animator.get()->animateFire();

        auto &entities = m_map->getEntityList();
        auto bomb_pos = m_bombs.at(id).bomb.get()->getPosition();
        for (auto &entity : entities) {
            if (entity.first == owner) {
                continue;
            }

            auto pos = entity.second->getPosition();
            int32_t dx = pos.x - bomb_pos.x;
            int32_t dy = pos.y - bomb_pos.y;
            if (dx * dx + dy * dy <= kBombRange * kBombRange) {
                if (auto *health = dynamic_cast<EC::Stats::Health*>(entity.second)) {
                    auto hp = health->getCurrentHP();
                    health->inflictDmg(kBombDamage);
                    health->EvDamaged.emit(kBombDamage);
                    if (kBombDamage > hp) {
                        health->EvDeath.emit();
                    }
                }
            }
        }

        m_bombs.at(id).bomb.get()->EvAttack.emit(kBombDamage);

        m_bombs.at(id).bomb.get()->EvEntityDeconstructed.emit();

        m_map->removeEntity(id);

        m_bombs.erase(id);
    }
};

} // namespace
