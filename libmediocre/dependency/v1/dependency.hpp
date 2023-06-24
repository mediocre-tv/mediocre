#ifndef dependency_h
#define dependency_h

#include <libmediocre/dependency/v1/dependency.grpc.pb.h>

namespace mediocre::dependency::v1 {

    using grpc::Status;
    using grpc::ServerContext;

    class DependencyServiceImpl final : public Dependency::Service {
        Status CheckOpenCV(
                ServerContext *context,
                const OpenCVCheckRequest *request,
                OpenCVCheckResponse *response) override;
        Status CheckTesseract(
                ServerContext *context,
                const TesseractCheckRequest *request,
                TesseractCheckResponse *response) override;
    };

}// namespace mediocre::dependency::v1

#endif// dependency_h