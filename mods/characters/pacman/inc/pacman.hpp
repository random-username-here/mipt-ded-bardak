#pragma once

#include "Map.hpp"

using namespace modlib;

class Pacman : virtual public modlib::Entity, virtual public EC::Stats::Health
{
public:
	static constexpr size_t kMaxHp = 100;
	static constexpr size_t kStartHp = 100;
	static constexpr Type PACMAN_TYPE = "pacman";

public:
	Pacman(Level * /*map*/, Tile *tile)
	    : Entity(PACMAN_TYPE, tile)
	    , Health(kStartHp, kMaxHp)
	{
	}
};
