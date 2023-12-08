#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(const Timer* pTimer);

		void ToggleDepthBuffer();
		void CycleLightingMode();
		void ToggleUseNormals();
		void ToggleRotation();

		void Render();

		bool SaveBufferToImage() const;

		void VertexTransformationFunction(const Matrix& world, const Matrix& worldViewProjectionMatrix, const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const;
		ColorRGB ShadePixel(const Sample& sample) const;

	private:
		enum class LightingMode
		{
			ObservedArea,
			Diffuse,
			Specular,
			Combined,

			enumSize
		};
		LightingMode m_CurrentLightingMode{ LightingMode::Combined };

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};
		//ColorRGB m_ClearColor{};
		Uint8 r{ 100 };
		Uint8 g{ 100 };
		Uint8 b{ 100 };
		Uint32 m_ClearColor{ static_cast<Uint32>(255 << 24) + static_cast<Uint32>(r << 16) + static_cast<Uint32>(g << 8) + static_cast<Uint32>(b) };

		std::vector<Mesh> m_Meshes{};
		Texture* m_TexturePtr{};

		Texture* m_VehicleDiffusePtr{};
		Texture* m_VehicleGlossPtr{};
		Texture* m_VehicleNormalPtr{};
		Texture* m_VehicleSpecularPtr{};

		float m_TotalRotation{};

		bool m_isDepthBuffer{};
		bool m_IsDepthBuffer{};
		bool m_ShouldSpin{ true };
		bool m_Normalz{ true };

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};
	};
}