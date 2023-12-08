#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

#include <iostream>

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
		//TODO
		//Load SDL_Surface using IMG_LOAD
		//Create & Return a new Texture Object (using SDL_Surface)

		SDL_Surface* surfacePtr{ IMG_Load(path.c_str()) };

		if (!surfacePtr)
		{
			std::cout << "Failed to load texture " << path << "! Error:\n" << IMG_GetError() << std::endl;
			abort();
		}

		return new Texture{ surfacePtr };
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//TODO
		//Sample the correct texel for the given uv
		const uint32_t x{ static_cast<uint32_t>(m_pSurface->w * abs(uv.x)) % m_pSurface->w };
		const uint32_t y{ static_cast<uint32_t>(m_pSurface->h * abs(uv.y)) % m_pSurface->h };

		uint8_t r, g, b;

		const size_t idx{ y * m_pSurface->w + x };

		SDL_GetRGB(m_pSurfacePixels[idx], m_pSurface->format, &r, &g, &b);

		return {
			r / 255.f ,
			g / 255.f,
			b / 255.f
		};
	}
}