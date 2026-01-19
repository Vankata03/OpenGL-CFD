#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tiny_gltf.h>
#include "SceneImporter.h"
#include <iostream>

std::unique_ptr<Mesh> SceneImporter::LoadGLTF(const std::string& filepath) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = false;
    if (filepath.find(".glb") != std::string::npos) {
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);
    } else {
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
    }

    if (!warn.empty()) {
        std::cout << "GLTF WARN: " << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << "GLTF ERR: " << err << std::endl;
    }

    if (!ret) {
        std::cerr << "Failed to parse glTF: " << filepath << std::endl;
        return nullptr;
    }

    std::vector<Vertex> globalVertices;
    std::vector<unsigned int> globalIndices;

    // Iterate over all meshes and primitives to flatten the geometry
    for (const auto& mesh : model.meshes) {
        for (const auto& primitive : mesh.primitives) {
            if (primitive.attributes.find("POSITION") == primitive.attributes.end())
                continue;

            // Position Accessor
            const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
            const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
            const tinygltf::Buffer& posBuffer = model.buffers[posView.buffer];

            const unsigned char* posData = &posBuffer.data[posView.byteOffset + posAccessor.byteOffset];
            int posStride = posAccessor.ByteStride(posView) ? posAccessor.ByteStride(posView) : sizeof(float) * 3;

            // Normal Accessor
            const unsigned char* normData = nullptr;
            int normStride = 0;
            if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.at("NORMAL")];
                const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
                const tinygltf::Buffer& normBuffer = model.buffers[normView.buffer];

                normData = &normBuffer.data[normView.byteOffset + normAccessor.byteOffset];
                normStride = normAccessor.ByteStride(normView) ? normAccessor.ByteStride(normView) : sizeof(float) * 3;
            }

            // TexCoord Accessor
            const unsigned char* uvData = nullptr;
            int uvStride = 0;
            if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
                const tinygltf::Buffer& uvBuffer = model.buffers[uvView.buffer];

                uvData = &uvBuffer.data[uvView.byteOffset + uvAccessor.byteOffset];
                uvStride = uvAccessor.ByteStride(uvView) ? uvAccessor.ByteStride(uvView) : sizeof(float) * 2;
            }

            size_t vertexStart = globalVertices.size();

            // Read Vertices
            for (size_t i = 0; i < posAccessor.count; ++i) {
                Vertex vertex;

                const float* p = reinterpret_cast<const float*>(posData + i * posStride);
                vertex.Position = glm::vec3(p[0], p[1], p[2]);

                if (normData) {
                    const float* n = reinterpret_cast<const float*>(normData + i * normStride);
                    vertex.Normal = glm::vec3(n[0], n[1], n[2]);
                } else {
                    vertex.Normal = glm::vec3(0.0f);
                }

                if (uvData) {
                    const float* uv = reinterpret_cast<const float*>(uvData + i * uvStride);
                    vertex.TexCoords = glm::vec2(uv[0], uv[1]);
                } else {
                    vertex.TexCoords = glm::vec2(0.0f);
                }

                globalVertices.push_back(vertex);
            }

            // Read Indices
            if (primitive.indices >= 0) {
                const tinygltf::Accessor& idxAccessor = model.accessors[primitive.indices];
                const tinygltf::BufferView& idxView = model.bufferViews[idxAccessor.bufferView];
                const tinygltf::Buffer& idxBuffer = model.buffers[idxView.buffer];

                const unsigned char* idxData = &idxBuffer.data[idxView.byteOffset + idxAccessor.byteOffset];
                int idxByteStride = idxAccessor.ByteStride(idxView);

                for (size_t i = 0; i < idxAccessor.count; ++i) {
                    unsigned int index = 0;
                    const unsigned char* ptr = idxData + i * (idxByteStride ? idxByteStride :
                        (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? 2 :
                        (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT ? 4 : 1)));

                    if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                        index = (unsigned int)(*reinterpret_cast<const unsigned short*>(ptr));
                    } else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                        index = *reinterpret_cast<const unsigned int*>(ptr);
                    } else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                        index = (unsigned int)(*reinterpret_cast<const unsigned char*>(ptr));
                    }
                    globalIndices.push_back(index + (unsigned int)vertexStart);
                }
            } else {
                // Non-indexed geometry
                 for (size_t i = 0; i < posAccessor.count; ++i) {
                     globalIndices.push_back((unsigned int)(vertexStart + i));
                 }
            }
        }
    }

    auto mesh = std::make_unique<Mesh>(globalVertices, globalIndices);

    // Load first texture if exists
    if (!model.textures.empty()) {
        const tinygltf::Texture& tex = model.textures[0];
        if (tex.source > -1 && tex.source < (int)model.images.size()) {
            const tinygltf::Image& image = model.images[tex.source];

            unsigned int textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            GLenum format = GL_RGBA;
            if (image.component == 3) format = GL_RGB;
            else if (image.component == 1) format = GL_RED;

            glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0, format, GL_UNSIGNED_BYTE, &image.image[0]);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            mesh->SetTexture(textureID);
        }
    }

    return mesh;
}
