//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
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
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f, .0f, -10.f });

	m_pTexture = Texture::LoadFromFile("Resources/uv_grid_2.png");
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//RENDER LOGIC
	//Render_W1_Part1(); //Rasterizer Stage Only
	//Render_W1_Part2(); //Projection Stage (Camera)
	//Render_W1_Part3(); //Barycentric Coordinates
	//Render_W1_Part4(); //Depth Buffer
	//Render_W1_Part5(); //BoundingBox Optimization

	//Render_W2_Part1(); //TriangleList
	//Render_W2_Part2(); //TriangleStrip
	Render_W2_Part3(); //Texture, Bounding box fix, Improved depth buffer

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	vertices_out = vertices_in;

	for (size_t i = 0; i < vertices_in.size(); i++)
	{
		const float aspectRatio = m_Width / static_cast<float>(m_Height);

		// Model space > World space
		// Not Implemented

		// World Space To View Space
		vertices_out[i].position =  m_Camera.viewMatrix.TransformPoint(vertices_out[i].position);

		// View Space To Projection Space
		// Camera settings
		vertices_out[i].position.x = vertices_out[i].position.x / (aspectRatio * m_Camera.fov);
		vertices_out[i].position.y = vertices_out[i].position.y / m_Camera.fov;
		// Perspective Divide
		vertices_out[i].position.x = vertices_out[i].position.x / vertices_out[i].position.z;
		vertices_out[i].position.y = vertices_out[i].position.y / vertices_out[i].position.z;
		
		// Projection Space To NDC/Screen Space
		vertices_out[i].position.x = (vertices_out[i].position.x + 1) / 2.f * m_Width;
		vertices_out[i].position.y = (1 - vertices_out[i].position.y) / 2.f * m_Height;
	}
}

void Renderer::VertexTransformationFunction(const std::vector<Mesh>& mesh_in, std::vector<Mesh>& mesh_out) const
{
	mesh_out = mesh_in;

	for (size_t meshIndex = 0; meshIndex < mesh_in.size(); meshIndex++)
	{
		for (size_t i = 0; i < mesh_in[meshIndex].vertices.size(); i++)
		{
			const float aspectRatio = m_Width / static_cast<float>(m_Height);

			// Model space > World space
			// Not Implemented

			// World Space To View Space
			mesh_out[meshIndex].vertices[i].position = m_Camera.viewMatrix.TransformPoint(mesh_out[meshIndex].vertices[i].position);

			// View Space To Projection Space
			// Camera settings
			mesh_out[meshIndex].vertices[i].position.x = mesh_out[meshIndex].vertices[i].position.x / (aspectRatio * m_Camera.fov);
			mesh_out[meshIndex].vertices[i].position.y = mesh_out[meshIndex].vertices[i].position.y / m_Camera.fov;
			// Perspective Divide
			mesh_out[meshIndex].vertices[i].position.x = mesh_out[meshIndex].vertices[i].position.x / mesh_out[meshIndex].vertices[i].position.z;
			mesh_out[meshIndex].vertices[i].position.y = mesh_out[meshIndex].vertices[i].position.y / mesh_out[meshIndex].vertices[i].position.z;

			// Projection Space To NDC/Screen Space
			mesh_out[meshIndex].vertices[i].position.x = (mesh_out[meshIndex].vertices[i].position.x + 1) / 2.f * m_Width;
			mesh_out[meshIndex].vertices[i].position.y = (1 - mesh_out[meshIndex].vertices[i].position.y) / 2.f * m_Height;
		}
	}

	
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void Renderer::Render_W1_Part1()
{
	std::vector<Vector3> vertices_ndc{
		{0.f, .5f, 1.f},
		{.5f, -.5f, 1.f},
		{-.5f, -.5f, 1.f}
	};

	for (uint8_t i = 0; i < vertices_ndc.size(); i++)
	{
		// Vertex to NDC space
		vertices_ndc[i].x = (vertices_ndc[i].x + 1 )/ 2.f * m_Width;
		vertices_ndc[i].y = (1 - vertices_ndc[i].y) / 2.f * m_Height;
	}

	for (int32_t px{}; px < m_Width; ++px)
	{
		for (int32_t py{}; py < m_Height; ++py)
		{
			Vector2 p{ static_cast<float>(px), static_cast<float>(py) };

			Vector2 v0{ vertices_ndc[0].x, vertices_ndc[0].y };
			Vector2 v1{ vertices_ndc[1].x, vertices_ndc[1].y };
			Vector2 v2{ vertices_ndc[2].x, vertices_ndc[2].y };

			//Does pixel and triangle overlap?

			ColorRGB finalColor{};
			if (Utils::TriangleHit(v0, v1, v2, p))
			{
				finalColor = ColorRGB(1, 1, 1);
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void Renderer::Render_W1_Part2()
{
	std::vector<Vertex> vertices_world
	{
		{Vector3(0.f, 2.f, 0.f)},
		{Vector3(1.f, 0.f, 0.f)},
		{Vector3(-1.f, 0.f, 0.f)}
	};

	m_Camera.CalculateViewMatrix();

	std::vector<Vertex> verteces_screen;
	VertexTransformationFunction(vertices_world, verteces_screen);

	for (int32_t px{}; px < m_Width; ++px)
	{
		for (int32_t py{}; py < m_Height; ++py)
		{
			Vector2 p{ static_cast<float>(px), static_cast<float>(py) };

			//Does pixel and triangle overlap?
			ColorRGB finalColor{};
			if (Utils::TriangleHit(verteces_screen[0], verteces_screen[1], verteces_screen[2], p))
			{
				finalColor = ColorRGB(1, 1, 1);
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void Renderer::Render_W1_Part3()
{
	std::vector<Vertex> vertices_world
	{
		{{0.f, 4.f, 2.f}, {1, 0, 0}},
		{{3.f, -2.f, 2.f}, {0, 1, 0}},
		{{-3.f, -2.f, 2.f}, {0, 0, 1}}
	};

	std::vector<Vertex> verteces_view;
	VertexTransformationFunction(vertices_world, verteces_view);

	for (int32_t px{}; px < m_Width; ++px)
	{
		for (int32_t py{}; py < m_Height; ++py)
		{
			Vector2 p{ static_cast<float>(px), static_cast<float>(py) };

			//Does pixel and triangle overlap?
			ColorRGB finalColor{};
			if (Utils::TriangleHit(verteces_view[0], verteces_view[1], verteces_view[2], p))
			{

				const Vector2 v0{ verteces_view[0].position.GetXY() };
				const Vector2 v1{ verteces_view[1].position.GetXY() };
				const Vector2 v2{ verteces_view[2].position.GetXY() };

				float w0{ Vector2::Cross(v2 - v1, p - v1) / 2.f};
				float w1{ Vector2::Cross(v0 - v2, p - v2) / 2.f };
				float w2{ Vector2::Cross(v1 - v0, p - v0) / 2.f };

				float total{ w0 + w1 + w2};
				w0 /= total;
				w1 /= total;
				w2 /= total;

				finalColor = verteces_view[0].color * w0 + verteces_view[1].color * w1 + verteces_view[2].color * w2;
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void Renderer::Render_W1_Part4()
{
	SDL_FillRect(m_pBackBuffer, NULL, 0);
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	const std::vector<Vertex> vertices_world
	{
		{{0.f, 2.f, 0.f}, {1, 0, 0}},
		{{1.5f, -1.f, 0.f}, {1, 0, 0}},
		{{-1.5f, -1.f, 0.f}, {1, 0, 0}},

		{{0.f, 4.f, 2.f}, {1, 0, 0}},
		{{3.f, -2.f, 2.f}, {0, 1, 0}},
		{{-3.f, -2.f, 2.f}, {0, 0, 1}}
	};

	std::vector<Vertex> verteces_view;
	VertexTransformationFunction(vertices_world, verteces_view);

	for (size_t i = 0; i < vertices_world.size() / 3; i++)
	{
		const Vertex vertex0{ verteces_view[i * 3] };
		const Vertex vertex1{ verteces_view[i * 3 + 1] };
		const Vertex vertex2{ verteces_view[i * 3 + 2] };

		for (int32_t px{}; px < m_Width; ++px)
		{
			for (int32_t py{}; py < m_Height; ++py)
			{
				Vector2 p{ static_cast<float>(px), static_cast<float>(py) };

				//Does pixel and triangle overlap?
				ColorRGB finalColor{};
				if (Utils::TriangleHit(vertex0, vertex1, vertex2, p))
				{
					const Vector2 v0{ vertex0.position.GetXY() };
					const Vector2 v1{ vertex1.position.GetXY() };
					const Vector2 v2{ vertex2.position.GetXY() };

					float w0{ Vector2::Cross(v2 - v1, p - v1) / 2.f };
					float w1{ Vector2::Cross(v0 - v2, p - v2) / 2.f };
					float w2{ Vector2::Cross(v1 - v0, p - v0) / 2.f };

					float total{ w0 + w1 + w2 };
					w0 /= total;
					w1 /= total;
					w2 /= total;

					const float currentDepth = vertex0.position.z * w0 + vertex1.position.z * w1 + vertex2.position.z * w2;

					if (m_pDepthBufferPixels[px + (py * m_Width)] >= currentDepth)
					{
						m_pDepthBufferPixels[px + (py * m_Width)] = currentDepth;

						finalColor = vertex0.color * w0 + vertex1.color * w1 + vertex2.color * w2;
						//Update Color in Buffer
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
}

void Renderer::Render_W1_Part5()
{
	SDL_FillRect(m_pBackBuffer, NULL, 0x111111);
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	const std::vector<Vertex> vertices_world
	{
		{{0.f, 2.f, 0.f}, {1, 0, 0}},
		{{1.5f, -1.f, 0.f}, {1, 0, 0}},
		{{-1.5f, -1.f, 0.f}, {1, 0, 0}},

		{{0.f, 4.f, 2.f}, {1, 0, 0}},
		{{3.f, -2.f, 2.f}, {0, 1, 0}},
		{{-3.f, -2.f, 2.f}, {0, 0, 1}}
	};

	std::vector<Vertex> verteces_view;
	VertexTransformationFunction(vertices_world, verteces_view);

	for (size_t i = 0; i < vertices_world.size() / 3; i++)
	{
		const Vertex vertex0{ verteces_view[i * 3] };
		const Vertex vertex1{ verteces_view[i * 3 + 1] };
		const Vertex vertex2{ verteces_view[i * 3 + 2] };

		/*Vector2 bbTopLeft{};
		bbTopLeft.x = std::min(vertex0.position.x, std::min(vertex1.position.x, vertex2.position.x));
		bbTopLeft.y = std::max(vertex0.position.y, std::max(vertex1.position.y, vertex2.position.y));

		Vector2 bbBottomRight{};
		bbBottomRight.x = std::max(vertex0.position.x, std::max(vertex1.position.x, vertex2.position.x));
		bbBottomRight.y = std::min(vertex0.position.y, std::min(vertex1.position.y, vertex2.position.y));

		for (int32_t px{}; px < m_Width; ++px)
		{
			for (int32_t py{}; py < m_Height; ++py)
			{
				if (!(px > bbTopLeft.x && px < bbBottomRight.x && py > bbBottomRight.y && py < bbTopLeft.y)) continue;*/

		uint32_t bbMaxx{}, bbMaxy{};
		bbMaxx = static_cast<uint32_t>(std::max(vertex0.position.x, std::max(vertex1.position.x, vertex2.position.x)));
		bbMaxy = static_cast<uint32_t>(std::max(vertex0.position.y, std::max(vertex1.position.y, vertex2.position.y)));

		uint32_t bbMinx{}, bbMiny{};
		bbMinx = static_cast<uint32_t>(std::min(vertex0.position.x, std::min(vertex1.position.x, vertex2.position.x)));
		bbMiny = static_cast<uint32_t>(std::min(vertex0.position.y, std::min(vertex1.position.y, vertex2.position.y)));

		bbMaxx = Clamp(bbMaxx, 0, m_Width);
		bbMaxy = Clamp(bbMaxy, 0, m_Height);

		bbMinx = Clamp(bbMinx, 0, m_Width);
		bbMiny = Clamp(bbMiny, 0, m_Height);

		for (uint32_t px{ bbMinx }; px < bbMaxx; ++px)
		{
			for (uint32_t py{ bbMiny }; py < bbMaxy; ++py)
			{
				Vector2 p{ static_cast<float>(px), static_cast<float>(py) };

				//Does pixel and triangle overlap?
				ColorRGB finalColor{};
				if (Utils::TriangleHit(vertex0, vertex1, vertex2, p))
				{
					const Vector2 v0{ vertex0.position.GetXY() };
					const Vector2 v1{ vertex1.position.GetXY() };
					const Vector2 v2{ vertex2.position.GetXY() };

					float w0{ Vector2::Cross(v2 - v1, p - v1) / 2.f };
					float w1{ Vector2::Cross(v0 - v2, p - v2) / 2.f };
					float w2{ Vector2::Cross(v1 - v0, p - v0) / 2.f };

					float total{ w0 + w1 + w2 };
					w0 /= total;
					w1 /= total;
					w2 /= total;

					const float currentDepth = vertex0.position.z * w0 + vertex1.position.z * w1 + vertex2.position.z * w2;

					if (m_pDepthBufferPixels[px + (py * m_Width)] >= currentDepth)
					{
						m_pDepthBufferPixels[px + (py * m_Width)] = currentDepth;

						finalColor = vertex0.color * w0 + vertex1.color * w1 + vertex2.color * w2;
						//Update Color in Buffer
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
}

void Renderer::Render_W2_Part1()
{

	ColorRGB clearColor = ColorRGB{ 100,100,100 };
	Uint32 clearColorUint = 0xFF000000 | (Uint32)clearColor.r | (Uint32)clearColor.b << 16 | (Uint32)clearColor.g << 8;
	SDL_FillRect(m_pBackBuffer, NULL, clearColorUint);
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	std::vector<Mesh> meshes_world
	{
		Mesh
		{
			{
				Vertex{{-3, 3, -2}},
				Vertex{{0, 3, -2}},
				Vertex{{3, 3, -2}},
				Vertex{{-3, 0, -2}},
				Vertex{{0, 0, -2}},
				Vertex{{3, 0, -2}},
				Vertex{{-3, -3, -2}},
				Vertex{{0, -3, -2}},
				Vertex{{3, -3, -2}}
			},
			{
				3, 0, 1,	1, 4, 3,	4, 1, 2,
				2, 5, 4,	6, 3, 4,	4, 7, 6,
				7, 4, 5,	5, 8, 7
			},
			PrimitiveTopology::TriangleList
		}
	};

	std::vector<Mesh> meshes_view{};
	VertexTransformationFunction(meshes_world, meshes_view);

	
	for (size_t meshIndex = 0; meshIndex < meshes_view.size(); meshIndex++)
	{
		Mesh mesh = meshes_view[meshIndex];
		for (size_t i = 0; i < mesh.indices.size() / 3; i++)
		{

			const Vertex vertex0{ mesh.vertices[mesh.indices[i * 3]] };
			const Vertex vertex1{ mesh.vertices[mesh.indices[i * 3 + 1]] };
			const Vertex vertex2{ mesh.vertices[mesh.indices[i * 3 + 2]] };

			
			//if (mesh.indices[i] == mesh.indices[i + 1] || mesh.indices[i + 1] == mesh.indices[i + 2])
			//{
			//	continue; // new strip, skip;
			//}

			
			//if (i % 2 == 0)
			//{
			//	//Clockwise
			//	vertex0 = mesh.vertices[mesh.indices[i]];
			//	vertex1 = mesh.vertices[mesh.indices[i + 1]];
			//	vertex2 = mesh.vertices[mesh.indices[i + 2]];
			//}
			//else
			//{
			//	//Counter Clockwise
			//	vertex0 = mesh.vertices[mesh.indices[i]];
			//	vertex1 = mesh.vertices[mesh.indices[i + 2]];
			//	vertex2 = mesh.vertices[mesh.indices[i + 1]];
			//}
			
			
			
			uint32_t bbMaxx{}, bbMaxy{};
			bbMaxx = static_cast<uint32_t>(std::max(vertex0.position.x, std::max(vertex1.position.x, vertex2.position.x)));
			bbMaxy = static_cast<uint32_t>(std::max(vertex0.position.y, std::max(vertex1.position.y, vertex2.position.y)));

			uint32_t bbMinx{}, bbMiny{};
			bbMinx = static_cast<uint32_t>(std::min(vertex0.position.x, std::min(vertex1.position.x, vertex2.position.x)));
			bbMiny = static_cast<uint32_t>(std::min(vertex0.position.y, std::min(vertex1.position.y, vertex2.position.y)));

			bbMaxx = Clamp(bbMaxx, 0, m_Width);
			bbMaxy = Clamp(bbMaxy, 0, m_Height);

			bbMinx = Clamp(bbMinx, 0, m_Width);
			bbMiny = Clamp(bbMiny, 0, m_Height);

			for (uint32_t px{ bbMinx }; px < bbMaxx; ++px)
			{
				for (uint32_t py{ bbMiny }; py < bbMaxy; ++py)
				{
					Vector2 p{ static_cast<float>(px), static_cast<float>(py) };

					//Does pixel and triangle overlap?
					ColorRGB finalColor{};
					if (Utils::TriangleHit(vertex0, vertex1, vertex2, p))
					{
						const Vector2 v0{ vertex0.position.GetXY() };
						const Vector2 v1{ vertex1.position.GetXY() };
						const Vector2 v2{ vertex2.position.GetXY() };

						float w0{ Vector2::Cross(v2 - v1, p - v1) / 2.f };
						float w1{ Vector2::Cross(v0 - v2, p - v2) / 2.f };
						float w2{ Vector2::Cross(v1 - v0, p - v0) / 2.f };

						float total{ w0 + w1 + w2 };
						w0 /= total;
						w1 /= total;
						w2 /= total;

						const float currentDepth = vertex0.position.z * w0 + vertex1.position.z * w1 + vertex2.position.z * w2;

						if (m_pDepthBufferPixels[px + (py * m_Width)] >= currentDepth)
						{
							m_pDepthBufferPixels[px + (py * m_Width)] = currentDepth;

							finalColor = vertex0.color * w0 + vertex1.color * w1 + vertex2.color * w2;
							//Update Color in Buffer
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
	}
}

void Renderer::Render_W2_Part2()
{
	SDL_FillRect(m_pBackBuffer, NULL, 0x111111);
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	std::vector<Mesh> meshes_world
	{
		Mesh
		{
			{
				Vertex{{-3, 3, -2}},
				Vertex{{0, 3, -2}},
				Vertex{{3, 3, -2}},
				Vertex{{-3, 0, -2}},
				Vertex{{0, 0, -2}},
				Vertex{{3, 0, -2}},
				Vertex{{-3, -3, -2}},
				Vertex{{0, -3, -2}},
				Vertex{{3, -3, -2}}
			},
			{
				3,0,4,1,5,2,
				2,6,
				6,3,7,4,8,5
			},
			PrimitiveTopology::TriangleStrip
		}
	};

	std::vector<Mesh> meshes_view{};
	VertexTransformationFunction(meshes_world, meshes_view);


	for (size_t meshIndex = 0; meshIndex < meshes_view.size(); meshIndex++)
	{
		Mesh mesh = meshes_view[meshIndex];
		for (size_t i = 0; i < mesh.indices.size() - 2; i++)
		{
			if (mesh.indices[i] == mesh.indices[i + 1] || mesh.indices[i + 1] == mesh.indices[i + 2])
			{
				continue; // new strip, skip;
			}
			Vertex vertex0{};
			Vertex vertex1{};
			Vertex vertex2{};
			if (i % 2 == 0)
			{
				//Clockwise
				vertex0 = mesh.vertices[mesh.indices[i]];
				vertex1 = mesh.vertices[mesh.indices[i + 1]];
				vertex2 = mesh.vertices[mesh.indices[i + 2]];
			}
			else
			{
				//Counter Clockwise
				vertex0 = mesh.vertices[mesh.indices[i]];
				vertex1 = mesh.vertices[mesh.indices[i + 2]];
				vertex2 = mesh.vertices[mesh.indices[i + 1]];
			}



			uint32_t bbMaxx{}, bbMaxy{};
			bbMaxx = static_cast<uint32_t>(std::max(vertex0.position.x, std::max(vertex1.position.x, vertex2.position.x)));
			bbMaxy = static_cast<uint32_t>(std::max(vertex0.position.y, std::max(vertex1.position.y, vertex2.position.y)));

			uint32_t bbMinx{}, bbMiny{};
			bbMinx = static_cast<uint32_t>(std::min(vertex0.position.x, std::min(vertex1.position.x, vertex2.position.x)));
			bbMiny = static_cast<uint32_t>(std::min(vertex0.position.y, std::min(vertex1.position.y, vertex2.position.y)));

			bbMaxx = Clamp(bbMaxx, 0, m_Width);
			bbMaxy = Clamp(bbMaxy, 0, m_Height);

			bbMinx = Clamp(bbMinx, 0, m_Width);
			bbMiny = Clamp(bbMiny, 0, m_Height);

			for (uint32_t px{ bbMinx }; px < bbMaxx; ++px)
			{
				for (uint32_t py{ bbMiny }; py < bbMaxy; ++py)
				{
					Vector2 p{ static_cast<float>(px), static_cast<float>(py) };

					//Does pixel and triangle overlap?
					ColorRGB finalColor{};
					if (Utils::TriangleHit(vertex0, vertex1, vertex2, p))
					{
						const Vector2 v0{ vertex0.position.GetXY() };
						const Vector2 v1{ vertex1.position.GetXY() };
						const Vector2 v2{ vertex2.position.GetXY() };

						float w0{ Vector2::Cross(v2 - v1, p - v1) / 2.f };
						float w1{ Vector2::Cross(v0 - v2, p - v2) / 2.f };
						float w2{ Vector2::Cross(v1 - v0, p - v0) / 2.f };

						float total{ w0 + w1 + w2 };
						w0 /= total;
						w1 /= total;
						w2 /= total;

						const float currentDepth = vertex0.position.z * w0 + vertex1.position.z * w1 + vertex2.position.z * w2;

						if (m_pDepthBufferPixels[px + (py * m_Width)] >= currentDepth)
						{
							m_pDepthBufferPixels[px + (py * m_Width)] = currentDepth;

							finalColor = vertex0.color * w0 + vertex1.color * w1 + vertex2.color * w2;
							//Update Color in Buffer
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
	}
}

void Renderer::Render_W2_Part3()
{
	SDL_FillRect(m_pBackBuffer, NULL, 0x111111);
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	std::vector<Mesh> meshes_world
	{
		Mesh
		{
			{
				Vertex{{-3, 3, -2}, {1, 0, 0}, {0, 0}},
				Vertex{{0, 3, -2}, {1, 0, 0}, {.5f, 0}},
				Vertex{{3, 3, -2}, {1, 0, 0}, {1, 0}},
				Vertex{{-3, 0, -2}, {0, 1, 0}, {0, .5f}},
				Vertex{{0, 0, -2}, {0, 1, 0}, {.5f, .5f}},
				Vertex{{3, 0, -2}, {0, 1, 0}, {1, .5f}},
				Vertex{{-3, -3, -2}, {0, 0, 1}, {0, 1}},
				Vertex{{0, -3, -2}, {0, 0, 1}, {.5f, 1}},
				Vertex{{3, -3, -2}, {0, 0, 1}, {1, 1}}
			},
			{
				3,0,4,1,5,2,
				2,6,
				6,3,7,4,8,5
			},
			PrimitiveTopology::TriangleStrip
		}
	};

	std::vector<Mesh> meshes_view{};
	VertexTransformationFunction(meshes_world, meshes_view);


	for (size_t meshIndex = 0; meshIndex < meshes_view.size(); meshIndex++)
	{
		Mesh mesh = meshes_view[meshIndex];
		for (size_t i = 0; i < mesh.indices.size() - 2; i++)
		{
			Vertex vertex0{};
			Vertex vertex1{};
			Vertex vertex2{};

			switch (mesh.primitiveTopology)
			{
			case PrimitiveTopology::TriangleList:
				vertex0 = mesh.vertices[mesh.indices[i * 3]];
				vertex1 = mesh.vertices[mesh.indices[i * 3 + 1]];
				vertex2 = mesh.vertices[mesh.indices[i * 3 + 2]];
				break;
			case PrimitiveTopology::TriangleStrip:
				if (mesh.indices[i] == mesh.indices[i + 1] || mesh.indices[i + 1] == mesh.indices[i + 2])
				{
					continue; // new strip, skip;
				}

				if (i % 2 == 0)
				{
					//Clockwise
					vertex0 = mesh.vertices[mesh.indices[i]];
					vertex1 = mesh.vertices[mesh.indices[i + 1]];
					vertex2 = mesh.vertices[mesh.indices[i + 2]];
				}
				else
				{
					//Counter Clockwise
					vertex0 = mesh.vertices[mesh.indices[i]];
					vertex1 = mesh.vertices[mesh.indices[i + 2]];
					vertex2 = mesh.vertices[mesh.indices[i + 1]];
				}
				break;
			}

			int bbMaxX{}, bbMaxY{};
			bbMaxX = static_cast<int>(std::max(vertex0.position.x, std::max(vertex1.position.x, vertex2.position.x)));
			bbMaxY = static_cast<int>(std::max(vertex0.position.y, std::max(vertex1.position.y, vertex2.position.y)));

			int bbMinX{}, bbMinY{};
			bbMinX = static_cast<int>(std::min(vertex0.position.x, std::min(vertex1.position.x, vertex2.position.x)));
			bbMinY = static_cast<int>(std::min(vertex0.position.y, std::min(vertex1.position.y, vertex2.position.y)));

			bbMaxX = Clamp(bbMaxX + 1, 0, m_Width) ;
			bbMaxY = Clamp(bbMaxY + 1, 0, m_Height);

			bbMinX = Clamp(bbMinX - 1, 0, m_Width);
			bbMinY = Clamp(bbMinY - 1, 0, m_Height);

			for (int px{ bbMinX }; px < bbMaxX; ++px)
			{
				for (int py{ bbMinY }; py < bbMaxY; ++py)
				{
					Vector2 p{ static_cast<float>(px), static_cast<float>(py) };

					//Does pixel and triangle overlap?
					ColorRGB finalColor{};
					if (Utils::TriangleHit(vertex0, vertex1, vertex2, p))
					{
						const Vector2 v0{ vertex0.position.GetXY() };
						const Vector2 v1{ vertex1.position.GetXY() };
						const Vector2 v2{ vertex2.position.GetXY() };

						float w0{ Vector2::Cross(v2 - v1, p - v1)};
						float w1{ Vector2::Cross(v0 - v2, p - v2)};
						float w2{ Vector2::Cross(v1 - v0, p - v0)};

						float total{ w0 + w1 + w2 };
						w0 /= total;
						w1 /= total;
						w2 /= total;

						//const float currentDepth = vertex0.position.z * w0 + vertex1.position.z * w1 + vertex2.position.z * w2;
						const float currentDepth = 1 / (1 / vertex0.position.z * w0 + 1 / vertex1.position.z * w1 + 1 / vertex2.position.z * w2);

						if (m_pDepthBufferPixels[px + (py * m_Width)] >= currentDepth)
						{
							m_pDepthBufferPixels[px + (py * m_Width)] = currentDepth;

							//finalColor = vertex0.color * w0 + vertex1.color * w1 + vertex2.color * w2;
							//Vector2 uv{ vertex0.uv * w0 + vertex1.uv * w1 + vertex2.uv * w2 };
							Vector2 uv = (vertex0.uv / vertex0.position.z * w0 + vertex1.uv / vertex1.position.z * w1 + vertex2.uv / vertex2.position.z * w2) * currentDepth;
							uv = Vector2{ Clamp(uv.y, 0.f, 1.f), Clamp(uv.x, 0.f, 1.f) }; // I am not sure why it's turned otherwise
							finalColor = m_pTexture->Sample(uv);
							

							//Update Color in Buffer
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
	}
}