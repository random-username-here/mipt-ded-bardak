#include "MSVA.hpp"
#include "libpan.h"
#include "modlib_manager.hpp"
#include "ini.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdarg.h>
#include "aixlog.hpp"

struct Refs {
    ModManager *mm;
    PAN *pan;
    msva::MsvaServer *server;
    std::filesystem::path configPath;
};

void l_addPans(std::filesystem::path p, PAN *pan, bool toplevel = true) {
    if (std::filesystem::is_directory(p)) {
        for (auto i : std::filesystem::directory_iterator(p))
            l_addPans(i, pan, false);
    } else if (std::filesystem::is_regular_file(p)) {
        if (p.extension() == ".pan") {
            LOG(DEBUG, "msva/main") << "Loading protocol definition " << p << "\n";
            pan_loadDefsFromFile(pan, p.c_str());
        }
    } else if (toplevel) {
        LOG(WARNING, "msva/main") << "File " << p << " is not a protocl definition\n";
    }
}

int l_iniHandler(void *p, const char *sec_c, const char *name_c, const char *val_c) {
    Refs *r = (Refs*) p;
    std::string_view sec(sec_c), name(name_c), val(val_c);
    if (sec != "") {
        LOG(WARNING, "msva/main") << "Sections in the config file aren't handled yet\n";
        return 0;
    }

    if (name == "server_name") {
        r->server->setName(val);
    } else if (name == "mods_path") {
        auto path = std::filesystem::absolute(r->configPath.parent_path() / val).lexically_normal();
        LOG(INFO, "msva/main") << "Adding module path " << path << "\n";
        r->mm->loadAllFrom(path.c_str());
    } else if (name == "pan_path") {
        auto path = std::filesystem::absolute(r->configPath.parent_path() / val).lexically_normal();
        LOG(INFO, "msva/main") << "Adding pan path " << path << "\n";
        l_addPans(path, r->pan, true);
    } else if (name == "port") {
        r->server->setPort(std::atoi(val_c));
    } else if (name == "tick_time") {
        r->server->setTickTime(std::atoi(val_c));
    } else if (name == "log_file") {
        auto path = std::filesystem::absolute(r->configPath.parent_path() / val).lexically_normal();
        AixLog::Log::instance().add_logsink<AixLog::SinkFile>(AixLog::Severity::trace, path);
    }
    r->server->setConfigValue(name, val);
    return 0;
}

static ModManager mgr;
static PAN pan;
static msva::MsvaServer srv(&mgr, &pan);

static void l_panLogger(void *data, const char *fmt, ...) {
    va_list args, copy;
    va_start(args, fmt);
    va_copy(copy, args);
    std::string res;
    res.resize(vsnprintf(nullptr, 0, fmt, copy) + 1);
    vsnprintf(res.data(), res.size(), fmt, args);
    va_end(args);
    if (data)
        LOG(TRACE, (const char*) data) << res << "\n";
    else
        LOG(INFO, "pan") << res << "\n";
}

int main(int argc, char *argv[]) {
   
    AixLog::Log::init<AixLog::SinkCallback>(
        AixLog::Severity::trace, 
        [](const AixLog::Metadata &md, const std::string &msg){
            std::cout 
                << AixLog::Color::BLUE << md.timestamp.to_string() << "  ";
            size_t h = (std::hash<std::string>{}(md.tag.text)) % 7 + 2;
            std::cout << AixLog::Color(h) << std::left << std::setw(30) << md.tag.text << AixLog::Color::NONE << "  ";
            switch (md.severity) {
                case AixLog::Severity::trace: std::cout << AixLog::Color::CYAN; break;
                case AixLog::Severity::debug: std::cout << AixLog::Color::CYAN; break;
                case AixLog::Severity::info: std::cout << AixLog::Color::GREEN; break;
                case AixLog::Severity::notice: std::cout << AixLog::Color::BLUE; break;
                case AixLog::Severity::warning: std::cout << AixLog::Color::YELLOW; break;
                case AixLog::Severity::error: std::cout << AixLog::Color::RED; break;
                case AixLog::Severity::fatal: std::cout << AixLog::Color::MAGENTA; break;
            }
            std::cout << std::setw(8) << std::right << AixLog::to_string(md.severity) << AixLog::Color::NONE;
            std::cout << "  " << msg << '\n';
        }
    );

    if (argc != 2) {
        LOG(FATAL, "msva/main") << "Config file not specified (the only argument)\n";
        return 1;
    }

    pan.userptr = nullptr;
    pan_init(&pan, l_panLogger, true);
    
    LOG(NOTICE, "msva/main") << "Starting server with config " << argv[1] << "\n";

    Refs r { &mgr, &pan, &srv, std::filesystem::path(argv[1]) };
    ini_parse(argv[1], l_iniHandler, &r);
    LOG(INFO, "msva/main") << "Config parsed\n";

    for (auto &i : mgr.all())
        LOG(INFO, "msva/main") << "Loaded mod " << i->id() << " "
            << (int) i->version().major << "." << (int) i->version().minor << "." << i->version().patch
            << " -- " << i->brief() << '\n';

    mgr.initLoaded();
    srv.initMods();
    LOG(NOTICE, "msva/main") << "Setup done, starting main loop!\n";
    srv.mainloop();

    return 0;
}
