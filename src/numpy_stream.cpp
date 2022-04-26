#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <cstring>
#include "jepture.hpp"

#include <limits>

namespace py = pybind11;

NumpyStream::NumpyStream(
        std::vector<std::tuple<uint32_t,std::string> > cameras, 
        std::pair<uint32_t,uint32_t> resolution, 
        float fps, 
        std::optional<uint32_t> mode,
        std::optional<std::unordered_map<std::string,double>> settings
        )
    : ArgusStream(cameras,resolution,fps,mode,settings)
{

    this->transform_params = {};
    this->transform_params.transform_flag = NVBUFFER_TRANSFORM_FILTER | NVBUFFER_TRANSFORM_FLIP;
    this->transform_params.transform_flip = NvBufferTransform_None;
    this->transform_params.transform_filter = NvBufferTransform_Filter_Nearest;
    

    NvBufferCreateParams create_params;
    std::memset(&create_params,0,sizeof(NvBufferCreateParams));
    create_params.width = this->resolution.width();
    create_params.height = this->resolution.height();
    create_params.layout = NvBufferLayout_Pitch;
    create_params.payloadType = NvBufferPayload_SurfArray;
    create_params.colorFormat = NvBufferColorFormat_ARGB32;
    create_params.nvbuf_tag = NvBufferTag_VIDEO_CONVERT;

    this->dma_buffer = -1;
    auto ret = NvBufferCreateEx(&this->dma_buffer,&create_params);
    if(ret){
        throw std::runtime_error("failed to create conversion buffer");
    }
}

py::array_t<uint8_t> NumpyStream::copy_buffer(int in_dma_buffer){
    auto ret = NvBufferTransform(in_dma_buffer,this->dma_buffer,&this->transform_params);
    if(ret){
        throw std::runtime_error("failed to transform buffer");
    }
    NvBufferParams params;
    ret = NvBufferGetParams (this->dma_buffer, &params);
    if (ret)
    {
        throw std::runtime_error("failed to retrieve buffer params");
    }
    if (params.num_planes != 1 || params.height[0] != this->resolution.height() || params.width[0] != this->resolution.width()){
        throw std::runtime_error("got invalid buffer_params");
    }

    void *data_ptr;
    ret = NvBufferMemMap(this->dma_buffer,0,NvBufferMem_Read_Write,&data_ptr);
    if(ret){
        throw std::runtime_error("failed to map image buffer");
    }
    uint32_t *out_buffer = new uint32_t[this->resolution.width() * this->resolution.height()];
    NvBufferMemSyncForCpu(this->dma_buffer,0,&data_ptr);
    for(uint32_t i = 0;i < this->resolution.height();++i){
        uint8_t * src_ptr = (uint8_t *)data_ptr + i * params.pitch[0];
        uint8_t * dst_ptr = (uint8_t *)(out_buffer + i * this->resolution.width());
        std::memcpy(dst_ptr, src_ptr, this->resolution.width() * sizeof(uint32_t));
    }
    NvBufferMemUnMap(this->dma_buffer,0,&data_ptr);

    py::capsule clean_up(out_buffer,[](void *out_buffer){
            uint32_t * out_buf = reinterpret_cast<uint32_t *>(out_buffer);
            delete[] out_buf;
    });

    return py::array_t<uint8_t>(
            std::array<long int,3>({ (long int)this->resolution.height(), (long int)this->resolution.width(), 4}),
            std::array<long int,3>({ (long int)this->resolution.width() * 4, 4, 1 }),
            (uint8_t *)out_buffer,
            clean_up );
}

std::vector<NumpyStreamOutput> NumpyStream::next(bool skip){
    auto frames = ArgusStream::next(skip);
    std::vector<NumpyStreamOutput> res;
    for(uint32_t i = 0;i < this->cameras.size();i++){
        py::array_t<uint8_t> array;
        if(!skip){
            array = this->copy_buffer(frames[i].dma_buffer);
        }else{
            array = py::array_t<uint8_t>();
        }
        res.push_back({
            frames[i].number,
            frames[i].time_stamp,
            array,
        });
    }
    return res;
}


NumpyStream::~NumpyStream(){
    if(this->dma_buffer != -1){
        NvBufferDestroy(this->dma_buffer);
    }
}
