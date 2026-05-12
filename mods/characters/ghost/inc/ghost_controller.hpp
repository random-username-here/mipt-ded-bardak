#pragma once

#include "ghost.hpp"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <memory>

class GhostCtl
{
	static constexpr uint64_t kMoveCdTicks = 1;
	static constexpr uint64_t kAttackCdTicks = 2;
	static constexpr int kAttackDamage = 25;

	Level *map_ = nullptr;
	std::unique_ptr<Ghost> ghost_ = nullptr;
	uint64_t m_next_move_tick = 0;
	uint64_t m_next_attack_tick = 0;

public:
	GhostCtl() = default;

	GhostCtl(Level *map, BmClient *client, uint64_t team_id)
	    : map_(map)
	{
		(void)client;
		assert(map);

		const auto sz = map_->getSize();
		assert(sz.x > 2 && sz.y > 2);

		const Vec2i pos{1 + rand() % (sz.x - 2), 1 + rand() % (sz.y - 2)};

		Tile *tile = map->getTile(pos);
		assert(tile);
		ghost_ = std::make_unique<Ghost>(map, tile, team_id);
		map_->newEntity(ghost_.get(), tile);
	}

	void move(int dx, int dy, uint64_t cur_tick)
	{
		assert(ghost_);
		assert(map_);

		if (cur_tick < m_next_move_tick) {
			return;
		}
		if (std::abs(dx) > 1 || std::abs(dy) > 1 || (dx == 0 && dy == 0)) {
			return;
		}

		const Vec2i cur = ghost_->getPosition();
		const Vec2i new_pos{cur.x + dx, cur.y + dy};
        if (!map_->isWalkable(new_pos)) {
            return;
        }

		ghost_->setPosition(new_pos);
		m_next_move_tick = cur_tick + kMoveCdTicks;
	}

	void attack(uint32_t whom, uint64_t cur_tick)
	{
		assert(ghost_);
		assert(map_);

		if (cur_tick < m_next_attack_tick) {
			return;
		}

		auto *u = map_->getEntity(static_cast<modlib::Entity::ID>(whom));
		if (!u) {
			return;
		}

		if (std::abs(u->getPosition().x - ghost_->getPosition().x) > 1 ||
		    std::abs(u->getPosition().y - ghost_->getPosition().y) > 1) {
			return;
		}

		if (auto *entity = dynamic_cast<EC::Stats::Health *>(u)) {
			entity->inflictDmg(static_cast<size_t>(kAttackDamage));
			ghost_->EvAttack.emit(u->getID());
		}

		m_next_attack_tick = cur_tick + kAttackCdTicks;
	}

	void destroy()
	{
		assert(ghost_);
		map_->removeEntity(ghost_->getID());
	}

	Vec2i pos() const
	{
		assert(ghost_);
		return ghost_->getPosition();
	}

	int32_t hp() const
	{
		assert(ghost_);
		return static_cast<int32_t>(ghost_->getCurrentHP());
	}

	Ghost *ghost()
	{
		return ghost_.get();
	}

	Level *map()
	{
		return map_;
	}
};
