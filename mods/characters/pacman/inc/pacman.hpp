#pragma once

#include "Map.hpp"

using namespace modlib;

class Pacman : virtual public modlib::Entity, virtual public EC::Stats::Health
{
public:
	static constexpr size_t kMaxHp = 100;
	static constexpr size_t kStartHp = 100;
	static constexpr Type PACMAN_TYPE = "pacman";

private:
	uint64_t team_id_ = 0;

public:
	Pacman(Level * /*map*/, Tile *tile, uint64_t team_id)
	    : Entity(PACMAN_TYPE, tile)
	    , Health(kStartHp, kMaxHp)
	    , team_id_(team_id)
	{
	}

	uint64_t teamId() const
	{
		return team_id_;
	}
};
