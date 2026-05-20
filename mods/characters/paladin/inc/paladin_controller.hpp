#pragma once

#include "ECbasis.hpp"
#include "Map.hpp"
#include "Timer.hpp"
#include "Vec2.hpp"
#include "paladin.hpp"
#include "combat_grid.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <string_view>

inline
bool
checkPaladinRange (
    modlib::Vec2i from,
    modlib::Vec2i to
)
{
    const int dx = combat_grid::iabs (from.x - to.x);
    const int dy = combat_grid::iabs (from.y - to.y);

    if (dx == 0 && dy == 0)
    {
        return false;
    }

    /*
     * 5x5 mask:
     *
     * 0 1 1 1 0
     * 1 1 1 1 1
     * 1 1 1 1 1
     * 1 1 1 1 1
     * 0 1 1 1 0
     */
    return dx <= 2 && dy <= 2 && 
         !(dx == 2 && dy == 2);
}

class PriestCtrl
{
    static constexpr modlib::Timer::Tick sce_moveCD        = 1;
    static constexpr modlib::Timer::Tick sce_crashCD       = 2;
    static constexpr modlib::Timer::Tick sce_divineSmiteCD = 10;
    static constexpr modlib::Timer::Tick sce_prayCD        = 1;
    static constexpr modlib::Timer::Tick sce_shieldsUpCD   = 10;

    static constexpr EC::Stats::Health::HP sce_crashDMG        = 20;
    static constexpr EC::Stats::Health::HP sce_divivneSmiteDMG = 16;
    static constexpr modlib::Timer::Tick   sce_prayDuration    = 1;
    static constexpr EC::Stats::Health::HP sce_prayHealing     = 5;
    static constexpr EC::Stats::Armor::AP  sce_shieldAp        = 12;
    static constexpr modlib::Timer::Tick   sce_shieldDuration  = 2;

    Level*                  m_map    = nullptr;
    Timer*                  m_timer  = nullptr;

    std::unique_ptr<Priest> m_priest = nullptr;

    struct
    {
        bool m_move        : 1 = true;
        bool m_crash       : 1 = true;
        bool m_divineSmite : 1 = true;
        bool m_pray        : 1 = true;
        bool m_shieldsUp   : 1 = true;
        bool allign        : 3 = 0;
    };

public:
    PriestCtrl () = default;

    PriestCtrl (
        Level* map,
        BmClient* client,
        Timer* timer
    )
    : m_map   (map)
    , m_timer (timer)
    {
        assert (m_map);
        assert (m_timer);

        const Vec2i pos = randomWalkablePosition ();
        Tile* tile = m_map->getTile (pos);
        assert (tile);

        m_priest = std::make_unique<Priest> (
            m_map,
            tile,
            client
        );
        m_map->newEntity (
            m_priest.get (),
            tile
        );
    }

    void
    move (
        int8_t dx,
        int8_t dy
    )
    {
        assert (m_priest);
        assert(m_map);

        if (m_move == false)
        {
            return;
        }

        if (std::abs(dx) + std::abs(dy) != 1)
        {
            return;
        }

        const Vec2i newPos = m_priest->getPosition () + Vec2i (dx, dy);
        if (
            !combat_grid::canEnter (
                m_map,
                newPos,
                m_priest.get ()
            )
        )
        {
            return;
        }

        m_priest->rotate (Delta2Dir (Vec2i (dx, dy)));
        m_priest->setPosition (newPos);
        m_move = false;

        m_timer->setTimer(
            sce_moveCD,
            [this] ()
            {
                this->m_move = true;
            }
        );
    }

    bool
    useAbility (
        std::string_view ability,
        Entity::ID targetId
    )
    {
        assert(m_priest);

        if (!m_priest->inventory ().hasAbility (ability))
        {
            return false;
        }

        if (ability == "crash") {
            return useCrash (targetId);
        }
        if (ability == "divine_smite") {
            return useDivineSmite (targetId);
        }
        if (ability == "pray") {
            return usePray ();
        }
        if (ability == "shieldsup") {
            return useShieldsUp ();
        }

        return false;
    }

    void
    destroy ()
    {
        assert (m_priest);
        m_map->removeEntity (m_priest->getID ());
    }

    Vec2i
    pos ()
    const
    {
        assert (m_priest);
        return m_priest->getPosition ();
    }

    int32_t
    hp ()
    const
    {
        assert(m_priest);
        return static_cast<int32_t> (m_priest->getCurrentHP ());
    }

    Priest*
    paladin ()
    {
        return m_priest.get ();
    }

    Level*
    map ()
    {
        return m_map;
    }

private:
    bool
    useCrash (
        Entity::ID targetID
    )
    {
        assert(m_priest);
        assert(m_map);

        if (m_crash == false)
        {
            return false;
        }

        Entity* target = m_map->getEntity (static_cast<modlib::Entity::ID> (targetID));
        if (
            target == nullptr ||
            target == m_priest.get ()
        )
        {
            return false;
        }

        const Vec2i delta = target->getPosition () - m_priest->getPosition ();
        if (
            !combat_grid::inMooreRange (
                m_priest->getPosition (),
                target->getPosition()
            )
        )
        {
            return false;
        }

        EC::Stats::Health* health = dynamic_cast<EC::Stats::Health*> (target);
        if (
            health == nullptr ||
            health->getCurrentHP () <= 0
        )
        {
            return false;
        }

        m_priest->rotate (Delta2Dir (delta));

        m_priest->EvAttack.emit (targetID);
        health->inflictDmg(sce_crashDMG);

        m_crash = false;
        m_timer->setTimer(
            sce_crashCD,
            [this] ()
            {
                this->m_crash = true;
            }
        );
        return true;
    }

    bool
    usePray ()
    {
        assert(m_priest);

        if (m_pray == false)
        {
            return false;
        }

        if (m_priest->getCurrentHP () >= m_priest->getMaxHP ())
        {
            return false;
        }

        m_timer->setTimer(
            sce_prayDuration,
            [this] ()
            {
                this->m_priest->heal(sce_prayHealing);
            }
        );

        m_pray = false;
        m_timer->setTimer(
            sce_prayCD,
            [this] ()
            {
                this->m_crash = true;
            }
        );
        return true;
    }

    bool
    useDivineSmite (
        Entity::ID targetID
    )
    {
        assert(m_priest);
        assert(m_map);

        if (m_divineSmite) {
            return false;
        }

        Entity* target = m_map->getEntity (static_cast<modlib::Entity::ID> (targetID));
        if (
            target == nullptr ||
            target == m_priest.get()
        )
        {
            return false;
        }

        const Vec2i delta = target->getPosition () - m_priest->getPosition ();
        if (!checkPaladinRange (m_priest->getPosition (), target->getPosition()))
        {
            return false;
        }

        EC::Stats::Health* health = dynamic_cast<EC::Stats::Health*> (target);
        if (
            health == nullptr ||
            health->getCurrentHP() <= 0
        )
        {
            return false;
        }

        m_priest->rotate (Delta2Dir(delta));
        m_priest->EvAttack.emit (targetID);

        health->inflictDmg(sce_divivneSmiteDMG);

        m_divineSmite = false;
        m_timer->setTimer(
            sce_divineSmiteCD,
            [this] ()
            {
                this->m_divineSmite = true;
            }
        );
        return true;
    }

    bool
    useShieldsUp ()
    {
        if (m_shieldsUp == false)
        {
            return false;
        }

        EC::Stats::Armor::AP oldArmor = m_priest->getArmor ();
        m_priest->setArmor (sce_shieldAp);
        
        m_timer->setTimer(
            sce_shieldDuration,
            [this, oldArmor] ()
            {
                this->m_priest->setArmor (oldArmor);
            }
        );

        m_shieldsUp = false;
        m_timer->setTimer(
            sce_shieldsUpCD,
            [this] ()
            {
                m_shieldsUp = true;
            }
        );

        return true;
    }

    Vec2i
    randomWalkablePosition () const
    {
        const Vec2i sz = m_map->getSize();
        assert (sz.x > 0 && sz.y > 0);

        for (int attempt = 0; attempt < 256; ++attempt)
        {
            const Vec2i pos {
                std::rand () % sz.x,
                std::rand () % sz.y
            };
            if (
                combat_grid::canEnter (
                    m_map,
                    pos,
                    m_priest.get ()
                )
            )
            {
                return pos;
            }
        }

        for (int x = 0; x < sz.x; ++x)
        {
            for (int y = 0; y < sz.y; ++y)
            {
                const Vec2i pos{x, y};
                if (
                    combat_grid::canEnter (
                        m_map,
                        pos,
                        m_priest.get ()
                    )
                )
                {
                    return pos;
                }
            }
        }

        return {0, 0};
    }
};
