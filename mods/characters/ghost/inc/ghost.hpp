#pragma once

#include "Map.hpp"

using namespace modlib;

class Ghost : virtual public modlib::Entity, virtual public EC::Stats::Health, virtual public EC::Stats::Attack
{
public:
	static constexpr size_t kMaxHp = 100;
	static constexpr size_t kStartHp = 100;
	static constexpr size_t kAttackStrength = 100;
	static constexpr Type GHOST_TYPE = "ghost";

public:
	Ghost(Level * /*map*/, Tile *tile)
	    : Entity(GHOST_TYPE, tile)
	    , Health(kStartHp, kMaxHp)
	    , Attack(kAttackStrength)
	{
	}
};
