#pragma once

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include "implot.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace py = pybind11;

/**
 * Some helpers to make handling arrays easier.
 */

template <typename T>
using array_like = py::array_t<T, py::array::c_style | py::array::forcecast>;

std::string shapeToStr(py::array& array);

void assertArrayShape(std::string name,
                      py::array& array,
                      std::vector<std::vector<int>> shapes);

#define assert_shape(array, ...) assertArrayShape(#array, array, __VA_ARGS__)

template<typename T>
ImVec4 interpretColor(T& color) {

    ImVec4 c(0, 0, 0, 1);
    size_t colorLength = color.shape()[0];

    if (colorLength == 1) {
        c.x = color.data()[0];
        c.y = color.data()[0];
        c.z = color.data()[0];
    } else if (colorLength == 3) {
        c.x = color.data()[0];
        c.y = color.data()[1];
        c.z = color.data()[2];
    } else if (colorLength == 4) {
        c.x = color.data()[0];
        c.y = color.data()[1];
        c.z = color.data()[2];
        c.w = color.data()[3];
    } else {
        c = IMPLOT_AUTO_COL;
    }

    return c;
}

struct ImageInfo {

    int imageWidth = 0;
    int imageHeight = 0;
    int channels = 0;
    GLenum format = 0;
    GLenum datatype = 0;
};

ImageInfo interpretImage(py::array& image);

GLuint uploadImage(std::string id, ImageInfo& i, py::array& image);

struct PlotArrayInfo {

    std::vector<double> indices;
    const double* xDataPtr = nullptr;
    const double* yDataPtr = nullptr;
    size_t count = 0;
};

PlotArrayInfo interpretPlotArrays(
        array_like<double>& x,
        array_like<double>& y);

/*
 * Custom type-casters
 */

namespace pybind11 {
    namespace detail {

        /**
         * ImGui/ImPlot datatypes to numpy-array-like objects
         */

        template<> struct type_caster<ImVec2> {

        public:
            PYBIND11_TYPE_CASTER(ImVec2, _("ImVec2"));

            bool load(handle src, bool) {

                auto array = array_like<float>::ensure(src);

                assert_shape(array, {{2,}});

                value.x = array.at(0);
                value.y = array.at(1);

                return true;
            }

            static handle cast(
                    const ImVec2& src,
                    return_value_policy policy,
                    handle parent) {

                if (return_value_policy::copy == policy) {
                    pybind11::array_t<float> array(2, (float*)(&src));
                    return array.release();
                } else {
                    pybind11::array_t<float> array(2, (float*)(&src), parent);
                    return array.release();
                }
            }
        };

        template<> struct type_caster<ImPlotPoint> {

        public:
            PYBIND11_TYPE_CASTER(ImPlotPoint, _("ImPlotPoint"));

            bool load(handle src, bool) {

                auto array = array_like<double>::ensure(src);

                assert_shape(array, {{2,}});

                value.x = array.at(0);
                value.y = array.at(1);

                return true;
            }

            static handle cast(
                    const ImPlotPoint& src,
                    return_value_policy policy,
                    handle parent) {

                if (return_value_policy::copy == policy) {
                    pybind11::array_t<double> array(2, (double*)(&src));
                    return array.release();
                } else {
                    pybind11::array_t<double> array(2, (double*)(&src), parent);
                    return array.release();
                }
            }
        };

        template<> struct type_caster<ImPlotRange> {

        public:
            PYBIND11_TYPE_CASTER(ImPlotRange, _("ImPlotRange"));

            bool load(handle src, bool) {

                auto array = array_like<double>::ensure(src);

                assert_shape(array, {{2,}});

                value.Min = array.at(0);
                value.Max = array.at(1);

                return true;
            }

            static handle cast(
                    const ImPlotRange& src,
                    return_value_policy policy,
                    handle parent) {

                if (return_value_policy::copy == policy) {
                    pybind11::array_t<double> array(2, (double*)(&src));
                    return array.release();
                } else {
                    pybind11::array_t<double> array(2, (double*)(&src), parent);
                    return array.release();
                }
            }
        };

        template<> struct type_caster<ImVec4> {

        public:
            PYBIND11_TYPE_CASTER(ImVec4, _("ImVec4"));

            bool load(handle src, bool) {

                auto array = array_like<float>::ensure(src);

                assert_shape(array, {{4,}});

                value.x = array.at(0);
                value.y = array.at(1);
                value.z = array.at(2);
                value.w = array.at(3);

                return true;
            }

            static handle cast(
                    const ImVec4& src,
                    return_value_policy policy,
                    handle parent) {

                if (return_value_policy::copy == policy) {
                    pybind11::array_t<float> array(4, (float*)(&src));
                    return array.release();
                } else {
                    pybind11::array_t<float> array(4, (float*)(&src), parent);
                    return array.release();
                }
            }
        };
    }
}
