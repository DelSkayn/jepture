#include "jepture.hpp"
#include "profile.hpp"

#include <cstdio>
#include <sstream>
#include <limits>
#include <iostream>

uint32_t select_sensor_mode(std::vector<SensorMode*> & sensor_modes, float fps){
    if (sensor_modes.size() == 0){
        throw std::runtime_error("Could not get sensor_modes for camera");
    }
    uint64_t fps_nano =  1e9 / static_cast<double>(fps) + 0.9;
    for (uint32_t i = 0; i < sensor_modes.size(); i++) {
        std::cout << "sensor mode[" << i << "] ";
        auto i_sensor_mode = interface_cast<ISensorMode>(sensor_modes[i]);
        auto duration = i_sensor_mode->getFrameDurationRange();
        std::cout << "fps: " << (double)1e9 / (double)duration.min();
        auto res = i_sensor_mode->getResolution();
        std::cout << " resolution: " << res.width() << "x" << res.height() << std::endl;


        if (duration.min() <= fps_nano && duration.max() >= fps_nano){
            std::cout << "selected mode: " << i << std::endl;
            return i;
        }

    }
    throw std::runtime_error("Could not find a sensor mode which supports requested fps");
}

ArgusStream::ArgusStream(
        std::vector<std::tuple<uint32_t,std::string> > cameras, 
        std::pair<uint32_t,uint32_t> resolution, 
        float fps,
        std::optional<uint32_t> mode){
    this->resolution = Size2D<uint32_t>(resolution.first,resolution.second);
    this->fps = fps;
    this->provider.reset(CameraProvider::create());
    ICameraProvider * i_camera_provider = interface_cast<ICameraProvider>(this->provider);
    if(!i_camera_provider)
        throw std::runtime_error("failed to create camera provider");

    std::printf("Argus Version: %s\n", i_camera_provider->getVersion().c_str());
    std::vector<CameraDevice *> camera_devices;
    i_camera_provider->getCameraDevices(&camera_devices);

    if (camera_devices.size() == 0){
        throw std::runtime_error("Could not find any cameras");
    }

    std::vector<CameraDevice *> cameras_in_use;
    for(auto & data: cameras){
        if(std::get<0>(data) > camera_devices.size()){
            auto stream = std::stringstream();
            stream << "Could not find camera with id: \"" << std::get<0>(data) << "\".";
            throw std::runtime_error(stream.str());
        }
        if (!camera_devices[std::get<0>(data)]){
            auto stream = std::stringstream();
            stream << "Camera with id: \"" << std::get<0>(data) << "\" used twice.";
            throw std::runtime_error(stream.str());
        }
        cameras_in_use.push_back(camera_devices[std::get<0>(data)]);
        camera_devices[std::get<0>(data)] = NULL;
    }

    this->session.reset(i_camera_provider->createCaptureSession(cameras_in_use));



    this->i_capture_session = interface_cast<ICaptureSession>(session);
    auto i_event_provider = interface_cast<IEventProvider>(session);

    if(!i_event_provider || !i_capture_session){
        throw std::runtime_error("Failed to create capture session.");
    }

    UniqueObj<OutputStreamSettings> settings(i_capture_session->createOutputStreamSettings(STREAM_TYPE_EGL));
    auto i_stream_settings = interface_cast<IOutputStreamSettings>(settings.get());
    auto i_egl_stream_settings = interface_cast<IEGLOutputStreamSettings>(settings.get());
    if(!i_egl_stream_settings || !i_stream_settings){
        throw std::runtime_error("Failed to create stream settings");
    }

    i_egl_stream_settings->setPixelFormat(PIXEL_FMT_YCbCr_420_888);
    i_egl_stream_settings->setResolution(this->resolution);
    i_egl_stream_settings->setEGLDisplay(eglGetDisplay(EGL_DEFAULT_DISPLAY));


    this->request.reset(i_capture_session->createRequest());
    auto i_request = interface_cast<IRequest>(request);
    if(!i_request){
        throw std::runtime_error("failed to create request");
    }

    std::vector<SensorMode*> sensor_modes;
    uint32_t sensor_mode = std::numeric_limits<uint32_t>::max();
    for(uint32_t i = 0;i < cameras.size();i++){
        ICameraProperties * i_camera_prop= interface_cast<ICameraProperties>(cameras_in_use[i]);
        if (!i_camera_prop){
            throw std::runtime_error("Failed to get ICameraProperties interface");
        }
        sensor_modes.clear();
        i_camera_prop->getBasicSensorModes(&sensor_modes);

        uint32_t cur_sensor_mode;
        if (mode){
            cur_sensor_mode = *mode;
        }else{
            cur_sensor_mode = select_sensor_mode(sensor_modes,fps);
        }
        if (sensor_mode != std::numeric_limits<uint32_t>::max() && cur_sensor_mode != sensor_mode){
            throw std::runtime_error("Cameras can not agree on an sensor mode");
        }
        sensor_mode = cur_sensor_mode;
        i_stream_settings->setCameraDevice(cameras_in_use[i]);

        this->cameras.push_back(std::make_unique<CameraStream>());

        this->cameras[i]->stream.reset(i_capture_session->createOutputStream(settings.get()));
        auto i_stream = interface_cast<IEGLOutputStream>(this->cameras[i]->stream.get());
        if(!i_stream){
            throw std::runtime_error("failed to create stream for one of the cameras");
        }
        this->cameras[i]->i_stream = i_stream;

        this->cameras[i]->consumer.reset(FrameConsumer::create(this->cameras[i]->stream.get()));
        auto i_consumer = interface_cast<IFrameConsumer>(this->cameras[i]->consumer.get());
        if(!i_consumer){
            throw std::runtime_error("failed to create frame consumer for one of the cameras");
        }
        this->cameras[i]->i_consumer = i_consumer;
        this->cameras[i]->name = std::get<1>(cameras[i]);
        this->cameras[i]->dma_buffer = 0;
        i_request->enableOutputStream(this->cameras[i]->stream.get());
    }

    auto i_source_settings = interface_cast<ISourceSettings>(i_request->getSourceSettings());
    if(!i_request){
        throw std::runtime_error("failed to create request for left camera");
    }
    i_source_settings->setSensorMode(sensor_modes.at(sensor_mode));
    i_source_settings->setFrameDurationRange(Range<uint64_t>(1e9/(double)fps));


    this->started = false;
}

ArgusStream::~ArgusStream(){
    if (this->started){
        this->i_capture_session->stopRepeat();
    }
    this->i_capture_session->waitForIdle();
    for(uint32_t i = 0;i < cameras.size();i++){
        this->cameras[i]->i_stream->disconnect();

        if(this->cameras[i]->dma_buffer){
            NvBufferDestroy(this->cameras[i]->dma_buffer);
        }
    }
    this->provider.reset();
}


std::vector<ArgusStreamOutput> ArgusStream::next(bool skip){
    std::vector<ArgusStreamOutput> res;
    if(!this->started){
        this->started = true;
        if(i_capture_session->repeat(this->request.get()) != STATUS_OK){
            throw std::runtime_error("failed to start capture request");
        }
        /*
        for(uint32_t i = 0;i < this->cameras.size();i++){
            this->cameras[i]->i_stream->waitUntilConnected();
        }
        */
    }

    for(uint32_t i = 0;i < this->cameras.size();i++){
        UniqueObj<Frame> frame(this->cameras[i]->i_consumer->acquireFrame());
        auto i_frame = interface_cast<IFrame>(frame.get());
        if(!i_frame){
            throw std::runtime_error("failed to get frame from camera");
        }


        auto time_stamp = i_frame->getTime();
        auto number = i_frame->getNumber();

        if(!skip){
            auto native_buffer = interface_cast<NV::IImageNativeBuffer>(i_frame->getImage());
            if(!native_buffer){
                throw std::runtime_error("native buffers not supported");
            }

            if(!this->cameras[i]->dma_buffer){
                this->cameras[i]->dma_buffer = native_buffer->createNvBuffer(this->cameras[i]->i_stream->getResolution(),
                            NvBufferColorFormat_YUV420,
                            NvBufferLayout_BlockLinear);
                if(!this->cameras[i]->dma_buffer){
                    throw std::runtime_error("failed to create dma buffer");
                }
            }else{
                if(native_buffer->copyToNvBuffer(this->cameras[i]->dma_buffer) != STATUS_OK){
                    throw std::runtime_error("failed to copy frame to buffer");
                }
            }
        }
        res.push_back({
                number,
                time_stamp,
                this->cameras[i]->dma_buffer
                });
    }
    return res;
}

