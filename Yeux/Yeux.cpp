#include "pch.h"
#include "Yeux.hpp"

namespace yeux {

	Yeux::Yeux()
		: m_pixels(nullptr)
	{
		m_tapi = new tesseract::TessBaseAPI();
	}

	Yeux::~Yeux()
	{
		if (m_pixels) {
			pixDestroy(&m_pixels);
		}
		if (m_tapi) {
			m_tapi->End();
			delete m_tapi;
		}
	}

	std::string const& Yeux::version() const noexcept
	{
		return VERSION;
	}

	bool Yeux::setup(std::string const& lang)
	{
		if (m_tapi->Init(nullptr, lang.c_str())) {
			std::cerr << "Could not initialize language " << lang << std::endl;
			return false;
		}
		return true;
	}

	void Yeux::screenshot(int x, int y, int width, int height, std::string const& path)
	{
		HWND				hwnd = ::GetDesktopWindow();
		HDC					displayDc = ::GetDC(nullptr);
		HDC					bitmapDc = ::CreateCompatibleDC(displayDc);
		HBITMAP				bitmap = ::CreateCompatibleBitmap(displayDc, width, height);
		HGDIOBJ				nullBitmap = ::SelectObject(bitmapDc, bitmap);
		HDC					windowDc = ::GetDC(hwnd);
		BITMAPFILEHEADER	bmfHeader{};
		BITMAPINFOHEADER	bi{};
		BITMAP				bmp{};

		::BitBlt(bitmapDc, 0, 0, width, height, windowDc, x, y, SRCCOPY | CAPTUREBLT);
		::GetObject(bitmap, sizeof(BITMAP), &bmp);

		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = bmp.bmWidth;
		bi.biHeight = bmp.bmHeight;
		bi.biPlanes = bmp.bmPlanes;
		bi.biBitCount = bmp.bmBitsPixel;
		bi.biCompression = BI_RGB;
		bi.biSizeImage = 0;
		bi.biXPelsPerMeter = 0;
		bi.biYPelsPerMeter = 0;
		bi.biClrUsed = 0;
		bi.biClrImportant = 0;

		DWORD	dwBmpSize = ((bmp.bmWidth * bmp.bmBitsPixel + 31) / 32) * 4 * bmp.bmHeight;
		HANDLE	hDIB = ::GlobalAlloc(GHND, dwBmpSize);

		if (hDIB) {
			char* lpbitmap = reinterpret_cast<char*>(::GlobalLock(hDIB));

			::GetDIBits(bitmapDc, bitmap, 0, (UINT)bmp.bmHeight, lpbitmap, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
			DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

			bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
			bmfHeader.bfSize = dwSizeofDIB;
			bmfHeader.bfType = 0x4D42;
			m_pixels = hbitmapToPixs(dwBmpSize, bmfHeader, bi, lpbitmap);
			if (path != "") {
				std::unique_ptr<wchar_t> execpath(stringToWideChar(path + ".bmp"));

				createBMPFile(execpath.get(), bitmap);
			}
			::GlobalUnlock(hDIB);
			::GlobalFree(hDIB);
		}
		::ReleaseDC(hwnd, windowDc);
		::SelectObject(bitmapDc, nullBitmap);
		::DeleteDC(bitmapDc);
		::DeleteObject(bitmap);
		::ReleaseDC(nullptr, displayDc);
	}

	void Yeux::clean()
	{
		pixDestroy(&m_pixels);
	}

	std::string const Yeux::getText(Region const& region, bool debug) const
	{
		Pix*		crop = cropPix(m_pixels, region.x, region.y, region.x2, region.y2);
		Pix*		process = processPix(crop, region.scale, false);
		auto const& ret = tesseractCompute(process, region.whiteList, debug);

		if (debug) {
			pixWriteImpliedFormat(std::string(region.name + ".png").c_str(), process, 0, 0);
			std::cerr << region.name << " predict: <" << ret.first << "> at " << std::to_string(ret.second) << "%" << " wl: " << region.whiteList << std::endl;
		}
		pixDestroy(&process);
		return ret.first;
	}

	std::string const Yeux::getColor(Region const& region, bool debug) const
	{
		std::string color = "";
		RGB			rgb = getRGB(region, false);

		if (rgb.r > 180 && rgb.g > 180 && rgb.b > 180) {
			color = "white";
		}
		else if (rgb.r < 66 && rgb.g < 66 && rgb.b < 66) {
			color = "black";
		}
		else {
			if (rgb.r > rgb.g && rgb.r > rgb.b) {
				color = "blue";
			}
			else if (rgb.g > rgb.r && rgb.g > rgb.b) {
				color = "green";
			}
			else if (rgb.b > rgb.r && rgb.b > rgb.g) {
				if (rgb.g > 80) {
					color = "yellow";
				}
				else {
					color = "red";
				}
			}
		}
		if (debug) {
			std::cerr << region.name << " " << rgb.r << " " << rgb.g << " " << rgb.b << ": " << color << std::endl;
		}
		return color;
	}

	RGB const Yeux::getRGB(Region const& region, bool debug) const
	{
		RGB			rgb = { 0, 0, 0 };
		l_uint32	val;

		pixGetPixel(m_pixels, static_cast<l_int32>(region.x), static_cast<l_int32>(region.y), &val);
		rgb.r = ((val >> 24) & 0xFF);
		rgb.g = ((val >> 16) & 0xFF);
		rgb.b = ((val >> 8) & 0xFF);
		if (debug) {
			std::cerr << region.name << " " << rgb.r << " " << rgb.g << " " << rgb.b << std::endl;
		}
		return rgb;
	}

	Pix* Yeux::hbitmapToPixs(DWORD dwBmpSize, BITMAPFILEHEADER bmfHeader, BITMAPINFOHEADER bi, char* lpbitmap)
	{
		std::vector<unsigned char> buffer(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize);

		std::copy(reinterpret_cast<unsigned char*>(&bmfHeader), reinterpret_cast<unsigned char*>(&bmfHeader) + sizeof(BITMAPFILEHEADER), buffer.begin());
		std::copy(reinterpret_cast<unsigned char*>(&bi), reinterpret_cast<unsigned char*>(&bi) + sizeof(BITMAPINFOHEADER), buffer.begin() + sizeof(BITMAPFILEHEADER));
		std::copy(lpbitmap, lpbitmap + dwBmpSize, buffer.begin() + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
		return pixReadMemBmp(&buffer[0], buffer.size());
	}

	wchar_t* Yeux::stringToWideChar(std::string const& string)
	{
		int			len = 0;
		int			slength = (int)string.length() + 1;
		wchar_t*	buf = nullptr;
		
		len = ::MultiByteToWideChar(CP_ACP, 0, string.c_str(), slength, 0, 0);
		buf = new wchar_t[len];
		if (buf) {
			::MultiByteToWideChar(CP_ACP, 0, string.c_str(), slength, buf, len);
		}
		return buf;
	}

	PBITMAPINFO Yeux::createBitmapInfoStruct(HBITMAP hBmp)
	{
		BITMAP		bmp{};
		PBITMAPINFO pbmi;
		WORD		cClrBits;

		::GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp);
		cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
		if (cClrBits == 1) {
			cClrBits = 1;
		} else if (cClrBits <= 4) {
			cClrBits = 4;
		} else if (cClrBits <= 8) {
			cClrBits = 8;
		} else if (cClrBits <= 16) {
			cClrBits = 16;
		} else if (cClrBits <= 24) {
			cClrBits = 24;
		} else {
			cClrBits = 32;
		}
		if (cClrBits < 24) {
			pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (static_cast<unsigned long long>(1) << cClrBits));
		} else {
			pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER));
		}
		if (pbmi) {
			pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			pbmi->bmiHeader.biWidth = bmp.bmWidth;
			pbmi->bmiHeader.biHeight = bmp.bmHeight;
			pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
			pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
			if (cClrBits < 24) {
				pbmi->bmiHeader.biClrUsed = (1 << cClrBits);
			}
			pbmi->bmiHeader.biCompression = BI_RGB;
			pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits + 31) & ~31) / 8 * pbmi->bmiHeader.biHeight;
			pbmi->bmiHeader.biClrImportant = 0;
		}
		return pbmi;
	}

	void Yeux::createBMPFile(LPTSTR pszFile, HBITMAP hBMP)
	{
		HANDLE				hf;
		BITMAPFILEHEADER	hdr{};
		PBITMAPINFOHEADER	pbih;
		LPBYTE				lpBits;
		DWORD				dwTotal;
		DWORD				cb;
		BYTE*				hp;
		DWORD				dwTmp = 0;
		PBITMAPINFO			pbi;
		HDC					hDC;

		hDC = ::CreateCompatibleDC(::GetWindowDC(::GetDesktopWindow()));
		::SelectObject(hDC, hBMP);
		pbi = createBitmapInfoStruct(hBMP);
		pbih = (PBITMAPINFOHEADER)pbi;
		lpBits = (LPBYTE)::GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);
		::GetDIBits(hDC, hBMP, 0, (WORD)pbih->biHeight, lpBits, pbi, DIB_RGB_COLORS);
		hf = ::CreateFile(pszFile, GENERIC_READ | GENERIC_WRITE, (DWORD)0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
		hdr.bfType = 0x4d42;
		hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof(RGBQUAD) + pbih->biSizeImage);
		hdr.bfReserved1 = 0;
		hdr.bfReserved2 = 0;
		hdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof(RGBQUAD);
		::WriteFile(hf, (LPVOID)&hdr, sizeof(BITMAPFILEHEADER), (LPDWORD)&dwTmp, NULL);
		::WriteFile(hf, (LPVOID)pbih, sizeof(BITMAPINFOHEADER) + pbih->biClrUsed * sizeof(RGBQUAD), (LPDWORD)&dwTmp, (NULL));
		dwTotal = cb = pbih->biSizeImage;
		hp = lpBits;
		::WriteFile(hf, (LPSTR)hp, (int)cb, (LPDWORD)&dwTmp, NULL);
		::CloseHandle(hf);
		::GlobalFree((HGLOBAL)lpBits);
	}

	Pix* Yeux::cropPix(Pix* pixs, int x, int y, int x2, int y2) const
	{
		if (x2 - x <= 0 || y2 - y <= 0) {
			return pixs;
		}
		Box* cropWindow = boxCreate(static_cast<l_int32>(x), static_cast<l_int32>(y), static_cast<l_int32>(x2 - x), static_cast<l_int32>(y2 - y));
		Pix* pix = pixClipRectangle(pixs, cropWindow, nullptr);

		boxDestroy(&cropWindow);
		return pix;
	}

	Pix* Yeux::processPix(Pix* pixd, float scaleFactor, bool s) const
	{
		l_float32	dark_bg_threshold = 0.5f;
		int			perform_scale = 1;
		int			perform_unsharp_mask = 1;
		l_int32		usm_halfwidth = 5;
		l_float32	usm_fract = 2.5f;
		int			perform_otsu_binarize = 1;
		l_int32		otsu_sx = 2000;
		l_int32		otsu_sy = 2000;
		l_int32		otsu_smoothx = 0;
		l_int32		otsu_smoothy = 0;
		l_float32	otsu_scorefract = 0.0f;
		l_float32	border_avg = 0.0f;

		Pix* pixs = pixConvertRGBToGray(pixd, 0.0f, 0.0f, 0.0f);
		
		pixDestroy(&pixd);
		Pix* otsu_pixs = nullptr;
		
		pixOtsuAdaptiveThreshold(pixs, otsu_sx, otsu_sy, otsu_smoothx, otsu_smoothy, otsu_scorefract, nullptr, &otsu_pixs);
		border_avg = pixAverageOnLine(otsu_pixs, 0, 0, otsu_pixs->w - 1, 0, 1);
		border_avg += pixAverageOnLine(otsu_pixs, 0, otsu_pixs->h - 1, otsu_pixs->w - 1, otsu_pixs->h - 1, 1);
		border_avg += pixAverageOnLine(otsu_pixs, 0, 0, 0, otsu_pixs->h - 1, 1);
		border_avg += pixAverageOnLine(otsu_pixs, otsu_pixs->w - 1, 0, otsu_pixs->w - 1, otsu_pixs->h - 1, 1);
		border_avg /= 4.0f;
		pixDestroy(&otsu_pixs);
		if (border_avg > dark_bg_threshold) {
			pixInvert(pixs, pixs);
		}
		Pix* pixscale = pixScaleGrayLI(pixs, scaleFactor, scaleFactor);
		
		pixDestroy(&pixs);
		if (s) {
			pixOtsuAdaptiveThreshold(pixscale, otsu_sx, otsu_sy, otsu_smoothx, otsu_smoothy, otsu_scorefract, nullptr, &otsu_pixs);
			pixDestroy(&pixscale);
			return otsu_pixs;
		}
		else {
			Pix* pixsunsharp = pixUnsharpMaskingGray(pixscale, usm_halfwidth, usm_fract);
			
			pixDestroy(&pixscale);
			pixOtsuAdaptiveThreshold(pixsunsharp, otsu_sx, otsu_sy, otsu_smoothx, otsu_smoothy, otsu_scorefract, nullptr, &otsu_pixs);
			pixDestroy(&pixsunsharp);
			return otsu_pixs;
		}
		return nullptr;
	}

	std::pair<std::string, int> const Yeux::tesseractCompute(Pix* pixs, std::string const& whiteList, bool debug) const
	{
		std::pair<std::string, int> ret("", 0);
		char*						text = nullptr;
		std::string					tmp("");

		m_tapi->SetImage(pixs);
		text = m_tapi->GetUTF8Text();
		tmp = std::string(text);
		if (debug) {
			std::cerr << "ocr debug: <" << tmp << ">" << std::endl;
		}
		ret.second = m_tapi->MeanTextConf();
		if (whiteList != "") {
			for (std::string::size_type i = 0; i < tmp.length(); i++) {
				std::size_t found = whiteList.find(tmp.at(i));

				if (found != std::string::npos) {
					ret.first += tmp.at(i);
				}
			}
		}
		else {
			ret.first = tmp;
		}
		if (ret.first.length() && ret.first.back() == '\n') {
			ret.first.pop_back();
		}
		delete[] text;
		return ret;
	}

}