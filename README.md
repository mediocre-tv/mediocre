Library under development, everything here is subject to change.

# Mediocre

Mediocre, or ***M**ediocre **E**xtraction of **D**ata **I**nvolving an **O**ptical **C**haracter **R**ecognition
**E**ngine*, is a configurable application for extracting and emitting data in real-time from videos.

Key libraries used:

- [gRPC](https://github.com/grpc/grpc) for exposing functions in a language and platform neutral manner
- [OpenCV](https://github.com/opencv/opencv) for various image processing methods
- [Tesseract](https://github.com/tesseract-ocr/tesseract) for optical character recognition

## Install and Run

### Windows/Linux

```shell
sudo apt update

# install build requirements
sudo apt install cmake ninja-build # build tools
sudo apt install build-essential gdb # building for linux
sudo apt install mingw-w64 # building for windows

# install dependency requirements
sudo apt install bison # gettext
sudo apt install libx11-dev libxft-dev libxext-dev xorg openbox # cairo
sudo apt install libx11-dev libgles2-mesa-dev # libepoxy
sudo apt install autoconf automake libtool pkg-config # libxcrypt
sudo apt install libxi-dev libxtst-dev # at-spi2-core
sudo apt install libxrandr-dev # gtk3

# install package manager
sudo apt install curl zip unzip tar
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && ./bootstrap-vcpkg.sh && cd ..
```

### Docker

Currently, the best way to run mediocre is via the pre-built docker images, though there are plans to provide
executables for various platforms in the future.

Docker hub: https://hub.docker.com/repository/docker/obroomhall/mediocre

Tag convention:

- `obroomhall/mediocre:master` is the most up-to-date image
- `obroomhall/mediocre:pr-x` is built from each pull request, where `x` is the pull request number
- `obroomhall/mediocre:sha-abc` is built from each commit, where `abc` is the commit hash
- `obroomhall/mediocre:*-cache` exist to speed up subsequent builds, these are not runnable
- There will also be some `vX.Y.Z` tags after the first release

#### Insecure

You should expose port `8081` for the gRPC server, for example:

```commandline
docker run -p 8081:8081 --name mediocre obroomhall/mediocre:master
```

### Secure

If you want to connect securely, you must provide valid TLS certificates.

```commandline
docker run -p 8081:8081 --name mediocre obroomhall/mediocre:master -v /path/to/your/certs:/certificates:ro
```

## Usage

### gRPC Tools

One way to invoke the functions in mediocre is to use a gRPC tool, some of which are listed in the
[grpc-ecosystem](https://github.com/grpc-ecosystem/awesome-grpc#tools) project. Ideally choose a tool that uses gRPC
server reflection, so that it can automatically detect which methods mediocre exports.

Some that I have used in the past:

- [grpc-client-cli](https://github.com/vadimi/grpc-client-cli) - an interactive gRPC client command line tool
- [grpcui](https://github.com/fullstorydev/grpcui) - an interactive web UI for gRPC

### Custom Client

You can also use invoke mediocre from any language that has gRPC support, for example; C/C++, C#, Go, Java, Python.
The full list of officially supported languages can be found in the [gRPC docs](https://grpc.io/docs/).

For some languages, you can add a dependency to mediocre via a
[Buf.Build Remote Package](https://buf.build/broomy/mediocre/assets), this is the easiest method, but is limited to
the languages currently supported by the Buf Schema Registry.

If there is no remote package available for your language, you must build the protobuf source files yourself.
Instructions on how to do that can be found at https://buf.build/plugins.

For example, to build source files for C#, you could create a `buf.gen.yaml` as:

```yaml
version: v1
managed:
  enabled: true
plugins:
  - plugin: buf.build/grpc/csharp:v1.56.0
    out: gen
  - plugin: buf.build/protocolbuffers/csharp
    out: gen
```

And then build with `buf generate`.

## Documentation

Documentation can be found on the [Buf Schema Registry](https://buf.build/broomy/mediocre).

## Contributing

Mediocre was developed in [JetBrains CLion](https://www.jetbrains.com/clion/), and though other IDEs can be used the
instructions below are tailored towards building and running in CLion.

### Build

1. Run the `Docker Toolchain with Registry Cache` run configuration to build the latest image as
   `mediocre:local-develop`
    1. This uses registry cache to speed up the initial build (should finish building in a matter of seconds)
    2. Use this script if there have been any significant changes to the docker image (full rebuilds take about an hour)
2. Go to `File > Settings > Build, Execution, Deployment` and add a `Docker` toolchain
    1. For image use the previously built `mediocre:local-develop` image
    2. For container settings, expose the port mediocre listens on `-p 0.0.0.0:8081:8081 --rm`
    3. For CMake use `cmake`
    4. For debugger, use `\local\gdb\install\bin\gdb`
3. Build the protobuf files using the `proto-objects` cmake run configuration

### Run

For library source file changes, run the `mediocre` cmake run configuration.

Otherwise, for docker changes, run the `Build and Run` docker run configuration.

### Proxy

You can run the proxy alongside
by [downloading the latest release](https://github.com/improbable-eng/grpc-web/releases),
adding it to your PATH, and then running the `Proxy` run configuration. You can run the proxy with or without TLS
certificates.

If you do not have your own TLS certificates, you can generate your own self-signed certificates. You can follow the
[Let's Encrypt guide](https://letsencrypt.org/docs/certificates-for-localhost/) for making and trusting your own
certificates. The general steps are:

1. Install [minica](https://github.com/jsha/minica)
2. Generate root and end certificates `minica --domains localhost`
3. Import the root certificate `Import-Certificate -FilePath .\minica.pem -CertStoreLocation cert:\CurrentUser\Root`
4. Mount the end certificates to your docker container `-v /path/to/your/certs/localhost:/certificates:ro`

### Updating Protobuf Definitions

If you are making changes in this project and a dependent project, such
as [mediocre-configure](https://github.com/mediocre-tv/mediocre-configure),
you will likely want to use your locally updated protobuf definitions in the dependent project, before committing
changes.

Other languages may vary, but to compile for typescript:

1. In `./@buf/typescript` run `npm install && npm link`
2. In the external project run `npm link @buf/broomy_mediocre.community_timostamm-protobuf-ts`
3. Each time a protobuf definition changes, run `buf generate` in the project root directory
