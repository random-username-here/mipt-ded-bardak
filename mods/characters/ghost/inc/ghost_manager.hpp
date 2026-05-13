#pragma once

#include "ghost_animator.hpp"
#include "ghost_controller.hpp"
#include "BmServerModule.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "modlib_mod.hpp"
#include "modlib_manager.hpp"
#include "ghost_proto.hpp"

#include <iostream>
#include <unordered_map>

namespace {

constexpr int kVisionRadius = 4;
constexpr uint64_t kGhostTeam = 1;
constexpr uint32_t kPacmanTeamProtocol = 0;

inline uint32_t protocolTeamIdForGhost(modlib::Entity *e)
{
	if (e == nullptr) {
		return 0;
	}
	if (auto *g = dynamic_cast<Ghost *>(e)) {
		return static_cast<uint32_t>(g->teamId());
	}
	if (e->getType() == modlib::Entity::Type("pacman")) {
		return kPacmanTeamProtocol;
	}
	return 0;
}

inline bool isAliveEntityForGhost(modlib::Entity *e)
{
	if (auto *health = dynamic_cast<EC::Stats::Health *>(e)) {
		return health->getCurrentHP() > 0;
	}
	return true;
}

} // namespace

class GhostManager {
	Timer *timer_ = nullptr;
	Level *map_ = nullptr;
	anim::AnimationManager *animator_ = nullptr;
	modlib::AssetManager *assets_ = nullptr;

	struct GhostUtils {
		GhostCtl ctl;
		GhostAnimator anim;

		GhostUtils(Level *map, BmClient *client, anim::AnimationManager *animator,
		           modlib::AssetManager *assets, uint64_t team_id)
		    : ctl(map, client, team_id)
		    , anim(&ctl, animator, assets)
		{}
	};

	std::unordered_map<BmClient *, GhostUtils> ghosts_;
	uint64_t m_tick = 0;

public:
	void setModules(Timer *timer, Level *map, anim::AnimationManager *animator,
	                modlib::AssetManager *assets)
	{
		timer_ = timer;
		map_ = map;
		animator_ = animator;
		assets_ = assets;
	}

	void destroy(BmClient *client)
	{
		const auto it = ghosts_.find(client);
		if (it != ghosts_.end()) {
			it->second.ctl.destroy();
			ghosts_.erase(it);
			client->send(bmsg::SV_ghost_hp{0});
		}
	}

	void resolve()
	{
		timer_->setTimer(1, [this]() { sendPeriodicState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
	}

	void receiveMoveCommand(BmClient *client, bmsg::CL_ghost_move move_cmd)
	{
		const auto it = ghosts_.find(client);
		if (it == ghosts_.end()) {
			return;
		}

		it->second.ctl.move(move_cmd.dx, move_cmd.dy, m_tick);
	}

	void receiveAttackCommand(BmClient *client, bmsg::CL_ghost_attack atk_cmd)
	{
		const auto it = ghosts_.find(client);
		if (it == ghosts_.end()) {
			return;
		}

		it->second.ctl.attack(atk_cmd.whom, m_tick);
	}

	void receiveWhereCommand(BmClient *client, bmsg::CL_ghost_where where_cmd)
	{
		sendWhereFor(client, where_cmd.teamId);
	}

	void receiveSeesCommand(BmClient *client)
	{
		sendVisionFor(client);
	}

	size_t count(BmClient *client) const
	{
		return ghosts_.count(client);
	}

	void spawnGhost(BmClient *client)
	{
		if (ghosts_.count(client)) {
			std::cerr << "ghost with client `" << client->id() << "` was already spawned\n";
			return;
		}

		ghosts_.try_emplace(client, map_, client, animator_, assets_, kGhostTeam);
	}

  private:
	void sendPeriodicState()
	{
		++m_tick;

		for (auto &[cl, gs] : ghosts_) {
			cl->send(bmsg::SV_ghost_hp{gs.ctl.hp()});
			if (!gs.ctl.alive()) {
				gs.ctl.destroy();
				continue;
			}

			const Vec2i ppos = gs.ctl.pos();
			cl->send(bmsg::SV_ghost_tick{});
			cl->send(bmsg::SV_ghost_at{ppos.x, ppos.y});
		}

		timer_->setTimer(1, [this]() { sendPeriodicState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
	}

	void sendWhereFor(BmClient *client, uint32_t team_id)
	{
		for (const auto &[_, ent] : map_->getEntityList()) {
			if (!isAliveEntityForGhost(ent)) {
				continue;
			}
			const uint32_t tid = protocolTeamIdForGhost(ent);
			if (tid != team_id) {
				continue;
			}
			const Vec2i p = ent->getPosition();
			client->send(bmsg::SV_ghost_where{p.x, p.y, static_cast<uint32_t>(ent->getID()), tid});
		}
	}

	void sendVisionFor(BmClient *client)
	{
		const auto it = ghosts_.find(client);
		if (it == ghosts_.end()) {
			return;
		}
		if (!it->second.ctl.alive()) {
			return;
		}

		const Vec2i origin = it->second.ctl.pos();
		const auto size = map_->getSize();

		for (int dx = -kVisionRadius; dx <= kVisionRadius; ++dx) {
			for (int dy = -kVisionRadius; dy <= kVisionRadius; ++dy) {
				const int x = origin.x + dx;
				const int y = origin.y + dy;
				if (x < 0 || y < 0 || x >= size.x || y >= size.y) {
					continue;
				}

				Tile *tile = map_->getTile({x, y});
				if (tile == nullptr) {
					continue;
				}

				if (tile->getType() == Tile::BasicTypes::WALL) {
					client->send(bmsg::SV_ghost_wall{x, y});
				}

				for (const auto &[eid, entity] : tile->getEntityList()) {
					(void)eid;
					if (entity != it->second.ctl.ghost() && isAliveEntityForGhost(entity)) {
						client->send(bmsg::SV_ghost_sees{x, y, static_cast<uint32_t>(entity->getID()),
						                              protocolTeamIdForGhost(entity)});
					}
				}
			}
		}
	}
};
