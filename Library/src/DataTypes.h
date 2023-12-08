#pragma once
#include "Maths.h"
#include "vector"

namespace dae
{
	struct Vertex
	{
		Vector4 position{};
		ColorRGB color{colors::White};
		Vector2 uv{}; //W2
		bool valid{ true };
		Vector3 normal{}; //W4
		Vector3 tangent{}; //W4
		Vector3 viewDirection{}; //W4
	};

	struct Sample
	{
		Vector2 uv{}; //W2
		Vector3 normal{}; //W4
		Vector3 tangent{}; //W4
		Vector3 viewDirection{}; //W4
		float depth{};
		Vector3 weight{};
	};

	enum class PrimitiveTopology
	{
		TriangleList,
		TriangleStrip
	};

	struct Mesh
	{
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleList };

		std::vector<Vertex> vertices_out{};
		Matrix worldMatrix{};
	};
}
