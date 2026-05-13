#pragma once

#include "ghost_animator.hpp"
#include "ghost_controller.hpp"
#include "pacman/inc/pacman.hpp"
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
		           modlib::AssetManager *assets)
		    : ctl(map, client)
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

		ghosts_.try_emplace(client, map_, client, animator_, assets_);
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
			cl->send(bmsg::SV_ghost_at{ppos.x, ppos.y});
			sendVisibleWalls(cl, gs.ctl.pos());
			sendVisibleTargets(cl);
			cl->send(bmsg::SV_ghost_tick{});
		}

		timer_->setTimer(1, [this]() { sendPeriodicState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
	}

	void sendVisibleTargets(BmClient *client)
	{
		for (const auto &[_, ent] : map_->getEntityList()) {
			if (!isAliveEntityForGhost(ent) || dynamic_cast<Pacman *>(ent) == nullptr) {
				continue;
			}
			const Vec2i p = ent->getPosition();
			client->send(bmsg::SV_ghost_sees{p.x, p.y, static_cast<uint32_t>(ent->getID())});
		}
	}

	void sendVisibleWalls(BmClient *client, Vec2i origin)
	{
		const auto size = map_->getSize();

		for (int dx = -kVisionRadius; dx <= kVisionRadius; ++dx) {
			for (int dy = -kVisionRadius; dy <= kVisionRadius; ++dy) {
				const int x = origin.x + dx;
				const int y = origin.y + dy;
				if (x < 0 || y < 0 || x >= size.x || y >= size.y) {
					continue;
				}

				Tile *tile = map_->getTile({x, y});
				if (tile && tile->getType() == Tile::BasicTypes::WALL) {
					client->send(bmsg::SV_ghost_wall{x, y});
				}
			}
		}
	}
};
