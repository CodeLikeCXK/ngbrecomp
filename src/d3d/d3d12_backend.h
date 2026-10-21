#pragma once
#include <memory>
#include "gpu/backend.h"

namespace gpu {
std::unique_ptr<Backend> CreateD3D12Backend();
}
