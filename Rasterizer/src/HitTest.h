#pragma once
#include <optional>

#include "DataTypes.h"

namespace HitTest
{
    std::optional<dae::Sample> Trongle(const dae::Vector3& fragPos, const dae::Vertex& v0, const dae::Vertex& v1, const dae::Vertex& v2);
}