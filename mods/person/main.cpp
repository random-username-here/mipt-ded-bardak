#include "BmServerModule.hpp"
#include "Map.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "modlib_mod.hpp"
#include "modlib_manager.hpp"
#include <cstdlib>
#include <optional>
#include "./person_proto.hpp"

using namespace modlib;

class PersonCtl;

struct Person : public Unit {
    BmClient *m_client;
    PersonCtl *m_ctl;
    Map *m_map;
    Vec2i m_pos;
    size_t m_id;
    int m_hp;
    bool m_actionDone = false;
    uint64_t m_nextMoveTick = 0;
    uint64_t m_nextAttackTick = 0;

    Person(Map *map, Vec2i pos, size_t id, PersonCtl *ctl, BmClient *cl) 
        :m_map(map), m_pos(pos), m_id(id), m_ctl(ctl), m_client(cl), m_hp(100) {}

    Map *map() override { return m_map; }
    Tile *tile() override { return m_map->at(m_pos); }
    
    uint64_t type() const override { return 0; }
    uint64_t teamId() const override { return 0; }

    int hp() const override { return m_hp; }
    Vec2i pos() const override { return m_pos; }
    size_t id() override { return m_id; }

    void takeDamage(int d) override {
        m_hp -= d;
        if (m_hp < 0) m_hp = 0;
        if (m_hp == 0) destroy();
    }

    void destroy() override {
        m_client->send(bmsg::SV_person_hp { 0 });
        Unit::destroy();
    }
};

class PersonCtl : public BmServerModule {

    std::string_view id() const override { return "isd.bardak.uctl.person"; };
    std::string_view brief() const override { return "Unit controller for a simple man walking around"; };
    ModVersion version() const override { return ModVersion(0, 0, 1); };

    Timer *tm;
    Map *map;
    std::unordered_map<BmClient *, Person*> m_people;
    uint64_t m_tick = 0;

    static constexpr uint64_t kMoveCdTicks = 1;
    static constexpr uint64_t kAttackCdTicks = 2;
    static constexpr int kBaseAttackDamage = 10;
    static constexpr int kBerserkBonusDamage = 6; // when low hp
    static constexpr int kLowHpThreshold = 25;

    void onResolveDeps(ModManager *mm) override {
        tm = mm->anyOfType<Timer>();
        map = mm->anyOfType<Map>();
        if (!tm) throw ModManager::Error("Timer module not found");
        if (!map) throw ModManager::Error("Map module not found");
    }

    void tick() {
        ++m_tick;
        auto size = map->size();
        for (auto [cl, ps] : m_people) {
            cl->send(bmsg::SV_person_tick {});
            cl->send(bmsg::SV_person_at { ps->pos().x, ps->pos().y });
            cl->send(bmsg::SV_person_hp { ps->hp() });
            ps->takeDamage(1);
            for (int dx = -4; dx <= 4; ++dx) {
                for (int dy = -4; dy <= 4; ++dy) {
                    int x = ps->pos().x + dx, y = ps->pos().y + dy;
                    if (x < 0 || y < 0 || x >= size.x || y >= size.y)
                        continue;
                    if (map->at({x, y})->type() == Tile::BasicType::Wall)
                        cl->send(bmsg::SV_person_wall { x, y });
                    for (auto i : map->at({x, y})->units())
                        if (i != ps)
                            cl->send(bmsg::SV_person_sees { x, y, (uint32_t) i->id() });
                }
            }
        }
        tm->setTimer(1, [this](){ tick(); }, modlib::Timer::Stage::ON_UPDATE);
    }

    void onDepsResolved(ModManager *mm) override {
        tm->setTimer(1, [this](){ tick(); }, modlib::Timer::Stage::ON_UPDATE);
    }

    void onSetup(BmServer *server) override {
        server->registerPrefix("person", this);
    };

    void onConnect(BmClient *client) override {
        m_people[client] = map->spawn<Person>(Vec2i { rand() % map->size().x, rand() % map->size().y }, this, client);
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
            pos.x += moveCmd->dx; pos.y += moveCmd->dy;
            auto *t = map->at(pos);
            if (!t) return;
            // New feature: when low HP, allow phasing through walls.
            if (!(t->type() == Tile::BasicType::Wall) && pl->hp() > kLowHpThreshold) return;
            pl->move(pos);
            pl->m_nextMoveTick = m_tick + kMoveCdTicks;
        } else if (m.header()->type == "attack") {
            auto atkCmd = bmsg::CL_person_attack::decode(m);
            if (!atkCmd) return;
            if (m_tick < pl->m_nextAttackTick) return;
            auto u = map->byId(atkCmd->whom);
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
