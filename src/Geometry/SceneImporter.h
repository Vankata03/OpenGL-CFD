#pragma once

#include <string>
#include <memory>
#include "Mesh.h"

class SceneImporter {
public:
    static std::unique_ptr<Mesh> LoadGLTF(const std::string& filepath);
};
