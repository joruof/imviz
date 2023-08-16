#include "binding_helpers.hpp"

std::string shapeToStr(py::array& array) {

    std::stringstream ss;
    ss << "(";

    for (int k = 0; k < array.ndim(); ++k) {
        ss << array.shape()[k];
        if (k != array.ndim() - 1) {
            ss <<  ", ";
        }
    }
    ss << ")";

    return ss.str();
}

void assertArrayShape(std::string name,
                      py::array& array,
                      std::vector<std::vector<int>> shapes) {

    bool foundShape = false;

    for (auto& shape : shapes) { 
        if ((int)shape.size() != array.ndim()) {
            continue;
        }
        bool ok = true;
        for (size_t i = 0; i < shape.size(); ++i) {
            if (shape[i] == -1) {
                continue;
            }
            ok &= shape[i] == array.shape()[i];
        }
        if (ok) { 
            foundShape = true;
            break;
        }
    }

    if (!foundShape) {
        std::stringstream ss;
        ss << "Expected \"" + name + "\" with shape ";

        for (size_t i = 0; i < shapes.size(); ++i) { 
            ss << "(";
            auto& shape = shapes[i];
            for (size_t k = 0; k < shape.size(); ++k) { 
                ss << shape[k]; 
                if (k != shape.size() - 1) { 
                    ss <<  ", ";
                }
            }
            ss << ")";
            if (i != shapes.size() - 1) { 
                ss << " | ";
            }
        }

        ss << ", but found " << shapeToStr(array) << std::endl;

        throw std::runtime_error(ss.str());
    }
}

ImVec4 interpretColor(py::handle& color, bool* isArray) {

    std::string typeName = py::str(color.attr("__class__").attr("__name__"));

    if (typeName == "str") {
        std::string sc = py::str(color);

        if (sc.size() == 1) {
            // single char color codes
            if (sc == "r") {
                return ImVec4(1.0, 0.0, 0.0, 1.0);
            } else if (sc == "g") {
                return ImVec4(0.0, 1.0, 0.0, 1.0);
            } else if (sc == "b") {
                return ImVec4(0.0, 0.0, 1.0, 1.0);
            } else if (sc == "y") {
                return ImVec4(1.0, 1.0, 0.0, 1.0);
            } else if (sc == "c") {
                return ImVec4(0.0, 1.0, 1.0, 1.0);
            } else if (sc == "m") {
                return ImVec4(1.0, 0.0, 1.0, 1.0);
            } else if (sc == "k") {
                return ImVec4(0.0, 0.0, 0.0, 1.0);
            } else if (sc == "w") {
                return ImVec4(1.0, 1.0, 1.0, 1.0);
            }
        } else if (sc.size() > 1) {
            if (sc[0] == '#') {
                // css like hex colors
                size_t pos = 0;
                size_t colorNumber = std::stoul(sc.substr(1), &pos, 16);

                ImVec4 c(0.0, 0.0, 0.0, 1.0);

                if (sc.size() == 9) {
                    // alpha 
                    c.w = (double)(colorNumber & 0xff) / 255.0;
                    colorNumber >>= 8;
                }
                // blue
                c.z = (double)(colorNumber & 0xff) / 255.0;
                colorNumber >>= 8;
                // green
                c.y = (double)(colorNumber & 0xff) / 255.0;
                colorNumber >>= 8;
                // red
                c.x = (double)(colorNumber & 0xff) / 255.0;
                colorNumber >>= 8;

                return c;
            } else {
                // full word colors
                if (sc == "red") {
                    return ImVec4(1.0, 0.0, 0.0, 1.0);
                } else if (sc == "green") {
                    return ImVec4(0.0, 1.0, 0.0, 1.0);
                } else if (sc == "blue") {
                    return ImVec4(0.0, 0.0, 1.0, 1.0);
                } else if (sc == "yellow") {
                    return ImVec4(1.0, 1.0, 0.0, 1.0);
                } else if (sc == "cyan") {
                    return ImVec4(0.0, 1.0, 1.0, 1.0);
                } else if (sc == "magenta") {
                    return ImVec4(1.0, 0.0, 1.0, 1.0);
                } else if (sc == "white") {
                    return ImVec4(1.0, 1.0, 1.0, 1.0);
                } else if (sc == "key") {
                    return ImVec4(0.0, 0.0, 0.0, 1.0);
                } else if (sc == "black") {
                    return ImVec4(0.0, 0.0, 0.0, 1.0);
                } else if (sc == "gray") {
                    return ImVec4(0.5, 0.5, 0.5, 1.0);
                } else if (sc == "grey") {
                    return ImVec4(0.5, 0.5, 0.5, 1.0);
                }
            }
        }

        return IMPLOT_AUTO_COL;
    } else if (typeName == "float") {
        float f = py::cast<float>(color);
        return ImVec4(f, f, f, 1.0f);
    } else if (typeName == "int") { 
        float f = std::max(0, std::min(255, py::cast<int>(color)));
        f /= 255.0;
        return ImVec4(f, f, f, 1.0f);
    }

    array_like<float> colorArray = array_like<float>::ensure(color);

    assert_shape(colorArray, {{-1}, {-1, 4}});

    if (colorArray.ndim() == 2 && isArray != nullptr) {
        *isArray = true;
        return ImVec4(1, 1, 1, 1);
    }

    ImVec4 c(0, 0, 0, 1);
    size_t colorLength = colorArray.shape()[0];

    if (colorLength == 1) {
        c.x = colorArray.data()[0];
        c.y = colorArray.data()[0];
        c.z = colorArray.data()[0];
    } else if (colorLength == 3) {
        c.x = colorArray.data()[0];
        c.y = colorArray.data()[1];
        c.z = colorArray.data()[2];
    } else if (colorLength == 4) {
        c.x = colorArray.data()[0];
        c.y = colorArray.data()[1];
        c.z = colorArray.data()[2];
        c.w = colorArray.data()[3];
    } else {
        c = IMPLOT_AUTO_COL;
    }

    return c;
}

ImageInfo interpretImage(py::array& image) {

    assert_shape(image, {{-1, -1}, {-1, -1, 1}, {-1, -1, 3}, {-1, -1, 4}});

    // determine image parameters
    
    ImageInfo i;

    if (image.ndim() == 2) {
        i.imageWidth = image.shape(1);
        i.imageHeight = image.shape(0);
        i.channels = 1;
    } else if (image.ndim() == 3) {
        i.imageWidth = image.shape(1);
        i.imageHeight = image.shape(0);
        i.channels = image.shape(2);
    }

    if (i.channels == 1) {
        i.format = GL_RED;
    } else if (i.channels == 3) {
        i.format = GL_RGB;
    } else if (i.channels == 4) {
        i.format = GL_RGBA;
    } 

    if (py::str(image.dtype()).equal(py::str("uint8"))) {
        i.datatype = GL_UNSIGNED_BYTE;
    } else if (py::str(image.dtype()).equal(py::str("float32"))) {
        i.datatype = GL_FLOAT;
    } else {
        i.datatype = GL_FLOAT;
    }

    return i;
}

GLuint uploadImage(std::string id, ImageInfo& i, py::array& image, bool skip, bool lerp) {

    static std::unordered_map<ImGuiID, GLuint> textureCache;

    ImGuiID uniqueId = ImGui::GetID(id.c_str());

    // create a texture if necessary
    
    if (textureCache.find(uniqueId) == textureCache.end()) {

        GLuint textureId;

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        glBindTexture(GL_TEXTURE_2D, 0);

        textureCache[uniqueId] = textureId;

        skip = false;
    }

    // upload texture

    GLuint textureId = textureCache[uniqueId];

    if (skip) {
        return textureId;
    }

    glBindTexture(GL_TEXTURE_2D, textureId);

    if (i.format == GL_RED) {
        GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    } else if (i.format == GL_RGB) {
        GLint swizzleMask[] = {GL_RED, GL_GREEN, GL_BLUE, GL_ONE};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    } else if (i.format == GL_RGBA) {
        GLint swizzleMask[] = {GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }

    // setup parameters for display

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (lerp) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    }

    if (i.datatype == GL_UNSIGNED_BYTE) {
        image = array_like<uint8_t>::ensure(image);
    } else if (i.datatype == GL_FLOAT) {
        image = array_like<float>::ensure(image);
    }

    glTexImage2D(
            GL_TEXTURE_2D,
            0,
            i.format,
            i.imageWidth,
            i.imageHeight,
            0,
            i.format,
            i.datatype,
            image.data());

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    return textureId;
}

PlotArrayInfo interpretPlotArrays(
        array_like<double>& x,
        array_like<double>& y) {

    PlotArrayInfo info;

    size_t yCount = y.shape()[0];

    if (1 == x.ndim() && 0 == yCount) {
        // one 1d array given
        // assume x is [0, 1, 2, ..., N]
        info.count = x.shape()[0];
        info.indices.resize(info.count);
        for (size_t i = 0; i < info.count; ++i) {
            info.indices[i] = i;
        }
        info.xDataPtr = info.indices.data();
        info.yDataPtr = x.data();
    } else if (2 == x.ndim() && 0 == yCount) {
        // one 2d array given
        size_t len0 = x.shape()[0];
        size_t len1 = x.shape()[1];
        if (len0 == 2) {
            info.xDataPtr = x.data();
            info.yDataPtr = x.data() + len1;
            info.count = len1;
        }
    } else if (1 == x.ndim() && 1 == y.ndim()) {
        // two 2d arrays given
        info.count = std::min(x.shape()[0], y.shape()[0]);
        info.xDataPtr = x.data();
        info.yDataPtr = y.data();
    } else {
        throw std::runtime_error(
                "Plot data with x-shape "
                + shapeToStr(x)
                + " and y-shape "
                + shapeToStr(y)
                + " cannot be interpreted");
    }

    return info;
}
