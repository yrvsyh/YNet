#include "WebServer.hpp"

#include <spdlog/spdlog.h>

int main() {
    spdlog::set_level(spdlog::level::info);
    EventLoop loop;
    WebServer server(&loop, "127.0.0.1", 8080);
    server.setPrefix("../");
    server.setHomePage("index.html");
    server.start(8, 65535);
    loop.addTimer({5, 0, 5}, [] { spdlog::info("5s timeout"); });
    loop.addSignal(SIGINT);
    loop.onSignal([&](int signo) {
        if (signo == SIGINT) {
            printf("\r");
            spdlog::info("quiting");
            loop.quit();
        }
    });
    loop.loop();
    return 0;
}
