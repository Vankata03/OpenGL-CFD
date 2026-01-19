#pragma once

#include <string>
#include <memory>
#include "Mesh.h"

class SceneImporter {
public:
    // Loads a glTF file and merges all meshes into a single Mesh object
    static std::unique_ptr<Mesh> LoadGLTF(const std::string& filepath);
};