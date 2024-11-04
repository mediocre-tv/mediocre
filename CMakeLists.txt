cmake_minimum_required(VERSION 3.25)
project(mediocre)

set(CMAKE_CXX_STANDARD 17)


## -- specify library files -- ##

# list all cpp files here
add_executable(
        mediocre
        mediocre/main/main.cpp
        mediocre/server/server.cpp
        mediocre/dependency/v1/dependency.cpp
        mediocre/image/ocr/v1beta/ocr.cpp
        mediocre/image/identity/v1beta/identity.cpp
        mediocre/image/crop/v1beta/crop.cpp
        grpc/health/v1/health.cpp
        mediocre/image/v1beta/image.cpp
        mediocre/transform/v1beta/transform.cpp
        mediocre/image/colour/v1beta/colour.cpp
        mediocre/image/threshold/v1beta/threshold.cpp
        mediocre/image/rotate/v1beta/rotate.cpp
        mediocre/image/invert/v1beta/invert.cpp
        mediocre/image/scale/v1beta/scale.cpp
        mediocre/image/border/v1beta/border.cpp
        mediocre/image/ocr/v1beta/mediocre_numeric_display.cpp
        mediocre/video/v1beta/video.cpp
)

# include all hpp files
target_include_directories(mediocre PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# list all .proto files here
add_library(
        proto-objects OBJECT
        mediocre/dependency/v1/dependency.proto
        mediocre/image/v1beta/image.proto
        mediocre/image/ocr/v1beta/ocr.proto
        mediocre/image/identity/v1beta/identity.proto
        mediocre/image/crop/v1beta/crop.proto
        mediocre/transform/v1beta/transform.proto
        mediocre/image/colour/v1beta/colour.proto
        mediocre/image/threshold/v1beta/threshold.proto
        mediocre/options/v1beta/options.proto
        mediocre/image/rotate/v1beta/rotate.proto
        mediocre/image/invert/v1beta/invert.proto
        mediocre/image/scale/v1beta/scale.proto
        mediocre/image/border/v1beta/border.proto
        mediocre/configuration/v1beta/game.proto
        mediocre/configuration/v1beta/test.proto
        mediocre/configuration/v1beta/user.proto
        mediocre/video/v1beta/video.proto
        grpc/health/v1/health.proto
)

target_link_libraries(mediocre proto-objects)


## -- find external packages -- ##

find_package(Tesseract CONFIG REQUIRED)
message(STATUS "Using Tesseract ${Tesseract_VERSION}")
target_include_directories(mediocre PUBLIC ${Tesseract_INCLUDE_DIRS})
target_link_libraries(mediocre Tesseract::libtesseract)

find_package(Leptonica CONFIG REQUIRED)
message(STATUS "Using Leptonica ${Leptonica_VERSION}")
target_include_directories(mediocre PUBLIC ${Leptonica_INCLUDE_DIRS})
target_link_libraries(mediocre ${Leptonica_LIBRARIES})

find_package(OpenCV CONFIG REQUIRED)
message(STATUS "Using OpenCV ${OpenCV_VERSION} with libraries ${OpenCV_LIBS}")
target_link_libraries(mediocre ${OpenCV_LIBS})

find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")
target_link_libraries(proto-objects PUBLIC protobuf::libprotobuf)

find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
target_link_libraries(proto-objects PUBLIC gRPC::grpc++ gRPC::grpc++_reflection)


## -- generate files for proto-objects -- ##

# set some reusable directory variables
set(PROTO_IMPORT_DIRS ${CMAKE_CURRENT_LIST_DIR})
set(PROTO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Protobuf)
file(MAKE_DIRECTORY ${PROTO_BINARY_DIR})

# link up proto-objects
target_include_directories(proto-objects PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")

# generate protobuf files (.pb.h and .pb.cc)
protobuf_generate(
        TARGET proto-objects
        IMPORT_DIRS ${PROTO_IMPORT_DIRS}
        PROTOC_OUT_DIR ${PROTO_BINARY_DIR})

# generate grpc files (.grpc.pb.h and .grpc.pb.cc)
protobuf_generate(
        TARGET proto-objects
        LANGUAGE grpc
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
        PLUGIN "protoc-gen-grpc=\$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
        IMPORT_DIRS ${PROTO_IMPORT_DIRS}
        PROTOC_OUT_DIR ${PROTO_BINARY_DIR})


## -- install project -- ##

set_target_properties(mediocre PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")

install(TARGETS mediocre RUNTIME_DEPENDENCIES DIRECTORIES ${OpenCV_INSTALL_PATH}/lib)