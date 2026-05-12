#pragma once

#include "binmsg.hpp"

namespace EC
{
    namespace Common
    {
        struct Damage
        {
            using DMG = size_t;
            using Type = bmsg::Char64;
            
            static constexpr Type PURE  {"PURE"};
            static constexpr Type PHYS  {"PHYS"};
            static constexpr Type MAGIC {"MAGIC"};

            Type m_type;
            DMG  m_damage;

            Damage (Type type, DMG damage);
        };
    }
}