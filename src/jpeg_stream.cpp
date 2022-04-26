#include "jepture.hpp"
#include "profile.hpp"

#include <cstdio>
#include <sstream>
#include <limits>
#include <iostream>

JpegStream::JpegStream(
        std::vector<std::tuple<uint32_t,std::string> > cameras, 
        std::pair<uint32_t,uint32_t> resolution, 
        float fps, 
        std::optional<uint32_t> mode,
        std::optional<std::unordered_map<std::string,double>> settings,
        std::string directory)
    : ArgusStream(cameras,resolution,fps,mode,settings),
    nv(NvJPEGEncoder::createJPEGEncoder("nvjpegjepture"))
{
    for(uint32_t i = 0;i < this->cameras.size();i++){
        fs::path new_dir(directory);
        this->directories.push_back(new_dir / this->cameras[i]->name);
        fs::create_directories(this->directories[i]);
    }
    this->jpeg_buffer_size = this->resolution.width() * this->resolution.height() * 3 / 2;
    this->jpeg_buffer = new unsigned char[this->jpeg_buffer_size];
}

std::vector<JpegStreamOutput> JpegStream::next(bool skip = false){
    auto frames = ArgusStream::next(skip);
    std::vector<JpegStreamOutput> res;
    for(uint32_t i = 0;i < this->cameras.size();i++){
        if(!skip){
            unsigned long buffer_size = this->jpeg_buffer_size;
            auto ret = this->nv->encodeFromFd(frames[i].dma_buffer, JCS_YCbCr, &this->jpeg_buffer,buffer_size,90);
            if(ret < 0){
                throw std::runtime_error("failed to encode jpeg");
            }
            if(buffer_size > this->jpeg_buffer_size){
                this->jpeg_buffer_size = buffer_size;
            }
            std::string file_name(std::to_string(frames[i].number));
            file_name.append(".jpg");
            std::fstream s(this->directories[i] / file_name, s.binary | s.trunc | s.out);
            s.write((char *)this->jpeg_buffer,buffer_size);
            s.flush();
            s.close();
        }

        res.push_back({
                frames[i].number,
                frames[i].time_stamp,
        });
    }
    return res;
}

JpegStream::~JpegStream(){
    delete[] this->jpeg_buffer;
}


JpegBytesStream::JpegBytesStream(
        std::vector<std::tuple<uint32_t,std::string> > cameras, 
        std::pair<uint32_t,uint32_t> resolution, 
        float fps, 
        std::optional<uint32_t> mode,
        std::optional<std::unordered_map<std::string,double>> settings
        )
    : ArgusStream(cameras,resolution,fps,mode,settings),
    nv(NvJPEGEncoder::createJPEGEncoder("nvjpegjepture"))
{
    this->jpeg_buffer_size = this->resolution.width() * this->resolution.height() * 3 / 2;
    this->jpeg_buffer = new unsigned char[this->jpeg_buffer_size];
}

std::vector<JpegBytesStreamOutput> JpegBytesStream::next(bool skip = false){
    auto frames = ArgusStream::next(skip);
    std::vector<JpegBytesStreamOutput> res;
    for(uint32_t i = 0;i < this->cameras.size();i++){
        unsigned long buffer_size = this->jpeg_buffer_size;
        if(!skip){
            auto ret = this->nv->encodeFromFd(frames[i].dma_buffer, JCS_YCbCr, &this->jpeg_buffer,buffer_size,90);
            if(ret < 0){
                throw std::runtime_error("failed to encode jpeg");
            }
            if(buffer_size > this->jpeg_buffer_size){
                this->jpeg_buffer_size = buffer_size;
            }
        }

        res.push_back({
                frames[i].number,
                frames[i].time_stamp,
                std::string((char *)this->jpeg_buffer,buffer_size)
        });
    }
    return res;
}

JpegBytesStream::~JpegBytesStream(){
    delete[] this->jpeg_buffer;
}
