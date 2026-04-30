#include <cstdlib>
#include <optional>

#include "BmServerModule.hpp"
#include "Map.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "modlib_mod.hpp"
#include "modlib_manager.hpp"
#include "AssetManager.hpp"
#include "AssetConfig.hpp"
#include <cstdlib>
#include <iostream>
#include <optional>
#include "./person_proto.hpp"

using namespace modlib;

class PersonCtl;
class PersonCtlGfx;

struct Person;
// AssetId PersonCtlGfx::assetId(Person *person) {


struct Person : public Unit {
    enum class rotationDir {
        up,
        down,
        left, 
        right,
    };

    const int max_hp = 200;
    BmClient *m_client;
    PersonCtl *m_ctl;
    PersonCtlGfx *m_gfx;
    Map *m_map;
    Vec2i m_pos;
    uint64_t m_id;
    int m_hp;
    bool m_actionDone = false;
    uint64_t m_nextMoveTick = 0;
    uint64_t m_nextAttackTick = 0;
    rotationDir m_dir;
    

    Person(Map *map, Vec2i pos, size_t id, PersonCtl *ctl, BmClient *cl)
        :m_map(map), m_pos(pos), m_id(id), m_ctl(ctl), m_client(cl), m_hp(max_hp) {}

    Map *map() override { return m_map; }
    Tile *tile() override { return m_map->at(m_pos); }
    
    uint64_t id() override { return m_id; }
    uint64_t type() const override { return 0; }
    uint64_t teamId() const override { return 0; }
    AssetId assetId() const override { return 7; }

    int hp() const override { return m_hp; }
    int maxHp() const override { return max_hp; }

    void takeDamage(int d) override {
        m_hp -= d;
        if (m_hp <= 0) {
            m_hp = 0;
            destroy();
        }
    }

    void pickUp() override {}
    int weight() const override { return 1; }
    void setWeight(const int weight) override {}

    Vec2i pos() const override { return m_pos; }

    void move(Vec2i to) override {
        Unit::move(to);
        m_pos = to;
    }

    void destroy() override {
        m_client->send(bmsg::SV_person_hp { 0 });
        Unit::destroy();
    }
};

static Person::rotationDir convertMoveToDir(int dx, int dy) {
    if (dx == 0 && dy == 0) return Person::rotationDir::down; 

    if (std::abs(dx) > std::abs(dy)) {
        return (dx > 0) ? Person::rotationDir::right: Person::rotationDir::left;
    } else {
        // Vertical movement is dominant (or equal)
        return (dy > 0) ? Person::rotationDir::down : Person::rotationDir::up;
    }
};

struct PersonCtlGfxTexturePackPathes {
    std::string_view left;
    std::string_view right;
    std::string_view down;
    std::string_view up;
};

class PersonCtlGfx {
    AssetId left = 0;
    AssetId right = 0;
    AssetId down = 0;
    AssetId up = 0;


public:
    PersonCtlGfx() = default; 

    void load_assets(AssetManager *assets, PersonCtlGfxTexturePackPathes &texturePack) {
        left = assets->addTexture(texturePack.left);
        if (left == kInvalidAssetId) {
            throw ModManager::Error("Failed to register unit left texture");
        }

        right = assets->addTexture(texturePack.right);
        if (right == kInvalidAssetId) {
            throw ModManager::Error("Failed to register unit right texture");
        }

        down = assets->addTexture(texturePack.down);
        if (down == kInvalidAssetId) {
            throw ModManager::Error("Failed to register unit down texture");
        }

        up = assets->addTexture(texturePack.up);
        if (up == kInvalidAssetId) {
            throw ModManager::Error("Failed to register unit up texture");
        }
    }

    AssetId assetId(Person *person) {
        switch (person->m_dir)
        {
            case Person::rotationDir::down: return down;
            case Person::rotationDir::up: return up;
            case Person::rotationDir::right: return right;
            case Person::rotationDir::left: return left;
            default: return down;
        }
    }
};


class PersonCtl : public BmServerModule {        
    static constexpr uint64_t kMoveCdTicks = 1;
    static constexpr uint64_t kAttackCdTicks = 2;
    static constexpr int kBaseAttackDamage = 10;
    static constexpr int kBerserkBonusDamage = 6; // when low hp
    static constexpr int kLowHpThreshold = 25;

    Timer *tm;
    Map *map;
    AssetManager *assets = nullptr;

    std::unordered_map<BmClient *, Person*> m_people;
    uint64_t m_tick = 0;
    AssetId m_unitAssetId = 0;
    PersonCtlGfx gfx;

    std::string_view id() const override { return "isd.bardak.uctl.person"; };
    std::string_view brief() const override { return "Unit controller for a simple man walking around"; };
    ModVersion version() const override { return ModVersion(0, 0, 1); };

    void onResolveDeps(ModManager *mm) override {
        tm = mm->anyOfType<Timer>();
        map = mm->anyOfType<Map>();
        assets = mm->anyOfType<AssetManager>();
        if (!tm) throw ModManager::Error("Timer module not found");
        if (!map) throw ModManager::Error("Map module not found");
        if (!assets) throw ModManager::Error("AssetManager module not found");
    }

    void sendState() {
        ++m_tick;
        auto size = map->size();

        for (auto [cl, ps] : m_people) {
            Vec2i ps_pos = ps->pos();

            cl->send(bmsg::SV_person_at { ps_pos.x, ps_pos.y });
            cl->send(bmsg::SV_person_hp { ps->hp() });

            for (int dx = -4; dx <= 4; ++dx) {
                for (int dy = -4; dy <= 4; ++dy) {
                    int x = ps_pos.x + dx, y = ps_pos.y + dy;

                    if (x < 0 || y < 0 || x >= size.x || y >= size.y)
                        continue;

                    Tile *tile = map->at({x, y});
                    if (tile->type() == modlib::Tile::BasicType::Wall)
                        cl->send(bmsg::SV_person_wall { x, y });

                    for (auto i : tile->units()) {
                        if (i != ps)
                            cl->send(bmsg::SV_person_sees { x, y, (uint32_t)i->id() });
                    }
                }
            }

            cl->send(bmsg::SV_person_tick {});
        }
        tm->setTimer(1, [this](){ sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void onDepsResolved(ModManager *mm) override {
        // Unit controller registers texture path on init and receives stable asset id.
        PersonCtlGfxTexturePackPathes texturePack;
        texturePack.down = asset_config::personUnitRotatedDownTexturePath;
        texturePack.right = asset_config::personUnitRotatedRightTexturePath;
        texturePack.left = asset_config::personUnitRotatedLeftTexturePath;
        texturePack.up = asset_config::personUnitRotatedUpTexturePath;
        gfx.load_assets(assets, texturePack);


        tm->setTimer(1, [this](){ sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void onSetup(BmServer *server) override {
        server->registerPrefix("person", this);
    };

    void onConnect(BmClient *client) override {
        auto sz = map->size();
        assert(sz.x > 2 && sz.y > 2);
        m_people[client] = map->spawn<Person>(
            Vec2i {
                1 + rand() % (sz.x - 2),
                1 + rand() % (sz.y - 2)
            },
            this,
            client
        );
    }

    void onMessage(BmClient *cl, bmsg::RawMessage m) override {
        assert(m.isCorrect());
        if (!m_people.count(cl)) return;
        auto pl = m_people.at(cl);
        if (pl->m_actionDone) return;

        if (m.header()->type == "move") {
            auto moveCmd = bmsg::CL_person_move::decode(m);
            if (moveCmd == std::nullopt) return;
            if (abs(moveCmd->dx) > 1 || abs(moveCmd->dy) > 1) return;
            if (m_tick < pl->m_nextMoveTick) return;
            Vec2i pos = pl->pos();
            pl->m_dir = convertMoveToDir(moveCmd->dx, moveCmd->dy);
            pos.x += moveCmd->dx; pos.y += moveCmd->dy;
            if (map->at(pos)->type() == Tile::BasicType::Wall) return;
            pl->move(pos);
            pl->m_nextMoveTick = m_tick + kMoveCdTicks;
        } else if (m.header()->type == "attack") {
            auto atkCmd = bmsg::CL_person_attack::decode(m);
            if (!atkCmd) return;
            if (m_tick < pl->m_nextAttackTick) return;
            auto u = map->byId(atkCmd->whom);
            // std::cerr << "attack " << u << '\n';
            if (!u) return;
            if (abs(u->pos().x - pl->pos().x) > 1 || abs(u->pos().y - pl->pos().y) > 1)
                return;
            int dmg = kBaseAttackDamage;
            if (pl->hp() <= kLowHpThreshold) dmg += kBerserkBonusDamage;
            u->takeDamage(dmg);
            pl->m_nextAttackTick = m_tick + kAttackCdTicks;
        }

        pl->m_actionDone = true;
        tm->setTimer(1, [pl](){ pl->m_actionDone = false; }, modlib::Timer::Stage::ON_UPDATE);
    }

    void onDisconnect(BmClient *cl) override {
        if (m_people.count(cl)) {
            m_people[cl]->destroy();
            m_people.erase(cl);
        }
    }
};

extern "C" Mod *modlib_create(ModManager *mm) {
    return new PersonCtl();
}
