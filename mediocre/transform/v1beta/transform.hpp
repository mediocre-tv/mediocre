#ifndef mediocre_image_transform_v1beta_h
#define mediocre_image_transform_v1beta_h

#include <mediocre/transform/v1beta/transform.grpc.pb.h>
#include <opencv2/core/mat.hpp>

namespace mediocre::transform::v1beta {

    using grpc::ServerContext;
    using grpc::ServerWriter;
    using grpc::Status;
    using mediocre::image::v1beta::Image;

    class TransformServiceImpl final : public TransformService::Service {
    public:
        Status Transform(
                ServerContext *context,
                const TransformRequest *request,
                ServerWriter<TransformResponse> *writer) override;
        Status TransformBatch(
                ServerContext *context,
                const TransformBatchRequest *request,
                ServerWriter<TransformBatchResponse> *writer) override;
        static std::string transform(
                const cv::Mat &image,
                const google::protobuf::RepeatedPtrField<mediocre::transform::v1beta::Transform> &transforms);

    private:
        using IntermediateType = std::variant<cv::Mat, std::string>;

        static cv::Mat transformImageToImage(const cv::Mat &input, const TransformImageToImage &transform);
        static std::string transformImageToText(const cv::Mat &input, const TransformImageToText &transform);
        static void transform(
                const google::protobuf::RepeatedPtrField<mediocre::transform::v1beta::Transform> &transforms,
                const std::function<void(mediocre::transform::v1beta::Transform, TransformResponse)> &onTransformed,
                IntermediateType &intermediate);
    };

}// namespace mediocre::transform::v1beta

#endif// mediocre_image_transform_v1beta_h