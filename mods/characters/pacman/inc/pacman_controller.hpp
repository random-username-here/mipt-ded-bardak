#pragma once

#include "BmServerModule.hpp"
#include "pacman.hpp"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <memory>

class PacmanCtl
{
	static constexpr uint64_t kMoveCdTicks = 1;

	Level *m_map = nullptr;
	std::unique_ptr<Pacman> m_pacman = nullptr;
	uint64_t m_nextMoveTick = 0;
	bool m_alive = false;

public:
	PacmanCtl() = default;

	PacmanCtl(Level *map, BmClient *client, uint64_t team_id)
	    : m_map(map)
	{
		(void)client;
		assert(map);

		const auto sz = m_map->getSize();
		assert(sz.x > 2 && sz.y > 2);

		const Vec2i pos{1 + rand() % (sz.x - 2), 1 + rand() % (sz.y - 2)};

		Tile *tile = map->getTile(pos);
		assert(tile);
		m_pacman = std::make_unique<Pacman>(map, tile, team_id);
		m_map->newEntity(m_pacman.get(), tile);
		m_alive = true;

        m_pacman->EvDeath.subscribe([this]() {
            m_alive = false;
        });
	}

	void move(int dx, int dy, uint64_t cur_tick)
	{
		assert(m_pacman);
		assert(m_map);

		if (!m_alive) return;

		if (cur_tick < m_nextMoveTick) {
			return;
		}
		if (std::abs(dx) > 1 || std::abs(dy) > 1 || (dx == 0 && dy == 0)) {
			return;
		}

		const Vec2i cur = m_pacman->getPosition();
		const Vec2i new_pos{cur.x + dx, cur.y + dy};
		Tile *next = m_map->getTile(new_pos);
		if (!next || next->getType() == Tile::BasicTypes::WALL) {
			return;
		}

		m_pacman->setPosition(new_pos);
		m_nextMoveTick = cur_tick + kMoveCdTicks;
	}

	void destroy()
	{
		assert(m_pacman);
		m_alive = false;
		if (m_pacman->getTile()) {
			m_map->removeEntity(m_pacman->getID());
		}
	}

	bool alive() const
	{
		return m_alive;
	}

	Vec2i pos() const
	{
		assert(m_pacman);
		return m_pacman->getPosition();
	}

	int32_t hp() const
	{
		assert(m_pacman);
		return static_cast<int32_t>(m_pacman->getCurrentHP());
	}

	Pacman *pacman()
	{
		return m_pacman.get();
	}

	Level *map()
	{
		return m_map;
	}
};
