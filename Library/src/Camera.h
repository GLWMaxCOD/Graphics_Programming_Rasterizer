#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle) :
			origin{ _origin },
			fovAngle{ _fovAngle }
		{
		}


		Vector3 origin{};
		float fovAngle{ 90.f };
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };
		float aspectRatio{};

		float cameraSpeed{ 2.f };
		float mouseSensitivity{ 0.25f };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float zNear{ .1f };
		float zFar{ 100.f };

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = { 0.f,0.f,0.f })
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;
		}

		void CalculateViewMatrix()
		{
			//TODO W1
			//ONB => invViewMatrix
			right = Vector3::Cross({ 0, 1, 0 }, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();

			invViewMatrix = Matrix{
				Vector4{right, 0},
				Vector4{up, 0},
				Vector4{forward, 0},
				Vector4{origin, 1}
			};

			//Inverse(ONB) => ViewMatrix
			viewMatrix = invViewMatrix.Inverse();

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			//TODO W3
			
			float near{ 0.1f };
			float far{ 100 };

			projectionMatrix = Matrix{
				Vector4{1 / (aspectRatio * fov), 0, 0, 0},
				Vector4{0, 1 / fov, 0, 0},
				Vector4{0, 0, far / (far - near), 1},
				Vector4{0, 0, -(far * near) / (far - near), 0}
			};

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(const Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Camera Update Logic
			//...
			//Add any additional keyboard input here (DOUBLECHECK BEFORE HAND-IN!!!)
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);
			Vector3 zDirection{ 0.f, 0.f, 0.f };
			const Vector3 xDirection{ 0.f, 0.f, 0.f };
			const Vector3 yDirection{ 0.f, 0.f, 0.f };

			float movementSpeed{ 10.f };

			if (pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_UP])
			{
				origin += forward * deltaTime * movementSpeed;
			}
			if (pKeyboardState[SDL_SCANCODE_A] || pKeyboardState[SDL_SCANCODE_LEFT])
			{
				origin -= right * deltaTime * movementSpeed;
			}
			if (pKeyboardState[SDL_SCANCODE_S] || pKeyboardState[SDL_SCANCODE_DOWN])
			{
				origin -= forward * deltaTime * movementSpeed;
			}
			if (pKeyboardState[SDL_SCANCODE_D] || pKeyboardState[SDL_SCANCODE_RIGHT])
			{
				origin += right * deltaTime * movementSpeed;
			}
			if (pKeyboardState[SDL_SCANCODE_Q])
			{
				origin -= up * deltaTime * movementSpeed;
			}
			if (pKeyboardState[SDL_SCANCODE_E])
			{
				origin += up * deltaTime * movementSpeed;
			}

			if (pKeyboardState[SDL_SCANCODE_LSHIFT])
			{
				movementSpeed *= 2; //Upon pressing LeftShift double the movementSpeed
 			}

			//Mouse input (LMB, RMB, etc)
			bool mousePosChange{ false };
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);
			mouseY *= -1; // Invert Y-axis for consistent coordinate system

			const float MaxPitch = 89.0f * TO_RADIANS; // Maximum pitch (looking up)
			const float MinPitch = -89.0f * TO_RADIANS; // Minimum pitch (looking down)

			if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
			{
				totalYaw += static_cast<float>(mouseX) * mouseSensitivity * TO_RADIANS;
				totalPitch += static_cast<float>(mouseY) * mouseSensitivity * TO_RADIANS;

				// Clamp pitch to avoid over-rotation
				totalPitch = std::max(std::min(totalPitch, MaxPitch), MinPitch);

				mousePosChange = true;
			}
			else if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
			{
				// Adjust zDirection based on mouse movement
				zDirection = forward.Normalized() * static_cast<float>(-mouseY);
				totalYaw += static_cast<float>(mouseX) * mouseSensitivity * TO_RADIANS;

				mousePosChange = true;
			}
			else if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT) && SDL_BUTTON(SDL_BUTTON_RIGHT))
			{
				origin.y += static_cast<float>(mouseY) / 2;

				mousePosChange = true;
			}

			// Update camera orientation based on mouse movement
			if (mousePosChange)
			{
				const Matrix yawMatrix{ Matrix::CreateRotationY(totalYaw) };
				const Matrix pitchMatrix{ Matrix::CreateRotationX(totalPitch) };

				const Matrix finalRotation{ pitchMatrix * yawMatrix };
				forward = finalRotation.TransformVector(Vector3::UnitZ);
				forward.Normalize();
			}

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
