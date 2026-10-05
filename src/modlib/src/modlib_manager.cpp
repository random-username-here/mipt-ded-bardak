#include "modlib_manager.hpp"
#include "aixlog.hpp"
#include <string>
#include <dlfcn.h>
#include <dirent.h>
#include <string.h>
#include <iostream>

//#define TAG "modlib"
//using namespace misc::color;

typedef Mod *(*ModCreateFn)(ModManager *mm);

ModManager::~ModManager() {
    for (auto &i : m_mods)
        i->onBeforeCleanup(this);
}

Mod *ModManager::loadFromFile(std::filesystem::path file) {
    //misc::info(TAG) << "Loading mod " << ACCENT << file << RST;
    void *so = dlopen(file.c_str(), RTLD_LAZY);
    if (so == nullptr)
        throw Error(dlerror());

    ModCreateFn modFn = reinterpret_cast<ModCreateFn>(dlsym(so, "modlib_create"));
    if (modFn == nullptr) {
        dlclose(so);
        LOG(ERROR, "modlib") << "Failed to get `modlib_create()` function from file " << file;
        return nullptr;
    }

    Mod *mod = modFn(this);
    m_soHandles.push_back(std::unique_ptr<void, int(*)(void*)>(so, dlclose));
    m_mods.push_back(std::unique_ptr<Mod>(mod));
    return mod;
}

void ModManager::loadAllFrom(std::filesystem::path path) {
    if (std::filesystem::is_directory(path)) {
        for (auto i : std::filesystem::directory_iterator(path))
            loadAllFrom(i);
    } else if (std::filesystem::is_regular_file(path)) {
        if (path.extension() != ".mod" && path.extension() != ".so")
            return;
        loadFromFile(path);
    }
}

void ModManager::initLoaded() {
    for (auto &i : m_mods)
        i->onResolveDeps(this);
    for (auto &i : m_mods)
        i->onDepsResolved(this);
}
