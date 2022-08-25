#include "pch.h"
#include "framework.h"
#include "Yeux.hpp"

namespace yeux {

	Yeux::Yeux()
		: m_version("1.0.0"), m_ocr()
	{
	}

	Yeux::~Yeux()
	{
	}

	std::string const& Yeux::version() const noexcept
	{
		return m_version;
	}

	bool Yeux::setup(std::string const& lang)
	{
		return m_ocr.loadTesseract(lang);
	}

	void Yeux::screenshot(int x, int y, int width, int height, std::string const& path)
	{
		m_ocr.screenshot(x, y, width, height, path);
	}

	void Yeux::clean()
	{
		m_ocr.clean();
	}

	std::string const Yeux::getText(std::string const& name, int x, int x2, int y, int y2, float scale, std::string const& whitelist, bool debug) const
	{
		Region region = { name, x, x2, y, y2, scale, whitelist };

		return m_ocr.getText(region, debug);
	}

	std::string const Yeux::getColor(std::string const& name, int x, int y, bool debug) const
	{
		Region region = { name, x, 0, y, 0, 0.0, "" };

		return m_ocr.getColor(region, debug);
	}

	RGB const Yeux::getRGB(std::string const& name, int x, int y, bool debug) const
	{
		Region region = { name, x, 0, y, 0, 0.0, "" };

		return m_ocr.getRGB(region, debug);
	}

}