#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
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
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};

		bool updateONB{ true };

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;
		}

		void CalculateViewMatrix()
		{
			if (!updateONB) return;
			//TODO W1
			//ONB => invViewMatrix
			//Inverse(ONB) => ViewMatrix

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh

			
			// need this otherwise camera movement is whacky
            Matrix rotationMatrix = Matrix::CreateRotationX(totalPitch * TO_RADIANS) * Matrix::CreateRotationY(totalYaw * TO_RADIANS);

            forward = rotationMatrix.GetAxisZ();
            right = rotationMatrix.GetAxisX();
            up = rotationMatrix.GetAxisY();
			
            /*right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
            up = Vector3::Cross(forward, right).Normalized();*/

			//Calculate ViewMatrix/ONB
			invViewMatrix = Matrix::CreateLookAtLH(origin, forward, up); //ONB
			viewMatrix = Matrix::Inverse(invViewMatrix);
            updateONB = false;
		}

		void CalculateProjectionMatrix()
		{
			//TODO W2

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Camera Update Logic
			InputLogic(pTimer);

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}

		void InputLogic(Timer* pTimer)
		{
            const float movementSpeed{ 7.0f };
            const float rotationSpeed{ 20.0f };
            const float keyboardRotationSpeed{ 80.0f };
			
            const float deltaTime = pTimer->GetElapsed();

            //Keyboard Input
            const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

            // Keyboard movement of the camera
            if (pKeyboardState[SDL_SCANCODE_W])
            {
                origin += forward * movementSpeed * deltaTime;
            }
            if (pKeyboardState[SDL_SCANCODE_S])
            {
                origin -= forward * movementSpeed * deltaTime;
            }
            if (pKeyboardState[SDL_SCANCODE_D])
            {
                origin += right * movementSpeed * deltaTime;
            }
            if (pKeyboardState[SDL_SCANCODE_A])
            {
                origin -= right * movementSpeed * deltaTime;
            }
            if (pKeyboardState[SDL_SCANCODE_SPACE])
            {
                origin += up * movementSpeed * deltaTime;
            }
            if (pKeyboardState[SDL_SCANCODE_LSHIFT])
            {
                origin -= up * movementSpeed * deltaTime;
            }
            if (pKeyboardState[SDL_SCANCODE_UP])
            {
                totalPitch += keyboardRotationSpeed * deltaTime;
            }
            if (pKeyboardState[SDL_SCANCODE_DOWN])
            {
                totalPitch -= keyboardRotationSpeed * deltaTime;
            }
            if (pKeyboardState[SDL_SCANCODE_LEFT])
            {
                totalYaw -= keyboardRotationSpeed * deltaTime;
            }
            if (pKeyboardState[SDL_SCANCODE_RIGHT])
            {
                totalYaw += keyboardRotationSpeed * deltaTime;
            }
            //Mouse Input
            int mouseX{}, mouseY{};
            const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

            // Mouse movements / rotation of the camera
            if ((mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) && mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
            {
                // mouseX yaw left & right, mouse Y moves forwards & backwards
                const float upwards = -mouseY * movementSpeed * deltaTime;
                origin += up * upwards;
                updateONB = true;
            }
            else if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
            {
                // mouseX yaw left & right, mouse Y moves forwards & backwards
                const float forwards = -mouseY * deltaTime;
                const float yaw = mouseX * deltaTime;

                origin += forward * forwards;
                totalYaw += yaw;
                updateONB = true;
            }
            else if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
            {
                // Look around the current origin
                const float pitch = -mouseY * rotationSpeed * deltaTime;
                const float yaw = mouseX * rotationSpeed * deltaTime;

                totalPitch += pitch;
                totalYaw += yaw;
                updateONB = true;
            }

            const Matrix finalRotation = Matrix::CreateRotationX(totalPitch * TO_RADIANS) * Matrix::CreateRotationY(totalYaw * TO_RADIANS);
            forward = finalRotation.TransformVector(Vector3::UnitZ);
            forward.Normalize();
		}
	};
}
