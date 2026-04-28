#pragma once

#include <filesystem>

#include "cloth/core/Cloth.h"

namespace io
{
Cloth load_cloth(const std::filesystem::path& path);
}
