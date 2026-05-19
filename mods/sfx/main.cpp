#include "AssetManager.hpp"
#include "Lobby.hpp"
#include "Map.hpp"
#include "Sfx.hpp"
#include "Timer.hpp"
#include "mage.hpp"
#include "modlib_manager.hpp"

#include <optional>
#include <random>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace {

sfx::Cue cue(std::string_view id) {
    sfx::Cue out;
    out.id = bmsg::Char64(id);
    return out;
}

class SfxModule final : public sfx::SoundManager {
    modlib::AssetManager *m_assets = nullptr;
    modlib::Level        *m_map    = nullptr;
    modlib::Timer        *m_timer  = nullptr;
    modlib::Lobby        *m_lobby  = nullptr;

    std::unordered_map<uint64_t, size_t>   m_usedByKey;
    std::unordered_set<modlib::Entity::ID> m_seenEntities;

    std::mt19937 m_rng{std::random_device{}()};

public:
    std::string_view id()    const override { return "ashww.bardak.sfx";             }
    std::string_view brief() const override { return "Sound effect event collector"; }
    ModVersion version()     const override { return ModVersion(0, 1, 0);            }

    void onResolveDeps(ModManager *mm) override {
        m_assets = mm->requireAnyOfType<modlib::AssetManager>("SFX needs AssetManager");
        m_map    = mm->requireAnyOfType<modlib::Level>("SFX needs Level");
        m_timer  = mm->requireAnyOfType<modlib::Timer>("SFX needs Timer");
        m_lobby  = mm->anyOfType<modlib::Lobby>();
    }

    void onDepsResolved(ModManager *) override {
        registerSounds();

        m_map->EvEntitySpawned.subscribe([this](modlib::Entity::ID id) {
            subscribeEntity(id);
        });

        for (const auto &[id, entity] : m_map->getEntityList()) {
            (void)entity;
            subscribeEntity(id);
        }

        if (m_lobby != nullptr) {
            m_lobby->EvGameEnded.subscribe([this](const modlib::LobbyGameEndedInfo &) {
                playNow(cue("gameover"));
            });
        }

        m_timer->setTimer(
            1,
            [this]() {
                m_usedByKey.clear();
            },
            modlib::Timer::Stage::ON_UPDATE_DONE,
            modlib::Timer::Type::CYCLE
        );

        scheduleBird();
    }

    void play(sfx::Cue soundCue) override {
        auto item = resolve(soundCue);
        if (!item) {
            return;
        }

        size_t &used = m_usedByKey[item->limitKey];

        if (item->maxPerTick != 0 && used >= item->maxPerTick) {
            return;
        }

        ++used;
        onPlay().emit(item->cue);
    }

private:
    void registerSounds() {
        registerSound("hit",      ASSETS_DIR "/sfx/hit.wav",           0.75f, 1.0f, 50,  0.05f, 4, "hit");
        registerSound("slash",    ASSETS_DIR "/sfx/slash.wav",         0.80f, 1.0f, 70,  0.00f, 3, "attack");
        registerSound("cut",      ASSETS_DIR "/sfx/cut.wav",           0.80f, 1.0f, 70,  0.00f, 3, "attack");
        registerSound("gameover", ASSETS_DIR "/sfx/gameover.wav",      1.00f, 1.0f, 100, 0.00f, 1, "ui");
        registerSound("step",     ASSETS_DIR "/sfx/step.wav",          0.35f, 1.0f, 10,  0.00f, 8, "step");
        registerSound("roots",    ASSETS_DIR "/sfx/roots_appear.wav",  0.80f, 1.0f, 75,  0.00f, 2, "magic");
        registerSound("fire",     ASSETS_DIR "/sfx/fire.wav",          0.85f, 1.0f, 75,  0.00f, 2, "magic");
        registerSound("bird",     ASSETS_DIR "/sfx/ambience_bird.wav", 0.45f, 1.0f, 0,   0.00f, 1, "amb");
        registerSound("heal",     ASSETS_DIR "/sfx/heal.wav",          0.80f, 1.0f, 80,  0.00f, 2, "magic");

        modlib::MusicAsset bgm;
        bgm.id     = bmsg::Char64("bgm");
        bgm.file   = ASSETS_DIR "/music/bgm.ogg";
        bgm.volume = 0.3f;
        bgm.loop   = true;

        m_assets->registerMusic(std::move(bgm));
    }

    void registerSound(
        std::string_view   id,
        const std::string &file,
        float  volume,
        float  pitch,
        int    priority,
        float  delaySeconds,
        size_t maxPerTick,
        std::string_view group
    ) {
        modlib::SoundAsset asset;
        asset.id           = bmsg::Char64(id);
        asset.file         = file;
        asset.volume       = volume;
        asset.pitch        = pitch;
        asset.priority     = priority;
        asset.delaySeconds = delaySeconds;
        asset.maxPerTick   = maxPerTick;
        asset.group        = bmsg::Char64(group);

        m_assets->registerSound(std::move(asset));
    }

    void subscribeEntity(modlib::Entity::ID id) {
        if (m_seenEntities.count(id) != 0) {
            return;
        }

        modlib::Entity *entity = m_map->getEntity(id);
        if (entity == nullptr) {
            return;
        }

        m_seenEntities.insert(id);

        entity->EvEntityMoved.subscribe([this](modlib::Vec2i) {
            playStep();
        });

        if (auto *health = dynamic_cast<EC::Stats::Health *>(entity)) {
            health->EvDamaged.subscribe([this](EC::Stats::Health::HP) {
                play(cue("hit"));
            });
        }

        if (auto *attack = dynamic_cast<EC::Stats::Attack *>(entity)) {
            const modlib::Entity::Type type = entity->getType();

            attack->EvAttack.subscribe([this, type](EC::Stats::Attack::Damage) {
                playAttack(type);
            });
        }

        if (auto *mage = dynamic_cast<Mage *>(entity)) {
            mage->EvCast.subscribe([this](bmsg::Char64 spell, modlib::Vec2i) {
                playMageCast(spell);
            });
        }

        entity->EvEntityDeconstructed.subscribe([this, id]() {
            m_seenEntities.erase(id);
        });
    }

    void playStep() {
        sfx::Cue first = cue("step");
        play(first);

        sfx::Cue second = cue("step");
        second.delaySeconds = 0.08f;
        play(second);
    }

    void playAttack(modlib::Entity::Type type) {
        if (type == "knight") {
            play(cue("slash"));
            return;
        }

        if (type == "rogue") {
            play(cue("cut"));
            return;
        }
    }

    void playMageCast(bmsg::Char64 spell) {
        if (spell == "heal") {
            play(cue("heal"));
            return;
        }

        if (spell == "flame") {
            play(cue("fire"));
            return;
        }

        if (spell == "plant") {
            play(cue("roots"));
            return;
        }
    }

    struct ResolvedCue {
        sfx::Cue cue;
        uint64_t limitKey = 0;
        size_t maxPerTick = 0;
    };

    std::optional<ResolvedCue> resolve(sfx::Cue soundCue) const {
        const auto asset = m_assets->sound(soundCue.id);
        if (!asset) {
            return std::nullopt;
        }

        if (soundCue.volume < 0.0f) {
            soundCue.volume = asset->volume;
        }

        if (soundCue.pitch < 0.0f) {
            soundCue.pitch = asset->pitch;
        }

        if (soundCue.delaySeconds < 0.0f) {
            soundCue.delaySeconds = asset->delaySeconds;
        }

        if (soundCue.priority == -1000000) {
            soundCue.priority = asset->priority;
        }

        const uint64_t limitKey = asset->group.as_u64 != 0
            ? asset->group.as_u64
            : asset->id.as_u64;

        return ResolvedCue{
            .cue = soundCue,
            .limitKey = limitKey,
            .maxPerTick = asset->maxPerTick,
        };
    }

    void playNow(sfx::Cue soundCue) {
        auto item = resolve(soundCue);
        if (item) {
            onPlay().emit(item->cue);
        }
    }

    void scheduleBird() {
        std::uniform_int_distribution<size_t> delay(20, 70);

        m_timer->setTimer(
            delay(m_rng),
            [this]() {
                playNow(cue("bird"));
                scheduleBird();
            },
            modlib::Timer::Stage::ON_UPDATE_DONE
        );
    }
};

} // namespace

extern "C" Mod *modlib_create(ModManager *) {
    return new SfxModule();
}
