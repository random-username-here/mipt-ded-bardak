#include "client-core/tcp/tcp_connection.hpp"
#include "proxy/proxy.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <atomic>
#include <unistd.h>
#include "ini.h"

struct ServerConfig
{
    std::string host;
    std::string port;

    ServerConfig()
    : host("localhost")
    , port("3000")
    {}
};

static
int
iniHandler
(
          void* user,
    const char* section,
    const char* name,
    const char* value
)
{
    ServerConfig* config = (ServerConfig*)user;

    if (
        section == NULL ||
        strlen(section) == 0
    )
    {
        if (
            strcmp(
                name,
                "host"
            ) == 0
        )
        {
            config->host = value;
        }
        else if (
            strcmp(
                name,
                "port"
            ) == 0
        )
        {
            config->port = value;
        }
    }

    return 1;
}

bool iniParse (
    const std::string_view filename,
    ServerConfig& config
)
{
    int result = ini_parse (
        filename.data (),
        iniHandler,
        &config
    );

    if (result < 0) {
        std::cerr << "[Error] Cant open file " << filename << std::endl;
        return false;
    } else if (result > 0) {
        std::cerr << "[Error] Error in string: " << result << std::endl;
        return false;
    }

    return true;
}


int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <program> <config>\n";
        return EXIT_FAILURE;
    }

    ServerConfig config;
    iniParse(
        argv[2],
        config
    );

    try
    {
        TcpConnection conn (
            config.host,
            config.port
        );
        std::cout << "[Log] Connected: host = " << config.host << " port = " << config.port << std::endl;

        Proxy proxy (
            argv[1]
        );
        std::cout << "[Log] Process started: pid = " << proxy.pid () << std::endl;

        std::atomic<bool> running {true};

        std::thread S2Ch (
            [&]()
            {
                while (running)
                {
                    PanFrame frame;
                    auto status = conn.readFrame(frame);

                    if (status == TcpConnection::IoStatus::OK)
                    {
                        proxy.write (
                            frame.payload.data(),
                            frame.payload.size()
                        );
                        proxy.flush ();
                    }
                    else if (status != TcpConnection::IoStatus::TIMEOUT)
                    {
                        running = false;
                        break;
                    }
                }
            }
        );

        std::thread Ch2S (
            [&]()
            {
                std::array<uint8_t, BUFSIZ> buffer;

                while (running)
                {
                    ssize_t read = proxy.read(
                        buffer.data (),
                        buffer.size ()
                    );

                    if (read > 0)
                    {
                        PanFrame frame;
                        frame.payload.assign (
                            buffer.begin(),
                            buffer.begin() + read
                        );

                        if (conn.writeRaw(frame.rawMessage()) != TcpConnection::IoStatus::OK)
                        {
                            running = false;
                            break;
                        }
                    }
                    else if (read <= 0)
                    {
                        running = false;
                        break;
                    }
                }
            }
        );

        S2Ch.join ();
        Ch2S.join ();

    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what () << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
