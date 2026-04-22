#pragma once

#include <cstdint>
#include <string_view>

class ModManager;

/**
 * Semver-style module version
 */
struct ModVersion {
    union {
        struct {
            uint16_t patch;
            uint8_t minor;
            uint8_t major;
        };
        uint32_t integer;
    };

    ModVersion(uint8_t major, uint8_t minor, uint16_t patch)
        :patch(patch), minor(minor), major(major) {}

    ModVersion(uint32_t v)
        :integer(v) {}
};

/** 
 * A COM-style module
 */
class Mod {
public:

    /**
     * Name, unique to this mod. UNIQUE, please, with author's id/repo/project id!
     * Like "isd.bardak.game.map". Not like "Game map"!
     * Also, please configure file name to be like this, but with ".mod" at the end,
     * like "isd.bardak.game.map.mod".
     */
    virtual std::string_view id() const = 0;

    /** Brief module description, for showing to user in mod lists */
    virtual std::string_view brief() const = 0;

    /** Module version, based on semver */
    virtual ModVersion version() const = 0;

    /** Modules were just loaded, find dependencies */
    virtual void onResolveDeps(ModManager *mm) {}

    /** Modules loaded and ready, attach hooks/call other modules */
    virtual void onDepsResolved(ModManager *mm) {}

    /** Application is about to shut down */
    virtual void onBeforeCleanup(ModManager *mm) {}

    virtual ~Mod() {}
};

