#pragma once

#include "BmServerModule.hpp"
#include "person.hpp"

inline RotationDir convertMoveToDir(int dx, int dy) {
    if (dx == 0 && dy == 0) return RotationDir::down;

    if (std::abs(dx) > std::abs(dy)) {
        return (dx > 0) ? RotationDir::right: RotationDir::left;
    } else {
        return (dy > 0) ? RotationDir::down : RotationDir::up;
    }
};

// Rules
class PersonCtl {
    static constexpr uint64_t kMoveCdTicks = 1;
    static constexpr uint64_t kAttackCdTicks = 2;
    static constexpr int kBaseAttackDamage = 10;
    static constexpr int kBerserkBonusDamage = 6;
    static constexpr int kLowHpThreshold = 25;

    Level *m_map=nullptr;
    std::unique_ptr<Person> m_person=nullptr;
    uint64_t m_nextMoveTick = 0;
    uint64_t m_nextAttackTick = 0;
    bool m_actionDone = false;
	bool m_alive = false;

public:
    PersonCtl() = default;

    PersonCtl(Level *map, BmClient *client):
        m_map(map)
    {
        assert(map);

        auto sz = m_map->getSize();
        assert(sz.x > 2 && sz.y > 2);

        Vec2i pos = Vec2i
        {
            1 + rand() % (sz.x - 2),
            1 + rand() % (sz.y - 2)
        };

        Tile *tile = map->getTile(pos);
        m_person = std::make_unique<Person>(map, tile, client);
        m_map->newEntity(m_person.get(), tile);
		m_alive = true;

        m_person->EvDeath.subscribe([this]() {
            m_alive = false;
        });	
    }

    void move(int dx, int dy, uint64_t curTick) {
        assert(m_person);
        assert(m_map);

		if (!m_alive) return;

        if (curTick < m_nextMoveTick) return;
        if (abs(dx) > 1 || abs(dy) > 1) return;

        Vec2i newPos = {m_person->getPosition().x + dx, m_person->getPosition().y + dy};
        if (m_map->getTile(newPos)->getType() == Tile::BasicTypes::WALL) return;

        // m_person->rotate(convertMoveToDir(dx, dy));
        m_person->setPosition(newPos);
        m_nextMoveTick = curTick + kMoveCdTicks;
    }

    void attack(size_t whom, uint64_t curTick) {
        assert(m_person);
        assert(m_map);

		if (!m_alive) return;

        if (curTick < m_nextAttackTick) return;
        auto u = m_map->getEntity(whom);
        if (!u) return;

        auto *entity = dynamic_cast<EC::Stats::Health *>(u);
        if (!entity || entity->getCurrentHP() <= 0) return;

        if (std::abs(u->getPosition().x - m_person->getPosition().x) > 1 || std::abs(u->getPosition().y - m_person->getPosition().y) > 1)
            return;

        int dmg = kBaseAttackDamage;
        if (m_person->getCurrentHP() <= kLowHpThreshold) dmg += kBerserkBonusDamage;
        entity->inflictDmg(dmg);
        m_person.get()->EvAttack.emit(u->getID());

        m_nextAttackTick = curTick + kAttackCdTicks;
    }

    void setActionDoneState(bool flag) {
        m_actionDone = flag;
    }

    void destroy() { // FIXME!!! what if m_person is not destructed, but map deletes person
        assert(m_person);
        m_alive = false;
        if (m_person->getTile()) {
            m_map->removeEntity(m_person->getID());
        }
    }

    bool alive() const {
        return m_alive;
    }

    Vec2i pos() const {
        assert(m_person);
        return m_person->getPosition();
    }
    int32_t hp() const {
        assert(m_person);
        return m_person->getCurrentHP();
    }
    Person *person() {
        return m_person.get();
    }

    Level *map() {
        return m_map;
    }
};
