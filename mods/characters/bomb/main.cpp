#include "bomb.hpp"

extern "C" Mod *modlib_create(ModManager *)
{
    return new bombs::BombsModule();
}
