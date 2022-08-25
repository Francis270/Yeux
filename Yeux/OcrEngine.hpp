#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <psapi.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>

struct Region {
	std::string name;
	int x;
	int y;
	int x2;
	int y2;
	float scale;
	std::string whiteList;
	int skipX;
};

struct RGB {
	short r;
	short g;
	short b;
};

class OcrEngine {

	std::string const DEF_FILE_TYPE = ".png";

public:
	OcrEngine();
	~OcrEngine();
	bool loadTesseract(std::string const& lang);
	void screenshot(int x, int y, int width, int height, std::string const& path);
	void clean();
	std::string const getText(Region const& region, bool const debug) const;
	RGB const getRGB(Region const& region, bool const debug) const;
	std::string const getColor(Region const& region, bool const debug) const;

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
