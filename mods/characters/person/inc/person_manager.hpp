#pragma once

#include "person.hpp"

class PersonManager {
    Timer        *timer_=nullptr;
    Map          *map_=nullptr;
    AssetManager *assetManager_=nullptr;

    std::unordered_map<BmClient *, PersonCtl> people_;
    uint64_t m_tick = 0;
public:
    void setModules(Timer *timer, Map *map, AssetManager *assetManager) {
        timer_ = timer;
        map_ = map;
        assetManager_ = assetManager;
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

        // PersonCtlGfxTexturePackPathes texturePack;
        // texturePack.down = asset_config::personUnitRotatedDownTexturePath;
        // texturePack.right = asset_config::personUnitRotatedRightTexturePath;
        // texturePack.left = asset_config::personUnitRotatedLeftTexturePath;
        // texturePack.up = asset_config::personUnitRotatedUpTexturePath;
        // gfx.load_assets(assets, texturePack);
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
        auto size = map_->size();

        for (auto [cl, ps] : people_) {
            Vec2i ps_pos = ps.pos();

            cl->send(bmsg::SV_person_at { ps_pos.x, ps_pos.y });
            cl->send(bmsg::SV_person_hp { ps.hp() });

            for (int dx = -4; dx <= 4; ++dx) {
                for (int dy = -4; dy <= 4; ++dy) {
                    int x = ps_pos.x + dx, y = ps_pos.y + dy;

                    if (x < 0 || y < 0 || x >= size.x || y >= size.y)
                        continue;

                    Tile *tile = map_->at({x, y});
                    if (tile->type() == modlib::Tile::BasicType::Wall)
                        cl->send(bmsg::SV_person_wall { x, y });

                    for (auto i : tile->units()) {
                        if (i != ps.person())
                            cl->send(bmsg::SV_person_sees { x, y, (uint32_t)i->id() });
                    }
                }
            }

            cl->send(bmsg::SV_person_tick {});
        }
        timer_->setTimer(1, [this](){ sendState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
    }
};  
