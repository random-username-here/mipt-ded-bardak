#pragma once

#include "BmServerModule.hpp"
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

	Level *m_map = nullptr;
	std::unique_ptr<Ghost> m_ghost = nullptr;
	uint64_t m_next_move_tick = 0;
	uint64_t m_next_attack_tick = 0;
	bool m_alive = false;

public:
	GhostCtl() = default;

	GhostCtl(Level *map, BmClient *client)
	    : m_map(map)
	{
		(void)client;
		assert(map);

		const auto sz = m_map->getSize();
		assert(sz.x > 2 && sz.y > 2);

		const Vec2i pos{1 + rand() % (sz.x - 2), 1 + rand() % (sz.y - 2)};

		Tile *tile = map->getTile(pos);
		assert(tile);
		m_ghost = std::make_unique<Ghost>(map, tile);
		m_map->newEntity(m_ghost.get(), tile);
		m_alive = true;

        m_ghost->EvDeath.subscribe([this]() {
            m_alive = false;
        });
	}

	void move(int dx, int dy, uint64_t cur_tick)
	{
		assert(m_ghost);
		assert(m_map);

		if (!m_alive) return;

		if (cur_tick < m_next_move_tick) {
			return;
		}
		if (std::abs(dx) > 1 || std::abs(dy) > 1 || (dx == 0 && dy == 0)) {
			return;
		}

		const Vec2i cur = m_ghost->getPosition();
		const Vec2i new_pos{cur.x + dx, cur.y + dy};
        if (!m_map->isWalkable(new_pos)) {
            return;
        }

		m_ghost->setPosition(new_pos);
		m_next_move_tick = cur_tick + kMoveCdTicks;
	}

	void attack(uint32_t whom, uint64_t cur_tick)
	{
		assert(m_ghost);
		assert(m_map);

		if (!m_alive) {
			return;
		}

		if (cur_tick < m_next_attack_tick) {
			return;
		}

		auto *u = m_map->getEntity(static_cast<modlib::Entity::ID>(whom));
		if (!u) {
			return;
		}

		auto *entity = dynamic_cast<EC::Stats::Health *>(u);
		if (!entity || entity->getCurrentHP() <= 0) {
			return;
		}

		if (std::abs(u->getPosition().x - m_ghost->getPosition().x) > 1 ||
		    std::abs(u->getPosition().y - m_ghost->getPosition().y) > 1) {
			return;
		}

		entity->inflictDmg(static_cast<size_t>(kAttackDamage));
		m_ghost->EvAttack.emit(u->getID());

		m_next_attack_tick = cur_tick + kAttackCdTicks;
	}

	void destroy()
	{
		assert(m_ghost);
		m_alive = false;
		if (m_ghost->getTile()) {
			m_map->removeEntity(m_ghost->getID());
		}
	}

	bool alive() const
	{
		return m_alive;
	}

	Vec2i pos() const
	{
		assert(m_ghost);
		return m_ghost->getPosition();
	}

	int32_t hp() const
	{
		assert(m_ghost);
		return static_cast<int32_t>(m_ghost->getCurrentHP());
	}

	Ghost *ghost()
	{
		return m_ghost.get();
	}

	Level *map()
	{
		return m_map;
	}
};
