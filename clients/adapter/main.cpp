#include "brain_bridge.hpp"
#include "brain_protocol.hpp"
#include "knight_script_adapter.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include "ini.h"

struct AdapterConfig
{
    std::string brain_host = "127.0.0.1";
    std::string brain_port = "17771";
};

static int iniHandler(void *user, const char *section, const char *name, const char *value)
{
    auto *config = static_cast<AdapterConfig *>(user);

    if (section != nullptr && std::string_view(section) != "brain") {
        return 1;
    }

    if (std::string_view(name) == "host") {
        config->brain_host = value;
    } else if (std::string_view(name) == "port") {
        config->brain_port = value;
    }

    return 1;
}

static bool loadBrainConfig(std::string_view ini_path, AdapterConfig &config)
{
    const int result = ini_parse(ini_path.data(), iniHandler, &config);
    if (result < 0) {
        std::cerr << "cannot open ini: " << ini_path << '\n';
        return false;
    }
    if (result > 0) {
        std::cerr << "ini parse error at line " << result << '\n';
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " BRAIN_EXE CLIENT_INI\n";
        std::cerr << "  BRAIN_EXE   - comp-lang executable, or \"-\" if you start the brain yourself\n";
        std::cerr << "  CLIENT_INI  - game client ini (host/port/pan_dir), e.g. clients/adapter/default.ini\n";
        return EXIT_FAILURE;
    }

    AdapterConfig brain_cfg;
    brain_cfg.brain_port = std::to_string(brain_proto::kDefaultPort);
    if (!loadBrainConfig("clients/adapter/brain.ini", brain_cfg)) {
        loadBrainConfig("brain.ini", brain_cfg);
    }

    try {
        BrainBridge bridge(brain_cfg.brain_host, brain_cfg.brain_port);
        if (!bridge.listen()) {
            return EXIT_FAILURE;
        }

        const bool manual_brain = std::string_view(argv[1]) == "-";
        if (!manual_brain) {
            if (!bridge.launchScript(argv[1])) {
                return EXIT_FAILURE;
            }
        } else {
            std::cout << "[adapter] waiting for brain on " << brain_cfg.brain_host << ':'
                      << brain_cfg.brain_port << " (start knight_brain.exe manually)\n";
        }

        if (!bridge.waitForScript(30000)) {
            std::cerr << "brain script did not connect to " << brain_cfg.brain_host << ':'
                      << brain_cfg.brain_port << '\n';
            return EXIT_FAILURE;
        }

        std::cout << "[adapter] brain connected on " << brain_cfg.brain_host << ':'
                  << brain_cfg.brain_port << '\n';

        KnightScriptAdapter client(argv[2], argv[1], bridge);
        return client.run() ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const std::exception &err) {
        std::cerr << err.what() << '\n';
        return EXIT_FAILURE;
    }
}
