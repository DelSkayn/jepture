#pragma once

#include <Argus/Argus.h>
#include "NvJpegEncoder.h"
#include "NvUtils.h"
#include <EGLStream/EGLStream.h>
#include <EGLStream/NV/ImageNativeBuffer.h>
#include <nvbuf_utils.h>

#include <memory>
#include <string>
#include <vector>
#include <unistd.h>
#include "filesystem.hpp"

using namespace Argus;
using namespace EGLStream;

namespace fs = ghc::filesystem;

struct CameraStream{
    std::string name;
    UniqueObj<FrameConsumer> consumer;
    UniqueObj<OutputStream> stream;
    IEGLOutputStream * i_stream;
    IFrameConsumer * i_consumer;
    int dma_buffer;
};

struct CameraData{
    uint32_t id;
    std::string name;
};

struct ArgusStreamOutput{
    uint64_t number;
    uint64_t time_stamp;
    int dma_buffer;
};

class ArgusStream{
protected:
    Size2D<uint32_t> resolution;
    float fps;

    std::vector<std::unique_ptr<CameraStream>> cameras;
    UniqueObj<CameraProvider> provider;
    UniqueObj<CaptureSession> session;
    UniqueObj<Request> request;

    ICaptureSession * i_capture_session;

    bool started;

public:
    ArgusStream(std::vector<std::tuple<uint32_t,std::string> > cameras, std::pair<uint32_t,uint32_t> resolution, float fps);

    virtual ~ArgusStream();

    std::vector<ArgusStreamOutput> next();
};

struct JpegStreamOutput{
    uint64_t number;
    uint64_t time_stamp;
};

class JpegStream: protected ArgusStream {
    std::unique_ptr<NvJPEGEncoder> nv;
    std::vector<fs::path> directories;
    unsigned char * jpeg_buffer;
    unsigned long jpeg_buffer_size;

public:
    JpegStream(std::vector<std::tuple<uint32_t,std::string> > cameras, std::pair<uint32_t,uint32_t> resolution, float fps, std::string directory);
    ~JpegStream();

    
    std::vector<JpegStreamOutput> next(bool skip);
};
