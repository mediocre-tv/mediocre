#include <csignal>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <libmediocre/dependency/v1/dependency.hpp>
#include <libmediocre/health/v1/health.hpp>
#include <libmediocre/server/server.hpp>

namespace mediocre::server {

    Server::Server(uint16_t port) {
        server_address = get_server_address(port);
    }

    void Server::run_server() {
        std::cout << "Building server on " << server_address << "." << std::endl;

        grpc::reflection::InitProtoReflectionServerBuilderPlugin();

        grpc::ServerBuilder builder;
        register_listener(builder, server_address);
        register_services(builder);

        server = builder.BuildAndStart();
        std::cout << "Server started." << std::endl;

        // TODO listen to termination signals
        server->Wait();
    }

    std::string Server::get_server_address(uint16_t port) {
        std::ostringstream server_address_stream;
        server_address_stream << "0.0.0.0:" << port;
        return server_address_stream.str();
    }

    void Server::register_listener(grpc::ServerBuilder &builder, const std::string &server_address) {
        // Listen on the given address without any authentication mechanism.
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    }

    void Server::register_services(grpc::ServerBuilder &builder) {
        // Define services.
        std::vector<grpc::Service *> services({
                new grpc::health::v1::HealthServiceImpl(),
                new dependency::v1::DependencyServiceImpl(),
        });

        // Register services.
        for (auto service: services) {
            builder.RegisterService(service);
        }

        std::cout << "Registered " << services.size() << " services." << std::endl;
    }

}// namespace mediocre::server
