#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		//Load SDL_Surface using IMG_LOAD
		//Create & Return a new Texture Object (using SDL_Surface)
		return new Texture{ IMG_Load(path.c_str()) };
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//Sample the correct texel for the given uv
		
		Uint32 u = uv.x * m_pSurface->w;
		Uint32 v = uv.y * m_pSurface->h;
		
		Uint8 r, g, b;
		SDL_GetRGB(m_pSurfacePixels[Uint32(v * m_pSurface->w + u)], m_pSurface->format, &r, &g, &b);
		ColorRGB color{ r, g, b };
		return  color / 255.0f;
	}
}