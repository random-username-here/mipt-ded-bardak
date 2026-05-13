#include "Animator.hpp"
#include "AssetManager.hpp"
#include "Map.hpp"
#include "Roots.hpp"
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

namespace {

constexpr float kTilePixels = 16.0f;

namespace root_sprite {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kTilePixels, kTilePixels};
};

namespace root_grow {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kTilePixels, kTilePixels};
};

constexpr int kZ = 1;
static constexpr float kFrameSeconds = 0.045f;

} // namespace root_grow

constexpr int kObjectLayer = 1;
constexpr int kZ = 0;
static constexpr float kFlashSeconds = 0.08f;

} // namespace root_sprite

namespace root_assets {

static const modlib::SpriteAsset Root = {
    .id = modlib::SpriteID("root"),
    .file = ASSETS_DIR "/entities/root.png",
    .clip = root_sprite::Config::kClip,
    .size = root_sprite::Config::kSize,
};

inline modlib::SpriteAsset growSprite(std::string_view id, int col)
{
    return {
        .id   = id,
        .file = ASSETS_DIR "/units/mage/anim_root_grow.png",
        .clip = {static_cast<float>(col * 16), 0, 16, 16},
        .size = root_sprite::root_grow::Config::kSize,
    };
}

static const std::array<modlib::SpriteAsset, 4> Grow = {
    growSprite("root.g.1", 0),
    growSprite("root.g.2", 1),
    growSprite("root.g.3", 2),
    growSprite("root.g.4", 3),
};

} // namespace root_assets

modlib::Vec2f pixelPosition(modlib::Vec2i cell)
{
    return modlib::Vec2f(cell.x * kTilePixels, cell.y * kTilePixels);
}

class Root final : public virtual modlib::Entity, public virtual EC::Stats::Health {
public:
    static constexpr EC::Stats::Health::HP kHp = 30;

    Root(modlib::Tile *tile)
        : modlib::Entity(modlib::Entity::BasicTypes::ROOT, tile)
        , EC::Stats::Health(kHp, kHp)
    {}
};

class RootAnimator {
    Root                   *m_root   = nullptr;
    anim::AnimationManager *m_anim   = nullptr;
    modlib::AssetManager   *m_assets = nullptr;
    anim::AnimatedObjectID  m_object = anim::NO_ANIMATION_OBJECT;
    anim::SpriteSlotID      m_slot   = 0;

    struct {
        anim::AnimationID idle  = anim::NO_ANIMATION;
        anim::AnimationID hit   = anim::NO_ANIMATION;
        anim::AnimationID death = anim::NO_ANIMATION;
    } m_anims;

public:
    RootAnimator(Root *root, anim::AnimationManager *anim, modlib::AssetManager *assets, bool grow)
        : m_root(root), m_anim(anim), m_assets(assets)
    {
        assert(m_root);
        assert(m_anim);
        assert(m_assets);

        m_object = m_anim->newObject();
        m_slot = m_anim->newSpriteSlot();
        registerAssets();
        buildAnimations();
        subscribe();
        if (grow) {
            animateGrow();
        } else {
            animateIdle();
        }
    }

    void animateGrow()
    {
        auto *animation = m_anim->newAnimation();

        for (const auto &grow : root_assets::Grow) {
            animation->addStep<anim::SetAssetStep>(m_slot, grow.id, root_sprite::root_grow::kZ);
            animation->addStep<anim::Step>(root_sprite::root_grow::kFrameSeconds, root_sprite::root_grow::kFrameSeconds);
        }

        animation->addStep<anim::SetAssetStep>(m_slot, root_assets::Root.id, root_sprite::kZ);
        animation->finishBuild();

        m_anim->play(m_object, pixelPosition(m_root->getPosition()), root_sprite::kObjectLayer, animation->id());
    }

private:
    void registerAssets()
    {
        m_assets->registerSprite(root_assets::Root);
        for (const auto &asset : root_assets::Grow) {
            m_assets->registerSprite(asset);
        }
    }

    void buildAnimations()
    {
        m_anims.idle  = buildIdleAnimation();
        m_anims.hit   = buildHitAnimation();
        m_anims.death = buildDeathAnimation();
    }

    void subscribe()
    {
        m_root->EvDamaged.subscribe([this](EC::Stats::Health::HP) { animateHit(); });
        m_root->EvDeath  .subscribe([this]() { animateDeath(); });
    }

    void animateIdle()
    {
        m_anim->play(m_object, pixelPosition(m_root->getPosition()), root_sprite::kObjectLayer, m_anims.idle);
    }

    void animateHit()
    {
        if (m_root->getTile() == nullptr) {
            return;
        }
        m_anim->play(m_object, pixelPosition(m_root->getPosition()), root_sprite::kObjectLayer, m_anims.hit);
    }

    void animateDeath()
    {
        if (m_root->getTile() == nullptr) {
            return;
        }
        m_anim->play(m_object, pixelPosition(m_root->getPosition()), root_sprite::kObjectLayer, m_anims.death);
    }

    anim::AnimationID buildIdleAnimation()
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_slot, root_assets::Root.id, root_sprite::kZ);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildHitAnimation()
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_slot, root_assets::Root.id, root_sprite::kZ);
        animation->addStep<anim::SetWhiteStep>(m_slot, true);
        animation->addStep<anim::Step>(root_sprite::kFlashSeconds, root_sprite::kFlashSeconds);
        animation->addStep<anim::SetWhiteStep>(m_slot, false);
        animation->finishBuild();
        return animation->id();
    }

    anim::AnimationID buildDeathAnimation()
    {
        auto *animation = m_anim->newAnimation();
        animation->addStep<anim::SetAssetStep>(m_slot, root_assets::Root.id, root_sprite::kZ);
        animation->addStep<anim::SetWhiteStep>(m_slot, true);
        animation->addStep<anim::Step>(root_sprite::kFlashSeconds, root_sprite::kFlashSeconds);
        animation->addStep<anim::DelSpriteStep>(m_slot);
        animation->finishBuild();
        return animation->id();
    }
};

struct RootEntry {
    std::unique_ptr<Root> root;
    std::unique_ptr<RootAnimator> animator;
};

class RootsModule final : public Mod, public modlib::RootSystem {
    modlib::Level          *m_map    = nullptr;
    modlib::Timer          *m_timer  = nullptr;
    anim::AnimationManager *m_anim   = nullptr;
    modlib::AssetManager   *m_assets = nullptr;
    std::unordered_map<modlib::Entity::ID, RootEntry> m_roots;
    std::unordered_set<modlib::Entity::ID> m_pendingRemoval;

    bool m_spawned = false;

public:
    std::string_view id()    const override { return "ashww.bardak.roots"; }
    std::string_view brief() const override { return "Static damageable blocking roots"; }
    ModVersion version()     const override { return ModVersion(0, 0, 1); }

    void onResolveDeps(ModManager *mm) override
    {
        m_map    = mm->requireAnyOfType<modlib::Level>         ("Roots need Map");
        m_timer  = mm->requireAnyOfType<modlib::Timer>         ("Roots need Timer");
        m_anim   = mm->requireAnyOfType<anim::AnimationManager>("Roots need Animator");
        m_assets = mm->requireAnyOfType<modlib::AssetManager>  ("Roots need AssetManager");
    }

    void onDepsResolved(ModManager *) override
    {
        m_timer->setTimer(
            1,
            [this]() {
                spawnDefaults();
                sweepDeadRoots();
            },
            modlib::Timer::Stage::ON_UPDATE_DONE
        );
    }

    modlib::Entity::ID spawnRoot(modlib::Vec2i pos, bool animateGrow) override
    {
        modlib::Tile *tile = m_map->getTile(pos);
        if (tile == nullptr || tile->getType() == modlib::Tile::BasicTypes::WALL) {
            return modlib::RootSystem::INVALID_ROOT_ID;
        }

        for (const auto &[id, entity] : tile->getEntityList()) {
            (void)id;

            if (entity != nullptr) {
                return modlib::RootSystem::INVALID_ROOT_ID;
            }
        }

        auto root = std::make_unique<Root>(tile);
        const modlib::Entity::ID id = m_map->newEntity(root.get(), tile);

        root->EvDeath.subscribe([this, id]() {
            m_pendingRemoval.insert(id);
        });

        auto animator = std::make_unique<RootAnimator>(root.get(), m_anim, m_assets, animateGrow);
        m_roots.emplace(id, RootEntry{std::move(root), std::move(animator)});

        return id;
    }

private:
    void spawnDefaults()
    {
        if (m_spawned) {
            return;
        }
        m_spawned = true;

        static const std::vector<modlib::Vec2i> positions = {
            { 4,  4},
            { 8,  5},
            {11,  9},
            {15, 12},
            { 6, 15},
        };

        for (const modlib::Vec2i pos : positions) {
            spawnRoot(pos, false);
        }
    }

    void sweepDeadRoots()
    {
        for (const modlib::Entity::ID id : m_pendingRemoval) {
            auto it = m_roots.find(id);
            if (it == m_roots.end()) {
                continue;
            }
            m_map->removeEntity(id);
            m_roots.erase(it);
        }
        m_pendingRemoval.clear();

        m_timer->setTimer(1, [this]() { sweepDeadRoots(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }
};

} // namespace

extern "C" Mod *modlib_create(ModManager *)
{
    return new RootsModule();
}
