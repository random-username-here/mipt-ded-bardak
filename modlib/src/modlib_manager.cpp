#include "modlib_manager.hpp"
//#include "misclib/dump_stream.hpp"
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

Mod *ModManager::loadFromFile(std::string_view file) {
    //misc::info(TAG) << "Loading mod " << ACCENT << file << RST;
    std::string fstr(file);
    void *so = dlopen(fstr.data(), RTLD_LAZY);
    if (so == nullptr)
        throw Error(dlerror());

    ModCreateFn modFn = reinterpret_cast<ModCreateFn>(dlsym(so, "modlib_create"));
    if (modFn == nullptr) {
        dlclose(so);
        std::cerr << "failed to get ModCreateFn from `" << file << "` plugin\n"; 
        /*misc::warn(TAG) << "Mod " << ACCENT << file << RST << " is missing " 
            << ACCENT << "modlib_create()" << RST << " function, skipping";*/
        return nullptr;
    }

    Mod *mod = modFn(this);
    m_soHandles.push_back(std::unique_ptr<void, int(*)(void*)>(so, dlclose));
    m_mods.push_back(std::unique_ptr<Mod>(mod));
    return mod;
}

void ModManager::loadAllFromDir(std::string_view sv_dirpath) {
    std::string dirpath(sv_dirpath);
    std::unique_ptr<DIR, int(*)(DIR*)> dir(opendir(dirpath.data()), closedir);
    if (!dir) {
        throw Error(std::string("Failed to open directory ")
                + dirpath + ": " + strerror(errno));
    }

    dirent *item = nullptr;
    while ((item = readdir(dir.get()))) {
        if (item->d_type == DT_DIR && strcmp(item->d_name, ".") != 0 && strcmp(item->d_name, "..") != 0)
            loadAllFromDir(dirpath + "/" + item->d_name);
        else if (item->d_type == DT_REG) {
            if (strcmp(item->d_name + strlen(item->d_name) - 4, ".mod") != 0)
                continue;
            std::string file = dirpath + '/' + item->d_name;
            loadFromFile(file);
        }
    }
}

void ModManager::initLoaded() {
    for (auto &i : m_mods)
        i->onResolveDeps(this);
    for (auto &i : m_mods)
        i->onDepsResolved(this);
}
