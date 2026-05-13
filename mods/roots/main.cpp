#include "Animator.hpp"
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

namespace {

constexpr float kTilePixels = 16.0f;

namespace root_sprite {

struct Config {
    static constexpr modlib::Rectf kClip = {0, 0, 16, 16};
    static constexpr modlib::Vec2f kSize = {kTilePixels, kTilePixels};
};

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
        anim::AnimationID idle = anim::NO_ANIMATION;
        anim::AnimationID hit = anim::NO_ANIMATION;
        anim::AnimationID death = anim::NO_ANIMATION;
    } m_anims;

public:
    RootAnimator(Root *root, anim::AnimationManager *anim, modlib::AssetManager *assets)
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
        animateIdle();
    }

private:
    void registerAssets()
    {
        m_assets->registerSprite(root_assets::Root);
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

class RootsModule final : public Mod {
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
            spawnRoot(pos);
        }
    }

    void spawnRoot(modlib::Vec2i pos)
    {
        modlib::Tile *tile = m_map->getTile(pos);
        if (tile == nullptr || tile->getType() == modlib::Tile::BasicTypes::WALL) {
            return;
        }

        auto root = std::make_unique<Root>(tile);
        const modlib::Entity::ID id = m_map->newEntity(root.get(), tile);

        root->EvDeath.subscribe([this, id]() {
            m_pendingRemoval.insert(id);
        });

        auto animator = std::make_unique<RootAnimator>(root.get(), m_anim, m_assets);
        m_roots.emplace(id, RootEntry{std::move(root), std::move(animator)});
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
