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
        image = array_like<float>::ensure(image);
    }

    return i;
}

GLuint uploadImage(std::string id, ImageInfo& i, py::array& image) {

    static std::unordered_map<ImGuiID, GLuint> textureCache;

    ImGuiID uniqueId = ImGui::GetID(id.c_str());

    // create a texture if necessary
    
    if (textureCache.find(uniqueId) == textureCache.end()) {

        GLuint textureId;

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        glBindTexture(GL_TEXTURE_2D, 0);

        textureCache[uniqueId] = textureId;
    }

    // upload texture

    GLuint textureId = textureCache[uniqueId];

    glBindTexture(GL_TEXTURE_2D, textureId);

    if (i.format == GL_RED) {
        GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }

    // setup parameters for display

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

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
