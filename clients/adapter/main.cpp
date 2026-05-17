#include "client-core/tcp/tcp_connection.hpp"
#include "proxy/proxy.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <atomic>
#include <unistd.h>



int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0] << " <program> <host> <port>\n";
        return EXIT_FAILURE;
    }

    try 
    {
        TcpConnection conn (
            "localhost",
            "3000"
        );
        Proxy         proxy (
            argv[1]
        );
        
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