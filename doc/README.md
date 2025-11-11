# networking

C/C++ networking examples for educational purposes.

TBD.

Here is a stripped version of the C++ `echoserver` program using
`echoserver.hh`. The number of max connections and the port are hardcoded.

```cpp
#include <cstdlib>
#include <iostream>
#include <thread>

#include <pdnnet/echoserver.hh>
#include <pdnnet/process.hh>
#include <pdnnet/server.hh>

int main()
{
  // create new server
  pdnnet::echoserver server;
  // start server in new thread with given parameters. we need to do this so we
  // can print the state of the running server from the current thread
  std::thread server_thread{
    [&server]
    {
      auto params = pdnnet::server_params{}.port(8888).max_pending(16);
      server.start(params);
    }
  };
  // wait until the server is running to prevent undefined member access
  while (!server.running());
  // on WSL Ubuntu 22.04.2 LTS we have to flush stdout otherwise the terminal
  // does not properly print on a new line + then display the new prompt
#ifndef _WIN32
  std::cout << std::flush;
#endif  // _WIN32
  // print address and port for debugging
  std::cout << pdnnet::getpid() << "max_threads=" << server.max_threads() <<
    ", address=" << server.dot_address() << ":" << server.port() << std::endl;
  // join + return (unreachable)
  server_thread.join();
  return EXIT_SUCCESS;
}
```
