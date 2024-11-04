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
#include <sys/wait.h>

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

    void VideoServiceImpl::processVideo(const mediocre::configuration::v1beta::GameConfiguration &configuration, const mediocre::configuration::v1beta::UserConfiguration &preferences, const VideoSource &source, const std::function<void(VideoResponse)> &onResponse) {

        cv::VideoCapture cap;
        if (source.has_file()) {
            cap.open(source.file());
        } else {
            cap.open(source.camera());
        }

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

            const auto blue_score = std::find_if(videoResponse.regions().begin(), videoResponse.regions().end(), [](const RegionResponse &region) { return region.name() == "Blue Score"; })->output();
            const auto orange_score = std::find_if(videoResponse.regions().begin(), videoResponse.regions().end(), [](const RegionResponse &region) { return region.name() == "Orange Score"; })->output();
            const auto clock = std::find_if(videoResponse.regions().begin(), videoResponse.regions().end(), [](const RegionResponse &region) { return region.name() == "Clock"; })->output();

            const auto previous_blue_score = region_values["Blue Score"];
            const auto previous_orange_score = region_values["Orange Score"];

            if ((!blue_score.empty() && blue_score != "Result was not a string") && (!orange_score.empty() && orange_score != "Result was not a string") && (!clock.empty() && clock != "Result was not a string") && (blue_score != previous_blue_score || orange_score != previous_orange_score)) {
                std::cout << "Change detected at " << timestamp << " seconds" << std::endl;

                std::ostringstream messageStream;
                messageStream
                        << "`" << clock << "`"
                        << " `Blue " << blue_score
                        << "-"
                        << orange_score << " Orange`";
                std::string message = messageStream.str();

                std::cout << message << std::endl;

                if (!preferences.notification_url().empty()) {
                    sendNotification("", message, preferences.notification_url());
                }

                region_values["Blue Score"] = blue_score;
                region_values["Orange Score"] = orange_score;
            }

            onResponse(videoResponse);

            double next_frame = (timestamp + 1) * fps;
            cap.set(cv::CAP_PROP_POS_FRAMES, next_frame);
        }

        cap.release();
    }

    int VideoServiceImpl::sendNotification(const std::string &title, const std::string &body, const std::string &url) {
        // Construct the command-line arguments as C-style strings
        std::vector<char *> args;

        // Add the apprise command
        args.push_back(strdup("apprise"));

        // Add each argument for the command
        if (!title.empty()) {
            args.push_back(strdup("-t"));
            args.push_back(strdup(title.c_str()));
        }

        if (!body.empty()) {
            args.push_back(strdup("-b"));
            args.push_back(strdup(body.c_str()));
        }

        // Add the Apprise URL
        args.push_back(strdup(url.c_str()));

        // Mark the end of the arguments with nullptr
        args.push_back(nullptr);

        // Fork the process
        pid_t pid = fork();

        if (pid == -1) {
            // Fork failed
            perror("fork failed");
            return errno;
        } else if (pid == 0) {
            // In the child process
            execv("/usr/local/bin/apprise", args.data());// Replace with the path to apprise
            perror("execv failed");                      // If execv returns, it failed
            _exit(EXIT_FAILURE);                         // Exit child process if execv fails
        } else {
            // In the parent process
            int status;
            waitpid(pid, &status, 0);// Wait for the child process to complete

            // Free the allocated memory for arguments
            for (char *arg: args) {
                free(arg);
            }

            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);// Return the exit code of the child process
            } else {
                return -1;// Indicate an error if the child did not terminate normally
            }
        }
    }

}// namespace mediocre::video::v1beta
