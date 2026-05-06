#pragma once

#include "AssetManager.hpp"
#include "Map.hpp"
#include "person_base.hpp"

#include <string_view>

using namespace modlib;

enum class RotationDir {
    up,
    down,
    left,
    right,
};

class Person : 
    virtual public modlib::Entity, 
    virtual public EC::Stats::Health,
    virtual public EC::Stats::Attack
{
public:
    static constexpr int MAX_HP = 200;
    static constexpr int CURRENT_HP = 100;
    static constexpr int STRENGTH = 10;
    static constexpr Type PERSON_TYPE = "person";
private:
    static constexpr std::string_view IDLE_TEXTURE_PATH = MODLIB_PROJECT_ROOT "/assets/units/person/RotatedDown.png";
    static constexpr std::string_view ATTACK_TEXTURE_PATH = MODLIB_PROJECT_ROOT "/assets/units/person/RotatedRight.png";
    static constexpr modlib::EventID IDLE_EVENT = modlib::EventID("idle");
    static constexpr modlib::EventID ATTACK_EVENT = modlib::EventID("attack");

    Level *map_;
    RotationDir dir_ = RotationDir::down;
public:

    Person(Level *map, Tile *tile, modlib::BmClient* client, modlib::AssetManager *assets):
        Entity(PERSON_TYPE, tile),
        Health(MAX_HP, CURRENT_HP),
        Attack(STRENGTH),
        map_(map)
    {
        (void)client;
        registerVisuals(assets);
    }

    void rotate(RotationDir dir) {
        dir_ = dir;
    }

    RotationDir dir() const { return dir_; }

private:
    void registerVisuals(modlib::AssetManager *assets) {
        if (!assets) return;

        const modlib::SpriteID idleSpriteId("per.idle");
        const modlib::SpriteID attackSpriteId("per.atk");

        registerSprite(assets, idleSpriteId, IDLE_TEXTURE_PATH);
        registerSprite(assets, attackSpriteId, ATTACK_TEXTURE_PATH);

        bindEventOnSprite(assets, IDLE_EVENT, idleSpriteId);
        bindEventOnSprite(assets, ATTACK_EVENT, attackSpriteId);
    }

    static void registerSprite(
        modlib::AssetManager *assets,
        modlib::SpriteID id,
        std::string_view texturePath
    ) {
        if (assets->sprite(id)) return;

        modlib::SpriteAsset sprite;
        sprite.id = id;
        sprite.file = texturePath;
        sprite.source = modlib::Recti{0, 0, 16, 16};
        sprite.size = modlib::Vec2i{16, 16};

        assets->registerSprite(sprite);
    }

	static void bindEventOnSprite(
        modlib::AssetManager *assets,
        modlib::EventID eventId,
        modlib::SpriteID spriteId
    ) {
        modlib::VisualBinding binding;
        binding.event_id = eventId;
        binding.sprite = spriteId;

        assets->registerBinding(binding);
    }
};
