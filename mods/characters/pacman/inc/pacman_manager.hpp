#pragma once

#include "pacman_animator.hpp"
#include "pacman_controller.hpp"
#include "BmServerModule.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "modlib_mod.hpp"
#include "modlib_manager.hpp"
#include "pacman_proto.hpp"

#include <iostream>
#include <unordered_map>

namespace {

constexpr int kVisionRadius = 4;
constexpr uint64_t kPacmanTeam = 0;
constexpr uint32_t kGhostTeamProtocol = 1;

inline uint32_t protocolTeamId(modlib::Entity *e)
{
	if (e == nullptr) {
		return 0;
	}
	if (auto *p = dynamic_cast<Pacman *>(e)) {
		return static_cast<uint32_t>(p->teamId());
	}
	if (e->getType() == modlib::Entity::Type("ghost")) {
		return kGhostTeamProtocol;
	}
	return 0;
}

inline bool isAliveEntityForPacman(modlib::Entity *e)
{
	if (auto *health = dynamic_cast<EC::Stats::Health *>(e)) {
		return health->getCurrentHP() > 0;
	}
	return true;
}

} // namespace

class PacmanManager {
	Timer *timer_ = nullptr;
	Level *map_ = nullptr;
	anim::AnimationManager *animator_ = nullptr;
	modlib::AssetManager *assets_ = nullptr;

	struct PacmanUtils {
		PacmanCtl ctl;
		PacmanAnimator anim;

		PacmanUtils(Level *map, BmClient *client, anim::AnimationManager *animator,
		            modlib::AssetManager *assets, uint64_t team_id)
		    : ctl(map, client, team_id)
		    , anim(&ctl, animator, assets)
		{}
	};

	std::unordered_map<BmClient *, PacmanUtils> pacmen_;
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
		const auto it = pacmen_.find(client);
		if (it != pacmen_.end()) {
			it->second.ctl.destroy();
			pacmen_.erase(it);
			client->send(bmsg::SV_pacman_hp{0});
		}
	}

	void resolve()
	{
		timer_->setTimer(1, [this]() { sendPeriodicState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
	}

	void receiveMoveCommand(BmClient *client, bmsg::CL_pacman_move move_cmd)
	{
		const auto it = pacmen_.find(client);
		if (it == pacmen_.end()) {
			return;
		}

		it->second.ctl.move(move_cmd.dx, move_cmd.dy, m_tick);
	}

	void receiveWhereCommand(BmClient *client, bmsg::CL_pacman_where where_cmd)
	{
		sendWhereFor(client, where_cmd.teamId);
	}

	void receiveSeesCommand(BmClient *client)
	{
		sendVisionFor(client);
	}

	size_t count(BmClient *client) const
	{
		return pacmen_.count(client);
	}

	void spawnPacman(BmClient *client)
	{
		if (pacmen_.count(client)) {
			std::cerr << "pacman with client `" << client->id() << "` was already spawned\n";
			return;
		}

		pacmen_.try_emplace(client, map_, client, animator_, assets_, kPacmanTeam);
	}

  private:
	void sendPeriodicState()
	{
		++m_tick;

		for (auto &[cl, ps] : pacmen_) {
			cl->send(bmsg::SV_pacman_hp{ps.ctl.hp()});
			if (!ps.ctl.alive()) {
				ps.ctl.destroy();
				continue;
			}

			const Vec2i ppos = ps.ctl.pos();
			cl->send(bmsg::SV_pacman_tick{});
			cl->send(bmsg::SV_pacman_at{ppos.x, ppos.y});
		}

		timer_->setTimer(1, [this]() { sendPeriodicState(); }, modlib::Timer::Stage::ON_UPDATE_DONE);
	}

	void sendWhereFor(BmClient *client, uint32_t team_id)
	{
		for (const auto &[_, ent] : map_->getEntityList()) {
			if (!isAliveEntityForPacman(ent)) {
				continue;
			}
			const uint32_t tid = protocolTeamId(ent);
			if (tid != team_id) {
				continue;
			}
			const Vec2i p = ent->getPosition();
			client->send(bmsg::SV_pacman_where{p.x, p.y, static_cast<uint32_t>(ent->getID()), tid});
		}
	}

	void sendVisionFor(BmClient *client)
	{
		const auto it = pacmen_.find(client);
		if (it == pacmen_.end()) {
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
					client->send(bmsg::SV_pacman_wall{x, y});
				}

				for (const auto &[eid, entity] : tile->getEntityList()) {
					(void)eid;
					if (entity != it->second.ctl.pacman() && isAliveEntityForPacman(entity)) {
						client->send(bmsg::SV_pacman_sees{x, y, static_cast<uint32_t>(entity->getID()),
						                                  protocolTeamId(entity)});
					}
				}
			}
		}
	}
};
