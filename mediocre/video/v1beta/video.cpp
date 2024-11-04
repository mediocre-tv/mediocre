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
#include <mediocre/video/v1beta/video.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>

namespace mediocre::video::v1beta {

    using mediocre::image::border::v1beta::BorderServiceImpl;
    using mediocre::image::colour::v1beta::GetColourServiceImpl;
    using mediocre::image::crop::v1beta::CropServiceImpl;
    using mediocre::image::identity::v1beta::IdentityServiceImpl;
    using mediocre::image::invert::v1beta::InvertServiceImpl;
    using mediocre::image::ocr::v1beta::OcrServiceImpl;
    using mediocre::image::rotate::v1beta::RotateServiceImpl;
    using mediocre::image::scale::v1beta::ScaleServiceImpl;
    using mediocre::image::threshold::v1beta::ThresholdServiceImpl;
    using mediocre::transform::v1beta::TransformServiceImpl;

    Status VideoServiceImpl::Video(
            ServerContext *context,
            const VideoRequest *request,
            ServerWriter<VideoResponse> *writer) {

        try {
            processVideo(request->configuration(), request->user(), request->source(), [&writer](const VideoResponse &response) {
                writer->Write(response);
            });
            return Status::OK;
        } catch (const std::exception &e) {
            return {grpc::StatusCode::INTERNAL, e.what()};
        }
    }

    transform::v1beta::Transform convertToTransform(const transform::v1beta::TransformImageToImage &imageToImageTransform) {
        transform::v1beta::Transform transform;
        transform.mutable_image_to_image()->CopyFrom(imageToImageTransform);
        return transform;
    }

    void VideoServiceImpl::processVideo(const mediocre::configuration::v1beta::GameConfiguration &configuration, const mediocre::configuration::v1beta::UserConfiguration &preferences, const std::string &source, const std::function<void(VideoResponse)> &onResponse) {

        cv::VideoCapture cap(source);
        if (!cap.isOpened()) {
            throw std::runtime_error("Error opening video stream or file");
        }

        double fps = cap.get(cv::CAP_PROP_FPS);
        if (fps <= 0) {
            throw std::runtime_error("Error: Could not retrieve FPS information from the video.");
        }

        const auto &stage = configuration.stages(0);
        const auto &zone_ids = stage.zone_ids();

        std::map<std::string, std::string> region_values;

        cv::Mat frame;
        while (true) {
            cap >> frame;
            if (frame.empty())
                break;

            const auto timestamp = cap.get(cv::CAP_PROP_POS_MSEC) / 1000;

            VideoResponse videoResponse;
            videoResponse.set_timestamp(timestamp);

            for (const auto &zone: configuration.zones()) {
                if (std::find(zone_ids.begin(), zone_ids.end(), zone.id()) == zone_ids.end()) {
                    continue;
                }

                google::protobuf::RepeatedPtrField<transform::v1beta::Transform> zoneTransforms;
                std::transform(zone.transformations().begin(), zone.transformations().end(), google::protobuf::RepeatedPtrFieldBackInserter(&zoneTransforms), convertToTransform);

                const auto &region_ids = zone.region_ids();
                for (const auto &region: configuration.regions()) {
                    if (std::find(region_ids.begin(), region_ids.end(), region.id()) == region_ids.end()) {
                        continue;
                    }

                    google::protobuf::RepeatedPtrField<transform::v1beta::Transform> regionTransforms;
                    regionTransforms.MergeFrom(zoneTransforms);
                    regionTransforms.MergeFrom(region.transformations());

                    auto result = TransformServiceImpl::transform(frame, regionTransforms);

                    RegionResponse regionResponse;
                    regionResponse.set_name(region.name());
                    regionResponse.set_output(result);

                    videoResponse.add_regions()->CopyFrom(regionResponse);
                }
            }

            for (const auto &region: videoResponse.regions()) {
                if (region.output() == "Result was not a string" || !(region.name() == "Blue Score" || region.name() == "Orange Score")) {
                    continue;
                }

                const auto previous_result = region_values[region.name()];
                const auto &current_result = region.output();

                region_values[region.name()] = current_result;

                if (region.output() == previous_result || region_values.count("Blue Score") == 0 || region_values.count("Orange Score") == 0) {
                    continue;
                }

                std::cout << "Change detected at " << timestamp << " seconds" << std::endl;

                const auto blue_score = std::find_if(videoResponse.regions().begin(), videoResponse.regions().end(), [](const RegionResponse &region) { return region.name() == "Blue Score"; })->output();
                const auto orange_score = std::find_if(videoResponse.regions().begin(), videoResponse.regions().end(), [](const RegionResponse &region) { return region.name() == "Orange Score"; })->output();
                const auto clock = std::find_if(videoResponse.regions().begin(), videoResponse.regions().end(), [](const RegionResponse &region) { return region.name() == "Clock"; })->output();

                std::ostringstream messageStream;
                messageStream
                        << "\\\`" << clock << "\\\`"
                        << " \\\`Blue " << blue_score
                        << "-"
                        << orange_score << " Orange\\\`";
                std::string message = messageStream.str();

                std::cout << message << std::endl;

                if (!preferences.notification_url().empty()) {
                    sendNotification("", message, preferences.notification_url());
                }
            }

            onResponse(videoResponse);

            double next_frame = (timestamp + 1) * fps;
            cap.set(cv::CAP_PROP_POS_FRAMES, next_frame);
        }

        cap.release();
    }

    int VideoServiceImpl::sendNotification(const std::string &title, const std::string &body, const std::string &url) {
        std::string command = "apprise -vv -t \"" + title + "\" -b \"" + body + "\" " + url;
        return std::system(command.c_str());
    }

}// namespace mediocre::video::v1beta
