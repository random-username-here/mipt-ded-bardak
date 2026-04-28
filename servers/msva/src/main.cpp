#include "./all.hpp"
#include "libpan.h"
#include "modlib_manager.hpp"
#include "ini.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>

struct Refs {
    ModManager *mm;
    PAN *pan;
    msva::Server *server;
    std::filesystem::path configPath;
};

std::ostream &l_logPrefix() {
    return std::cerr << ESC_GRY << "server ... : " << ESC_RST;
}

void l_addPans(std::filesystem::path p, PAN *pan, bool toplevel = true) {
    if (std::filesystem::is_directory(p)) {
        for (auto i : std::filesystem::directory_iterator(p))
            l_addPans(i, pan, false);
    } else if (std::filesystem::is_regular_file(p)) {
        if (p.extension() == ".pan") {
            l_logPrefix() << "Loading protocol definition " << ESC_BLU << p.c_str() << ESC_RST << '\n';
            pan_loadDefsFromFile(pan, p.c_str());
        }
    } else if (toplevel) {
        l_logPrefix() << ESC_YLW << "Warning: file " << p.c_str() << " is not a protocol defenition!\n" << ESC_RST;
    }
}

int l_iniHandler(void *p, const char *sec_c, const char *name_c, const char *val_c) {
    Refs *r = (Refs*) p;
    std::string_view sec(sec_c), name(name_c), val(val_c);
    if (sec != "") {
        l_logPrefix() << ESC_YLW << "Did not expect sections in config file, found section [" << sec << "]\n" << ESC_RST;
        return 0;
    }

    if (name == "server_name") {
        r->server->setName(val);
    } else if (name == "mods_path") {
        auto path = std::filesystem::absolute(r->configPath.parent_path() / val).lexically_normal();
        l_logPrefix() << "Adding mod path " << ESC_BLU << path.c_str() << ESC_RST << '\n';
        r->mm->loadAllFromDir(path.c_str());
    } else if (name == "pan_path") {
        auto path = std::filesystem::absolute(r->configPath.parent_path() / val).lexically_normal();
        l_logPrefix() << "Adding pan path " << ESC_BLU << path.c_str() << ESC_RST << '\n';
        l_addPans(path, r->pan, true);
    } else if (name == "port") {
        r->server->setPort(std::atoi(val_c));
    } else if (name == "tick_time") {
        r->server->setTickTime(std::atoi(val_c));
    } else {
        l_logPrefix() << ESC_YLW << "Key " << name << " is not known\n" << ESC_RST;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    
    if (argc != 2) {
        l_logPrefix() << "Error: Config file name expected!\n";
        return 1;
    }

    ModManager mgr;
    PAN pan;
    pan_init(&pan, nullptr, true);
    msva::Server srv(&mgr, &pan);
    

    l_logPrefix() << "Starting server with config from " << ESC_BLU << argv[1] << ESC_RST << '\n';

    Refs r { &mgr, &pan, &srv, std::filesystem::path(argv[1]) };
    ini_parse(argv[1], l_iniHandler, &r);
    l_logPrefix() << "Config loaded\n";

    for (auto &i : mgr.all())
        l_logPrefix() << "Loaded mod " << ESC_BLU << i->id() << " " << ESC_YLW 
            << (int) i->version().major << "." << (int) i->version().minor << "." << i->version().patch
            << ESC_RST << " -- " << i->brief() << '\n';
    mgr.initLoaded();
    srv.initMods();
    l_logPrefix() << "Setup done, main loop begins!\n";
    l_logPrefix() << ESC_GRY << "----------------------------------\n" << ESC_RST;
    srv.mainloop();

    return 0;
}
