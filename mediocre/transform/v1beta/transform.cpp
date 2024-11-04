#include <mediocre/image/border/v1beta/border.hpp>
#include <mediocre/image/colour/v1beta/colour.hpp>
#include <mediocre/image/crop/v1beta/crop.hpp>
#include <mediocre/image/identity/v1beta/identity.hpp>
#include <mediocre/image/invert/v1beta/invert.hpp>
#include <mediocre/image/ocr/v1beta/ocr.hpp>
#include <mediocre/image/rotate/v1beta/rotate.hpp>
#include <mediocre/image/scale/v1beta/scale.hpp>
#include <mediocre/image/threshold/v1beta/threshold.hpp>
#include <mediocre/image/v1beta/image.hpp>
#include <mediocre/transform/v1beta/transform.hpp>
#include <opencv2/core/mat.hpp>

namespace mediocre::transform::v1beta {

    using mediocre::image::border::v1beta::BorderServiceImpl;
    using mediocre::image::colour::v1beta::GetColourServiceImpl;
    using mediocre::image::crop::v1beta::CropServiceImpl;
    using mediocre::image::identity::v1beta::IdentityServiceImpl;
    using mediocre::image::invert::v1beta::InvertServiceImpl;
    using mediocre::image::ocr::v1beta::OcrServiceImpl;
    using mediocre::image::rotate::v1beta::RotateServiceImpl;
    using mediocre::image::scale::v1beta::ScaleServiceImpl;
    using mediocre::image::threshold::v1beta::ThresholdServiceImpl;


    Status TransformServiceImpl::Transform(
            ServerContext *context,
            const TransformRequest *request,
            ServerWriter<TransformResponse> *writer) {

        IntermediateType intermediate = image::v1beta::Decode(request->image());

        const std::function<void(mediocre::transform::v1beta::Transform, TransformResponse)> writeResponseFn =
                [&writer](const mediocre::transform::v1beta::Transform &transform, const TransformResponse &response) {
                    writer->Write(response);
                };

        transform(request->transformations(), writeResponseFn, intermediate);

        return Status::OK;
    }

    Status TransformServiceImpl::TransformBatch(
            ServerContext *context,
            const TransformBatchRequest *request,
            ServerWriter<TransformBatchResponse> *writer) {

        const auto image = image::v1beta::Decode(request->image());

        for (const auto &batch: request->batch()) {

            TransformBatchResponse batchResponse;
            batchResponse.set_id(batch.id());

            const std::function<void(mediocre::transform::v1beta::Transform, TransformResponse)> onResponse =
                    [&batchResponse](const mediocre::transform::v1beta::Transform &transform, const TransformResponse &response) {
                        batchResponse.mutable_transforms()->Add()->CopyFrom(transform);
                        batchResponse.mutable_responses()->Add()->CopyFrom(response);
                    };

            IntermediateType intermediate = image.clone();
            transform(batch.transforms(), onResponse, intermediate);

            writer->Write(batchResponse);
        }

        return Status::OK;
    }

    std::string TransformServiceImpl::transform(const cv::Mat &image, const google::protobuf::RepeatedPtrField<mediocre::transform::v1beta::Transform> &transforms) {
        IntermediateType intermediate = image.clone();
        transform(transforms, [](const mediocre::transform::v1beta::Transform &transform, const TransformResponse &response) -> void {}, intermediate);

        if (const auto value = std::get_if<std::string>(&intermediate)) {
            return *value;
        } else {
            return "Result was not a string";
        }
    }

    void TransformServiceImpl::transform(
            const google::protobuf::RepeatedPtrField<mediocre::transform::v1beta::Transform> &transforms,
            const std::function<void(mediocre::transform::v1beta::Transform, TransformResponse)> &onTransformed,
            IntermediateType &intermediate) {

        int index = 0;
        for (const auto &transform: transforms) {
            TransformResponse response;
            auto before = std::chrono::high_resolution_clock::now();

            try {
                if (auto *mat = std::get_if<cv::Mat>(&intermediate)) {
                    if (transform.has_image_to_image()) {
                        *mat = transformImageToImage(*mat, transform.image_to_image());
                        image::v1beta::Encode(*mat, response.mutable_transformed()->mutable_image());
                    } else if (transform.has_image_to_text()) {
                        auto text = transformImageToText(*mat, transform.image_to_text());
                        response.mutable_transformed()->set_characters(text);
                        intermediate = text;
                    } else {
                        throw std::logic_error("Cannot handle transform, unimplemented method");
                    }
                } else {
                    throw std::logic_error("Cannot handle transform, unimplemented type");
                }
            } catch (const std::exception &e) {
                response.set_error(e.what());

                auto after = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> elapsed = after - before;
                response.set_elapsed(elapsed.count());

                onTransformed(transform, response);
                break;
            }

            auto after = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = after - before;
            response.set_elapsed(elapsed.count());

            onTransformed(transform, response);
            index++;
        }
    }

    cv::Mat TransformServiceImpl::transformImageToImage(const cv::Mat &input, const TransformImageToImage &transform) {
        if (transform.has_get_identity()) {
            return IdentityServiceImpl::GetIdentity(input, transform.get_identity());
        } else if (transform.has_crop()) {
            return CropServiceImpl::Crop(input, transform.crop());
        } else if (transform.has_threshold()) {
            return ThresholdServiceImpl::Threshold(input, transform.threshold());
        } else if (transform.has_rotate()) {
            return RotateServiceImpl::Rotate(input, transform.rotate());
        } else if (transform.has_invert()) {
            return InvertServiceImpl::Invert(input, transform.invert());
        } else if (transform.has_scale()) {
            return ScaleServiceImpl::Scale(input, transform.scale());
        } else if (transform.has_border()) {
            return BorderServiceImpl::Border(input, transform.border());
        } else {
            throw std::logic_error("Unrecognised transform image to image request");
        }
    }

    std::string TransformServiceImpl::transformImageToText(const cv::Mat &input, const TransformImageToText &transform) {
        if (transform.has_get_characters()) {
            return OcrServiceImpl::GetCharacters(input, transform.get_characters());
        } else {
            throw std::logic_error("Unrecognised transform image to image request");
        }
    }

}// namespace mediocre::transform::v1beta
