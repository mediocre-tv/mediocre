#include <csignal>
#include <cstdlib>
#include <mediocre/main/main.hpp>
#include <mediocre/server/server.hpp>
#include <thread>
#include <atomic>

static auto server = mediocre::server::Server(8081);

std::atomic<bool> shutdown_requested = false;

void shutdown() {
    server.shutdown_server();
}

void signal_handler(const int signal_num) {
    shutdown_requested = true;
}

int main(int argc, char *argv[]) {
    signal(SIGTERM, signal_handler);

    std::thread server_thread([] {
        server.run_server();
    });

    while (!shutdown_requested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    shutdown();

    server_thread.join();

    return EXIT_SUCCESS;
}
