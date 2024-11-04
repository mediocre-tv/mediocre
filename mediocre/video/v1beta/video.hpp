#ifndef mediocre_image_video_v1beta_h
#define mediocre_image_video_v1beta_h

#include <mediocre/video/v1beta/video.grpc.pb.h>
#include <opencv2/core/mat.hpp>

namespace mediocre::video::v1beta {

    using grpc::ServerContext;
    using grpc::ServerWriter;
    using grpc::Status;
    using mediocre::image::v1beta::Image;

    class VideoServiceImpl final : public VideoService::Service {
    public:
        Status Video(
                ServerContext *context,
                const VideoRequest *request,
                ServerWriter<VideoResponse> *writer) override;

    private:
        static void processVideo(
                const mediocre::configuration::v1beta::GameConfiguration &configuration,
                const std::string &source,
                const std::function<void(VideoResponse)> &onResponse);
    };

}// namespace mediocre::video::v1beta

#endif// mediocre_image_video_v1beta_h