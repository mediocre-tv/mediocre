#include "mediocre/image/colour/v1beta/colour.hpp"
#include "mediocre/image/invert/v1beta/invert.hpp"
#include "mediocre/image/rotate/v1beta/rotate.hpp"
#include "mediocre/image/threshold/v1beta/threshold.hpp"
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <mediocre/dependency/v1/dependency.hpp>
#include <mediocre/image/crop/v1beta/crop.hpp>
#include <mediocre/image/identity/v1beta/identity.hpp>
#include <mediocre/image/ocr/v1beta/ocr.hpp>
#include <mediocre/server/server.hpp>
#include <mediocre/transform/v1beta/transform.hpp>
#include <mediocre/video/v1beta/video.hpp>

namespace mediocre::server {

    Server::Server(uint16_t port) {
        server_address = get_server_address(port);
    }

    void Server::run_server() {
        std::cout << "Building server on " << server_address << "." << std::endl;

        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();

        grpc::ServerBuilder builder;
        register_listener(builder, server_address);
        register_services(builder);
        set_options(builder);

        server = builder.BuildAndStart();
        std::cout << "Server started." << std::endl;

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
                new dependency::v1::DependencyServiceImpl(),
                new image::ocr::v1beta::OcrServiceImpl(),
                new image::identity::v1beta::IdentityServiceImpl(),
                new image::crop::v1beta::CropServiceImpl(),
                new transform::v1beta::TransformServiceImpl(),
                new image::colour::v1beta::GetColourServiceImpl(),
                new image::threshold::v1beta::ThresholdServiceImpl(),
                new image::rotate::v1beta::RotateServiceImpl(),
                new image::invert::v1beta::InvertServiceImpl(),
                new video::v1beta::VideoServiceImpl(),
        });

        // Register services.
        for (auto service: services) {
            builder.RegisterService(service);
        }

        std::cout << "Registered " << services.size() << " services." << std::endl;
    }

    void Server::set_options(grpc::ServerBuilder &builder) {
        builder.SetMaxReceiveMessageSize(10 * 1024 * 1024);// 10MB
    }

    void Server::shutdown_server() {
        std::cout << "Shutting down server." << std::endl;
        server->Shutdown();
    }

}// namespace mediocre::server
