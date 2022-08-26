#pragma once

#include "pch.h"
#include "framework.h"

#include <Windows.h>
#include <psapi.h>

#include <iostream>
#include <string>
#include <vector>

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>

namespace yeux {

	struct Region {
		std::string name;
		int x;
		int y;
		int x2;
		int y2;
		float scale;
		std::string whiteList;
	};

	struct RGB {
		short r;
		short g;
		short b;
	};

	class Yeux {
	
		std::string const VERSION = "1.0.0";

	public:
		Yeux();
		~Yeux();
		std::string const& version() const noexcept;
		bool setup(std::string const& lang);
		void screenshot(int x, int y, int width, int height, std::string const& path);
		void clean();
		std::string const getText(Region const& region, bool const debug) const;
		std::string const getColor(Region const& region, bool const debug) const;
		RGB const getRGB(Region const& region, bool const debug) const;

	private:
		Pix* hbitmapToPixs(DWORD dwBmpSize, BITMAPFILEHEADER bmfHeader, BITMAPINFOHEADER bi, char* lpbitmap);
		wchar_t* stringToWideChar(std::string const& string);
		PBITMAPINFO createBitmapInfoStruct(HBITMAP hBmp);
		void createBMPFile(LPTSTR pszFile, HBITMAP hBMP);
		Pix* cropPix(Pix* pixs, int const x, int const y, int const x2, int const y2) const;
		Pix* processPix(Pix* pixd, float scaleFactor, bool s) const;
		std::pair<std::string, int> const tesseractCompute(Pix* pixs, std::string const& whiteList, bool debug) const;

		Pix* m_pixels;
		tesseract::TessBaseAPI* m_tapi;
	};

}
