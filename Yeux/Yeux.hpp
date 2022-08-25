#pragma once

#include <iostream>
#include "OcrEngine.hpp"

namespace yeux {

	class Yeux {

	public:
		Yeux();
		~Yeux();
		std::string const& version() const noexcept;
		bool setup(std::string const &lang);
		void screenshot(int x, int y, int width, int height, std::string const& path);
		void clean();
		std::string const getText(std::string const& name, int x, int x2, int y, int y2, float scale, std::string const& whitelist, bool debug) const;
		std::string const getColor(std::string const& name, int x, int y, bool debug) const;
		RGB const getRGB(std::string const& name, int x, int y, bool debug) const;

	private:
		std::string m_version;
		OcrEngine m_ocr;

	};

}
