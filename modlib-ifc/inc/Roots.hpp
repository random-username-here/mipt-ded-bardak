#pragma once

#include "Map.hpp"

namespace modlib {

class RootSystem {
public:
    static constexpr Entity::ID INVALID_ROOT_ID = 0;

    virtual Entity::ID spawnRoot(Vec2i pos, bool animateGrow) = 0;

    virtual ~RootSystem() = default;
};

} // namespace modlib
