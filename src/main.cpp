#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "jepture.hpp"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;


PYBIND11_MODULE(jepture, m) {
    m.doc() = R"pbdoc(
        Jepture plugin
        -----------------------
        .. currentmodule:: jepture 
        .. autosummary::
           :toctree: _generate


           JpegStreamOutput 
           JpegStream
    )pbdoc";
    
    py::class_<JpegStreamOutput>(m,"JpegStreamOutput")
        .def_readwrite("number",&JpegStreamOutput::number)
        .def_readwrite("time_stamp",&JpegStreamOutput::time_stamp);

    py::class_<JpegStream>(m,"JpegStream", R"pbdoc(
                A stream of jpegs.

                Encodes and then writes frame directly to disk as jpeg files using nvidia's gpu accelerated jpeg encoder.
            )pbdoc")
        .def(py::init<std::vector<std::tuple<uint32_t,std::string> > , std::pair<uint32_t,uint32_t> , float , std::optional<uint32_t>,std::optional<std::unordered_map<std::string,double>>,  std::string >(),
                py::arg("cameras"), py::arg("resolution"), py::arg("fps"), py::arg("mode") = std::optional<uint32_t>(),py::arg("settings") = std::optional<std::unordered_map<std::string,double>>() ,py::arg("image_dir") = "./data",
                R"pbdoc(
                    Parameters
                    ----------
                    cameras : list
                        A list of of tuples with respectively a camera id and a name for the camera
                        Jpegs for a specific camera will writen to a directory with the name of the camera.
                    resolution: tuple
                        A tuple containing the capture image width and height in pixels.
                    fps: float
                        The target fps to capture frames at.
                    mode: int, optional
                        A sensor mode to use. If empty the implementation will select a sensor mode based on the target fps.
                    image_dir: str, optional
                        The directory to write the jpeg files to, (default is './data')
                )pbdoc")
        .def("next",&JpegStream::next, py::arg("skip") = false,
                R"pbdoc(
                    Captures the next frame

                    It is recommend to call this functions at least as often as the target fps to avoid missing frames.

                    Parameters
                    ----------
                    skip: bool, optional
                        Skip writing the next frame
                )pbdoc");

    py::class_<JpegBytesStreamOutput>(m,"JpegBytesStreamOutput")
        .def_readwrite("number",&JpegBytesStreamOutput::number)
        .def_readwrite("time_stamp",&JpegBytesStreamOutput::time_stamp)
        .def_readwrite("bytes",&JpegBytesStreamOutput::bytes);

    py::class_<JpegBytesStream>(m,"JpegBytesStream", R"pbdoc(
                A stream of jpegs.

                Encodes and then writes returns the bytes of the encoded jpeg.
            )pbdoc")
        .def(py::init<std::vector<std::tuple<uint32_t,std::string> > , std::pair<uint32_t,uint32_t> , float , std::optional<uint32_t>,std::optional<std::unordered_map<std::string,double>>>(),
                py::arg("cameras"), py::arg("resolution"), py::arg("fps"), py::arg("mode") = std::optional<uint32_t>(),py::arg("settings") = std::optional<std::unordered_map<std::string,double>>(),
                R"pbdoc(
                    Parameters
                    ----------
                    cameras : list
                        A list of of tuples with respectively a camera id and a name for the camera
                        Jpegs for a specific camera will writen to a directory with the name of the camera.
                    resolution: tuple
                        A tuple containing the capture image width and height in pixels.
                    fps: float
                        The target fps to capture frames at.
                    mode: int, optional
                        A sensor mode to use. If empty the implementation will select a sensor mode based on the target fps.
                )pbdoc")
        .def("next",&JpegBytesStream::next, py::arg("skip") = false,
                R"pbdoc(
                    Captures the next frame

                    It is recommend to call this functions at least as often as the target fps to avoid missing frames.

                    Parameters
                    ----------
                    skip: bool, optional
                        Skip writing the next frame
                )pbdoc");



    py::class_<NumpyStreamOutput>(m,"NumpyStreamOutput", R"pbdoc(
        The value returned by JpegsStream.next().
    )pbdoc")
        .def_readwrite("number",&NumpyStreamOutput::number)
        .def_readwrite("time_stamp",&NumpyStreamOutput::time_stamp)
        .def_readwrite("array",&NumpyStreamOutput::array);

    py::class_<NumpyStream>(m,"NumpyStream", R"pbdoc(
                A stream of numpy arrays containing a image in ABGR format.
            )pbdoc")
        .def(py::init<std::vector<std::tuple<uint32_t,std::string> > , std::pair<uint32_t,uint32_t> , float , std::optional<uint32_t>, std::optional<std::unordered_map<std::string,double>>>(),
                py::arg("cameras"), py::arg("resolution"), py::arg("fps"), py::arg("mode") = std::optional<uint32_t>(),py::arg("settings") = std::optional<std::unordered_map<std::string,double>>(),
                R"pbdoc(
                    Parameters
                    ----------
                    cameras : list
                        A list of of tuples with respectively a camera id and a name for the camera
                        Jpegs for a specific camera will writen to a directory with the name of the camera.
                    resolution: tuple
                        A tuple containing the capture image width and height in pixels.
                    fps: float
                        The target fps to capture frames at.
                    mode: int, optional
                        A sensor mode to use. If empty the implementation will select a sensor mode based on the target fps.
                )pbdoc")
        .def("next",&NumpyStream::next, py::arg("skip") = false,
                R"pbdoc(
                    Captures the next frame

                    It is recommend to call this functions at least as often as the target fps to avoid missing frames.

                    Parameters
                    ----------
                    skip: bool, optional
                        Skips processing the next frame, returned arrays will be empty
                )pbdoc");

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
