//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"

#include <chrono>
#include <iostream>

#include "HitTest.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = static_cast<uint32_t*>(m_pBackBuffer->pixels);

	m_pDepthBufferPixels = new float[static_cast<int>(m_Width * m_Height)];

	//Initialize Camera
	m_Camera.Initialize(45.f, { 0.f, 5.f, -64.f });
	m_Camera.aspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);

	//Initialize Mesh
	Mesh tempMesh{};
	//Utils::ParseOBJ("Resources/tuktuk.obj", tempMesh.vertices, tempMesh.indices);
	Utils::ParseOBJ("Resources/vehicle.obj", tempMesh.vertices, tempMesh.indices);

	m_Meshes.push_back(tempMesh);

	//m_VehicleDiffusePtr = Texture::LoadFromFile("./Resources/tuktuk.png");
	m_VehicleDiffusePtr = Texture::LoadFromFile("./Resources/vehicle_diffuse.png");
	m_VehicleGlossPtr = Texture::LoadFromFile("./Resources/vehicle_gloss.png");
	m_VehicleNormalPtr = Texture::LoadFromFile("./Resources/vehicle_normal.png");
	m_VehicleSpecularPtr = Texture::LoadFromFile("./Resources/vehicle_specular.png");
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;

	delete m_VehicleDiffusePtr;
	delete m_VehicleGlossPtr;
	delete m_VehicleNormalPtr;
	delete m_VehicleSpecularPtr;
}

void Renderer::Update(const Timer* pTimer)
{
	m_Camera.Update(pTimer);

	if (m_ShouldSpin)
	{
		m_TotalRotation += pTimer->GetElapsed();
		m_Meshes[0].worldMatrix = Matrix::CreateRotationY(m_TotalRotation);
	}
}

void Renderer::Render()
{
	//@START
	// Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);
	SDL_FillRect(m_pBackBuffer, nullptr, m_ClearColor);
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, std::numeric_limits<float>::max());

	// RENDER LOGIC
	ColorRGB finalColor{};
	constexpr int numVertices{ 3 };

	std::vector<Vertex> screenSpaceVec{};
	for (Mesh& currentMesh : m_Meshes)
	{
		const Matrix worldViewProjectionMatrix{ currentMesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };

		VertexTransformationFunction(currentMesh.worldMatrix, worldViewProjectionMatrix, currentMesh.vertices, currentMesh.vertices_out);

		int numTriangles;
		Vertex vertex0, vertex1, vertex2;

		switch (currentMesh.primitiveTopology)
		{
		case PrimitiveTopology::TriangleList:
			numTriangles = static_cast<int>(currentMesh.indices.size()) / numVertices;
			break;
		case PrimitiveTopology::TriangleStrip:
			numTriangles = static_cast<int>(currentMesh.indices.size()) - 2;
			break;
		default:
			abort();
		}

		for (int triangleIndex{}; triangleIndex < numTriangles; triangleIndex++)
		{
			Vertex vertex0, vertex1, vertex2;

			switch (currentMesh.primitiveTopology)
			{
			case PrimitiveTopology::TriangleList:
			{
				vertex0 = currentMesh.vertices_out[currentMesh.indices[triangleIndex * numVertices + 0]];
				vertex1 = currentMesh.vertices_out[currentMesh.indices[triangleIndex * numVertices + 1]];
				vertex2 = currentMesh.vertices_out[currentMesh.indices[triangleIndex * numVertices + 2]];
			}
			break;
			case PrimitiveTopology::TriangleStrip:
			{
				vertex0 = currentMesh.vertices_out[currentMesh.indices[triangleIndex + 0]];
				vertex1 = currentMesh.vertices_out[currentMesh.indices[triangleIndex + 1]];
				vertex2 = currentMesh.vertices_out[currentMesh.indices[triangleIndex + 2]];

				if (triangleIndex % 2 == 1)
					std::swap(vertex1, vertex2);

				if (vertex0.position == vertex1.position ||
					vertex0.position == vertex2.position ||
					vertex1.position == vertex2.position)
					continue;
			}
			break;
			default:
				abort();
			}

			// Ensure counterclockwise winding order
			Vector3 normal = Vector3::Cross(vertex1.position - vertex0.position, vertex2.position - vertex0.position);
			float triangleOrientation = Vector3::Dot(normal, m_Camera.forward);

			if (triangleOrientation < 0.0f)
			{
				// Swap vertices to enforce counterclockwise winding order
				std::swap(vertex1, vertex2);
			}

			if (!vertex0.valid || !vertex1.valid || !vertex2.valid)
				continue;

			// Generalized logic for bounding box
			int xMin = static_cast<int>(std::min(vertex0.position.x, std::min(vertex1.position.x, vertex2.position.x)));
			int xMax = static_cast<int>(std::max(vertex0.position.x, std::max(vertex1.position.x, vertex2.position.x)));
			int yMin = static_cast<int>(std::min(vertex0.position.y, std::min(vertex1.position.y, vertex2.position.y)));
			int yMax = static_cast<int>(std::max(vertex0.position.y, std::max(vertex1.position.y, vertex2.position.y)));

			if (xMin < 0) continue; else xMin -= 1;
			if (yMin < 0) continue; else yMin -= 1;

			if (xMax > m_Width) continue; else xMax += 1;
			if (yMax > m_Height) continue; else yMax += 1;

			// RENDER LOGIC
			for (int px{ xMin }; px < xMax; ++px)
			{
				if (px < 0) continue;

				for (int py{ yMin }; py < yMax; ++py)
				{
					if (py < 0) continue;

					Vector3 point{ px + 0.5f, py + 0.5f, 0.f };

					std::optional<Sample> sample = HitTest::Trongle(point, vertex0, vertex1, vertex2);

					if (!sample.has_value())
						continue;

					const int depthBufferIndex{ px + (py * m_Width) };

					float min{ .985f };
					float max{ 1.f };
					// Depth buffer calculation
					float depthBuffer = (sample.value().depth - min) / (max - min);

					// Depth buffer update
					if (depthBuffer < m_pDepthBufferPixels[depthBufferIndex])
					{
						m_pDepthBufferPixels[depthBufferIndex] = depthBuffer;

						// Update Color in Buffer
						if (m_IsDepthBuffer)
						{
							// Normalize depth value for visualization
							float normalizedDepth = (depthBuffer - 0.985f) / (1.0f - 0.985f);

							// Map normalized depth to greyscale color
							finalColor = ColorRGB{ normalizedDepth, normalizedDepth, normalizedDepth };
							finalColor.MaxToOne(); // Ensure values are in the valid color range
						}
						else
						{
							finalColor = ShadePixel(sample.value());
						}

						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));
					}
				}
			}
		}
	}
	//@END
	// Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, nullptr, m_pFrontBuffer, nullptr);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const Matrix& world, const Matrix& worldViewProjectionMatrix, const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	vertices_out.clear();

	for (const Vertex& vert : vertices_in)
	{
		Vertex ret{ vert };

		Vector4 vertPos{ worldViewProjectionMatrix.TransformPoint({ret.position, 1}) };

		const Vector3 normal{ world.TransformPoint(ret.normal) };
		const Vector3 tangent{ world.TransformPoint(ret.tangent) };

		// Add perspective
		vertPos.x /= vertPos.w;
		vertPos.y /= vertPos.w;
		vertPos.z /= vertPos.w;

		if (vertPos.x < -1.f || vertPos.x > 1.f ||
			vertPos.y < -1.f || vertPos.y > 1.f ||
			vertPos.z < 0.f || vertPos.z > 1.f)
			ret.valid = false;

		//ndc to screen
		vertPos.x = ((vertPos.x + 1.f) / 2.f) * static_cast<float>(m_Width);
		vertPos.y = ((1.f - vertPos.y) / 2.f) * static_cast<float>(m_Height);

		ret.position = vertPos;
		ret.normal = normal;
		ret.tangent = tangent;
		ret.viewDirection = Vector3(vertPos) - m_Camera.origin;

		vertices_out.push_back(ret);
	}
}

ColorRGB Renderer::ShadePixel(const Sample& sample) const
{
	const Vector3 lightDirection{ .577f, -.577f, .577f };
	constexpr float lightIntensity{ 7.f };

	ColorRGB color{ 1, 1, 1 };
	constexpr ColorRGB ambient{ .03f, .03f, .03f };

	Vector3 normal = sample.normal;

	if (m_Normalz)
	{
		const ColorRGB normalSampleColor{ m_VehicleNormalPtr->Sample(sample.uv) };
		const Vector4 normalSample{
			2.f * normalSampleColor.r - 1.f,
			2.f * normalSampleColor.g - 1.f,
			2.f * normalSampleColor.b - 1.f,
			0.f };

		const Vector3 binormal{ Vector3::Cross(normal, sample.tangent) };
		const Matrix tangentSpaceAxis = Matrix{
			Vector4{ sample.tangent, 0.f },
			Vector4{ binormal, 0.f },
			Vector4{ normal, 0.f },
			Vector4{ 0.f, 0.f, 0.f, 1.f }
		};

		normal = tangentSpaceAxis.TransformVector(normalSample);
	}

	const float cosAngle{ std::max(0.f,  Vector3::Dot(normal, lightDirection)) };

	const ColorRGB diffuseSample{ m_VehicleDiffusePtr->Sample(sample.uv) };
	const ColorRGB lambert{ diffuseSample * lightIntensity / PI };

	float specularReflectance{ 1.f };
	float shininess{ 25.f };

	specularReflectance *= m_VehicleSpecularPtr->Sample(sample.uv).r;
	shininess += m_VehicleGlossPtr->Sample(sample.uv).r;

	const ColorRGB specular = specularReflectance * powf(std::max(0.f, cosAngle), shininess) * colors::White;

	switch (m_CurrentLightingMode)
	{
	case LightingMode::ObservedArea:
	{
		break;
	}
	case LightingMode::Diffuse:
	{
		color = lambert;
		break;
	}
	case LightingMode::Specular:
	{
		color = specular;
		break;
	}
	case LightingMode::Combined:
	{
		color = lambert + specular + ambient;
		break;
	}
	}

	color *= ColorRGB{ cosAngle, cosAngle, cosAngle };

	return color;
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void Renderer::ToggleDepthBuffer()
{
	m_IsDepthBuffer = !m_IsDepthBuffer;
}

void Renderer::ToggleRotation()
{
	m_ShouldSpin = !m_ShouldSpin;
}

void Renderer::ToggleUseNormals()
{
	m_Normalz = !m_Normalz;
}

void Renderer::CycleLightingMode()
{
	m_CurrentLightingMode = LightingMode((int(m_CurrentLightingMode) + 1) % int(LightingMode::enumSize));
}
