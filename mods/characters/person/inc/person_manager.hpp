#pragma once

#include "person_controller.hpp"
#include "BmServerModule.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "modlib_mod.hpp"
#include "modlib_manager.hpp"
#include "person_proto.hpp"

class PersonManager {
    Timer        *timer_=nullptr;
    Level        *map_=nullptr;

    std::unordered_map<BmClient *, PersonCtl> people_;
    uint64_t m_tick = 0;
public:
    void setModules(Timer *timer, Level *map) {
        timer_ = timer;
        map_ = map;
    }

    void destroy(BmClient *client) {
        if (people_.count(client)) {
            people_[client].destroy();
            people_.erase(client);
            client->send(bmsg::SV_person_hp { 0 });
        }
    }

    void resolve() {
        timer_->setTimer(1, [this](){ sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }

    void receiveMoveCommand(BmClient *client, bmsg::CL_person_move moveCmd) {
        if (!people_.count(client)) return;
        people_[client].move(moveCmd.dx, moveCmd.dy, m_tick);
    }

    void receiveAttackCommand(BmClient *client, bmsg::CL_person_attack atkCmd) {
        if (!people_.count(client)) return;
        people_[client].attack(atkCmd.whom, m_tick);
    }

    void receiveAcionDone(BmClient *client) {
        if (!people_.count(client)) return;
        auto *person = &people_[client]; 

        person->setActionDoneState(true);
        timer_->setTimer(1, [person](){ person->setActionDoneState(false); }, modlib::Timer::Stage::ON_UPDATE);
    }

    void spawnPerson(BmClient *client) {
        if (people_.count(client)) {
            std::cerr << "person with client `" << client->id() << "` was already spawned\n";
            return;
        }
        people_.try_emplace(client, map_);
    }

    void sendState() {
        ++m_tick;
        auto size = map_->getSize();

        for (auto &[cl, ps] : people_) {
            Vec2i ps_pos = ps.pos();

            cl->send(bmsg::SV_person_at { ps_pos.x, ps_pos.y });
            cl->send(bmsg::SV_person_hp { ps.hp() });

            for (int dx = -4; dx <= 4; ++dx) {
                for (int dy = -4; dy <= 4; ++dy) {
                    int x = ps_pos.x + dx, y = ps_pos.y + dy;

                    if (x < 0 || y < 0 || x >= size.x || y >= size.y)
                        continue;

                    Tile *tile = map_->getTile({x, y});
                    if (tile->getType() == modlib::Tile::BasicTypes::WALL)
                        cl->send(bmsg::SV_person_wall { x, y });

                    for (auto &[id, entity] : tile->getEntityList()) {
                        if (entity != ps.person()) {
                            cl->send(bmsg::SV_person_sees { x, y, (uint32_t)entity->getID() });
                        }
                    }
                }
            }

            cl->send(bmsg::SV_person_tick {});
        }
        timer_->setTimer(1, [this](){ sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }
};  
