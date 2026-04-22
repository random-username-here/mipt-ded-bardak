#include "./all.hpp"
#include "libpan.h"
#include "modlib_manager.hpp"
#include "ini.h"
#include <cstdlib>
#include <iostream>

struct Refs {
    ModManager *mm;
    PAN *pan;
    msva::Server *server;
};

int l_iniHandler(void *p, const char *sec_c, const char *name_c, const char *val_c) {
    Refs *r = (Refs*) p;
    std::string_view sec(sec_c), name(name_c), val(val_c);
    if (sec != "") return 0;

    if (name == "server_name") {
        r->server->setName(val);
    } else if (name == "mods_dir") {
        r->mm->loadAllFromDir(val);
    } else if (name == "pan_dir") {
        pan_loadDefsFromFile(r->pan, val_c); // TODO: directory walking
    } else if (name == "port") {
        r->server->setPort(std::atoi(val_c));
    }
    return 0;
}

int main(int argc, char *argv[]) {
    
    ModManager mgr;
    PAN pan;
    pan_init(&pan, nullptr, true);
    msva::Server srv(&mgr, &pan);
    
    if (argc != 2) {
        std::cerr << "Config file name expected!\n";
        return 1;
    }

    Refs r { &mgr, &pan, &srv };
    ini_parse(argv[1], l_iniHandler, &r);

    mgr.initLoaded();
    srv.initMods();
    srv.mainloop();

    return 0;
}
