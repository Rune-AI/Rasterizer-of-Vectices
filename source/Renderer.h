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

	enum Rendermodes
	{
		ObservedArea,
		Diffuse,
		Specular,
		Ambient,
		Combined
	};

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		void Render_W1_Part1(); //Rasterizer Stage Only
		void Render_W1_Part2(); //Projection Stage (Camera)
		void Render_W1_Part3(); //Barycentric Coordinates
		void Render_W1_Part4(); //Depth Buffer
		void Render_W1_Part5(); //BoundingBox Optimization

		void Render_W2_Part1(); //TriangleList
		void Render_W2_Part2(); //TriangleStrip
		void Render_W2_Part3(); //Texture, Bounding box fix, Improved depth buffer

		void Render_W3_Part1(); //TUKTUK

		void Render_W4_Part1(); //Pixel Shading


		void InputLogic(const SDL_Event& e);

		void PrintInstructions() const;

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		bool m_RenderBoundingBox{ false }; //F3
		bool m_RenderDepth{ false }; //F4
		bool m_RotateMeshes{ true }; //F5
		bool m_RenderNormalMap{ true }; //F6
		Rendermodes m_RenderMode{ Rendermodes::Combined }; //F7
		
		Camera m_Camera{};

		int m_Width{};
		int m_Height{};

		Texture* m_pTexture{};
		Texture* m_pDiffuseTexture{};
		Texture* m_pGlossTexture{};
		Texture* m_pNormalTexture{};
		Texture* m_pSpecularTexture{};

		std::vector<Mesh> m_Meshes{};

		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const; //W1 Version
		void VertexTransformationFunction(const std::vector<Mesh>& mesh_in, std::vector<Mesh>& mesh_out) const; //W2 Version
		void VertexTransformationFunction(std::vector<Mesh>& meshes) const;

		ColorRGB PixelShading(const Vertex_Out& v) const;
		

	};
}
