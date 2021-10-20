#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "jepture.hpp"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;


PYBIND11_MODULE(jepture, m) {
    
    py::class_<JpegStreamOutput>(m,"JpegStreamOutput")
        .def_readwrite("number",&JpegStreamOutput::number)
        .def_readwrite("time_stamp",&JpegStreamOutput::time_stamp);

    py::class_<JpegStream>(m,"JpegStream")
        .def(py::init<std::vector<std::tuple<uint32_t,std::string> > , std::pair<uint32_t,uint32_t> , float , std::string >())
        .def("next",&JpegStream::next, py::arg("skip") = false);

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
