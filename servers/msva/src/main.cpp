#include "./all.hpp"
#include "libpan.h"
#include "modlib_manager.hpp"
#include "ini.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdarg.h>

struct Refs {
    ModManager *mm;
    PAN *pan;
    msva::ServerImpl *server;
    std::filesystem::path configPath;
};

std::ostream &l_logPrefix() {
    return std::cerr << ESC_GRY << "server ... : " << ESC_RST;
}

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
    } else if (name == "tick_time") {
        r->server->setTickTime(std::atoi(val_c));
    } else if (name == "log_file") {
        auto path = std::filesystem::absolute(r->configPath.parent_path() / val).lexically_normal();
        r->server->setLogFile(std::string(path.c_str()));
    } else {
        l_logPrefix() << ESC_YLW << "Key " << name << " is not known\n" << ESC_RST;
    }
    return 0;
}

static ModManager mgr;
static PAN pan;
static msva::ServerImpl srv(&mgr, &pan);

static void l_panLogger(void *data, const char *fmt, ...) {
    va_list args, copy;
    va_start(args, fmt);
    va_copy(copy, args);
    std::string res;
    res.resize(vsnprintf(nullptr, 0, fmt, copy) + 1);
    vsnprintf(res.data(), res.size(), fmt, args);
    va_end(args);
    std::ostream *ls = (std::ostream*) data;
    if (ls) (*ls) << res;
}

int main(int argc, char *argv[]) {
    
    if (argc != 2) {
        l_logPrefix() << "Error: Config file name expected!\n";
        return 1;
    }

    pan.userptr = &std::cerr;
    pan_init(&pan, l_panLogger, true);

    Refs r { &mgr, &pan, &srv };
    ini_parse(argv[1], l_iniHandler, &r);

    for (auto &i : mgr.all())
        std::cerr << ESC_GRY << "server ... : " << ESC_RST << "loaded mod " << ESC_BLU << i->id() << ESC_RST << " -- " << i->brief() << '\n';
    mgr.initLoaded();
    srv.initMods();
    srv.mainloop();

    return 0;
}
