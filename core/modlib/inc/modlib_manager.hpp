#pragma once
#include <stdexcept>
#include <string_view>
#include <vector>
#include <memory>
#include "modlib_mod.hpp"

/** 
 * Module manager to load mods, and find them
 */
class ModManager {

    // NOTE: DO NOT REORDER!
    // Otherwise .so handles will be destroyed before plugins,
    // and nonexistent destructors will be called.
    std::vector<std::unique_ptr<void, int (*)(void*)>> m_soHandles;
    std::vector<std::unique_ptr<Mod>> m_mods;

public:

    class Error : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    ModManager() = default;
    ModManager(const ModManager &mm) = delete;
    ModManager &operator=(const ModManager &mm) = delete;
    ~ModManager();

    Mod *loadFromFile(std::string_view file);
    void loadAllFromDir(std::string_view dir);

    void initLoaded();

    Mod *withId(std::string_view id) {
        for (auto &mod : m_mods) {
            if (mod->id() == id) return mod.get();
        }
        return nullptr;
    }

    template<typename Interface>
    Interface *anyOfType() const {
        for (auto &mod : m_mods) {
            Interface *impl = dynamic_cast<Interface*>(mod.get());
            if (impl) return impl;
        }
        return nullptr;
    }

    template<typename Interface>
    std::vector<Interface*> allOfType() const {
        std::vector<Interface*> res;
        for (auto &mod : m_mods) {
            Interface *impl = dynamic_cast<Interface*>(mod.get());
            if (impl) res.push_back(impl);
        }
        return res;
    }
    
    const std::vector<std::unique_ptr<Mod>> &all() const { return m_mods; }

};
