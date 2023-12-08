#include "HitTest.h"

#include <complex>

using namespace dae;

float CrossZ(const Vector3& p0, const Vector3& p1, const Vector3& point)
{
    return (p1.x - p0.x) * (point.y - p0.y)
        - (p1.y - p0.y) * (point.x - p0.x);
}

std::optional<Sample> HitTest::Trongle(const Vector3& fragPos, const Vertex& v0, const Vertex& v1, const Vertex& v2)
{
    Vector3 weights;

    weights.x = CrossZ(v2.position, v1.position, fragPos);
    if (weights.x > 0)
        return std::nullopt;

    weights.y = CrossZ(v0.position, v2.position, fragPos);
    if (weights.y > 0)
        return std::nullopt;

    weights.z = CrossZ(v1.position, v0.position, fragPos);
    if (weights.z > 0)
        return std::nullopt;

    const float totalWeight{ weights.x + weights.y + weights.z };

    // normalize
    const Vector3 normWeights{
        weights.x / totalWeight,
        weights.y / totalWeight,
        weights.z / totalWeight,
    };

    const float depth =
        1 / (normWeights.x / v0.position.w +
            normWeights.y / v1.position.w +
            normWeights.z / v2.position.w);

    const Vector2 uv =
        v0.uv * depth * normWeights.x / v0.position.w +
        v1.uv * depth * normWeights.y / v1.position.w +
        v2.uv * depth * normWeights.z / v2.position.w;

    auto interpolate =
        [&]<typename T>(const T & val0, const T & val1, const T & val2) -> T
    {
        return {
            val0 * depth * weights.x +
            val1 * depth * weights.y +
            val2 * depth * weights.z
        };
    };

    const Vector3 normal = interpolate(v0.normal, v1.normal, v2.normal).Normalized();
    const Vector3 tangent = interpolate(v0.tangent, v1.tangent, v2.tangent).Normalized();
    const Vector3 viewDir = interpolate(v0.viewDirection, v1.viewDirection, v2.viewDirection).Normalized();

    return Sample{ uv, normal, tangent, viewDir, depth, normWeights };
}