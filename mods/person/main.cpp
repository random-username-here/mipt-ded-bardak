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

    Person(Map *map, Vec2i pos, size_t id, PersonCtl *ctl, BmClient *cl) 
        :m_map(map), m_pos(pos), m_id(id), m_ctl(ctl), m_client(cl), m_hp(100) {}

    Map *map() override { return m_map; }
    Tile *tile() override { return m_map->at(m_pos); }
    int hp() const override { return m_hp; }
    Vec2i pos() const override { return m_pos; }
    size_t id() override { return m_id; }

    void takeDamage(int d) override {
        m_hp += d;
        if (m_hp < 0) m_hp = 0;
        m_client->send(bmsg::SV_person_hp { m_hp });
        if (m_hp < 0) destroy();
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

    void onResolveDeps(ModManager *mm) override {
        tm = mm->anyOfType<Timer>();
        map = mm->anyOfType<Map>();
        if (!tm) throw ModManager::Error("Timer module not found");
        if (!map) throw ModManager::Error("Map module not found");
    }

    void tick() {
        auto size = map->size();
        for (auto [cl, ps] : m_people) {
            cl->send(bmsg::SV_person_tick {});
            for (int dx = -4; dx <= 4; ++dx) {
                for (int dy = -4; dy <= 4; ++dy) {
                    int x = ps->pos().x, y = ps->pos().y;
                    if (x < 0 || y < 0 || x >= size.x || y >= size.y)
                        continue;
                    if (!map->at({x, y})->isWalkable())
                        cl->send(bmsg::SV_person_wall { x, y });
                    for (auto i : map->at({x, y})->units())
                        cl->send(bmsg::SV_person_sees { x, y, (uint32_t) i->id() });
                }
            }
        }
        tm->setTimer(1, [this](){ tick(); });
    }

    void onDepsResolved(ModManager *mm) override {
        tm->setTimer(1, [this](){ tick(); });
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
            Vec2i pos = pl->pos();
            pos.x += moveCmd->dx; pos.y += moveCmd->dy;
            if (!map->at(pos)->isWalkable()) return;
            pl->move(pos);
        } else if (m.header()->type == "attack") {
            // pass            
        }

        tm->setTimer(1, [pl](){ pl->m_actionDone = false; });
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
