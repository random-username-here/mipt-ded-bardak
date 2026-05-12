#pragma once

#include "pacman.hpp"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <memory>

class PacmanCtl
{
	static constexpr uint64_t kMoveCdTicks = 1;

	Level *map_ = nullptr;
	std::unique_ptr<Pacman> pacman_ = nullptr;
	uint64_t m_nextMoveTick = 0;

public:
	PacmanCtl() = default;

	PacmanCtl(Level *map, BmClient *client, uint64_t team_id)
	    : map_(map)
	{
		(void)client;
		assert(map);

		const auto sz = map_->getSize();
		assert(sz.x > 2 && sz.y > 2);

		const Vec2i pos{1 + rand() % (sz.x - 2), 1 + rand() % (sz.y - 2)};

		Tile *tile = map->getTile(pos);
		assert(tile);
		pacman_ = std::make_unique<Pacman>(map, tile, team_id);
		map_->newEntity(pacman_.get(), tile);
	}

	void move(int dx, int dy, uint64_t cur_tick)
	{
		assert(pacman_);
		assert(map_);

		if (cur_tick < m_nextMoveTick) {
			return;
		}
		if (std::abs(dx) > 1 || std::abs(dy) > 1 || (dx == 0 && dy == 0)) {
			return;
		}

		const Vec2i cur = pacman_->getPosition();
		const Vec2i new_pos{cur.x + dx, cur.y + dy};
        if (!map_->isWalkable(new_pos)) {
            return;
        }

		pacman_->setPosition(new_pos);
		m_nextMoveTick = cur_tick + kMoveCdTicks;
	}

	void destroy()
	{
		assert(pacman_);
		map_->removeEntity(pacman_->getID());
	}

	Vec2i pos() const
	{
		assert(pacman_);
		return pacman_->getPosition();
	}

	int32_t hp() const
	{
		assert(pacman_);
		return static_cast<int32_t>(pacman_->getCurrentHP());
	}

	Pacman *pacman()
	{
		return pacman_.get();
	}

	Level *map()
	{
		return map_;
	}
};
