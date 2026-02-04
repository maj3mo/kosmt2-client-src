#include "StdAfx.h"
#include "TextBar.h"
#include "FontManager.h"
#include "Util.h"

#include <utf8.h>

#include <ft2build.h>
#include FT_BITMAP_H

#include <cmath>

// Same gamma LUT as GrpFontTexture for consistent text sharpness
static struct STextBarGammaLUT {
	unsigned char table[256];
	STextBarGammaLUT() {
		table[0] = 0;
		for (int i = 1; i < 256; ++i)
			table[i] = (unsigned char)(pow(i / 255.0, 0.80) * 255.0 + 0.5);
	}
} s_textBarGammaLUT;

void CTextBar::__SetFont(int fontSize, bool isBold)
{
	// Create a per-instance FT_Face (this instance owns it)
	m_ftFace = CFontManager::Instance().CreateFace("Tahoma");
	if (!m_ftFace)
		return;

	int pixelSize = (m_fontSize < 0) ? -m_fontSize : m_fontSize;
	if (pixelSize == 0)
		pixelSize = 12;

	FT_Set_Pixel_Sizes(m_ftFace, 0, pixelSize);
	FT_Set_Transform(m_ftFace, NULL, NULL);

	m_ascender = (int)(m_ftFace->size->metrics.ascender >> 6);
	m_lineHeight = (int)(m_ftFace->size->metrics.height >> 6);
}

void CTextBar::SetTextColor(int r, int g, int b)
{
	m_textColor = ((DWORD)r) | ((DWORD)g << 8) | ((DWORD)b << 16);
}

void CTextBar::GetTextExtent(const char* c_szText, SIZE* p_size)
{
	if (!c_szText || !p_size)
	{
		if (p_size)
		{
			p_size->cx = 0;
			p_size->cy = 0;
		}
		return;
	}

	if (!m_ftFace)
	{
		p_size->cx = 0;
		p_size->cy = 0;
		return;
	}

	std::wstring wText = Utf8ToWide(c_szText);

	int totalAdvance = 0;
	for (size_t i = 0; i < wText.size(); ++i)
	{
		FT_UInt glyphIndex = FT_Get_Char_Index(m_ftFace, wText[i]);
		if (glyphIndex == 0)
			glyphIndex = FT_Get_Char_Index(m_ftFace, L' ');

		if (FT_Load_Glyph(m_ftFace, glyphIndex, FT_LOAD_DEFAULT) == 0)
			totalAdvance += (int)ceilf((float)(m_ftFace->glyph->advance.x) / 64.0f);
	}

	p_size->cx = totalAdvance;
	p_size->cy = m_lineHeight;
}

void CTextBar::TextOut(int ix, int iy, const char * c_szText)
{
	if (!c_szText || !*c_szText || !m_ftFace)
		return;

	DWORD* pdwBuf = (DWORD*)m_dib.GetPointer();
	if (!pdwBuf)
		return;

	int bufWidth = m_dib.GetWidth();
	int bufHeight = m_dib.GetHeight();

	std::wstring wText = Utf8ToWide(c_szText);

	int penX = ix;
	int penY = iy;

	DWORD colorRGB = m_textColor;  // 0x00BBGGRR in memory

	for (size_t i = 0; i < wText.size(); ++i)
	{
		FT_UInt glyphIndex = FT_Get_Char_Index(m_ftFace, wText[i]);
		if (glyphIndex == 0)
			glyphIndex = FT_Get_Char_Index(m_ftFace, L' ');

		FT_Int32 loadFlags = FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL;
		if (FT_Load_Glyph(m_ftFace, glyphIndex, loadFlags) != 0)
			continue;

		FT_GlyphSlot slot = m_ftFace->glyph;

		// Apply synthetic bold (32 = 0.5px embolden; 64 = 1px was too aggressive)
		if (m_isBold && slot->bitmap.buffer)
			FT_Bitmap_Embolden(CFontManager::Instance().GetLibrary(), &slot->bitmap, 32, 0);

		FT_Bitmap& bitmap = slot->bitmap;

		int bmpX = penX + slot->bitmap_left;
		int bmpY = penY + m_ascender - slot->bitmap_top;

		for (int row = 0; row < (int)bitmap.rows; ++row)
		{
			int destY = bmpY + row;
			if (destY < 0 || destY >= bufHeight)
				continue;

			unsigned char* srcRow = bitmap.buffer + row * bitmap.pitch;
			DWORD* dstRow = pdwBuf + destY * bufWidth;

			for (int col = 0; col < (int)bitmap.width; ++col)
			{
				int destX = bmpX + col;
				if (destX < 0 || destX >= bufWidth)
					continue;

				unsigned char alpha = srcRow[col];
				if (alpha)
				{
					alpha = s_textBarGammaLUT.table[alpha];
					DWORD r = (colorRGB >> 0) & 0xFF;
					DWORD g = (colorRGB >> 8) & 0xFF;
					DWORD b = (colorRGB >> 16) & 0xFF;
					dstRow[destX] = ((DWORD)alpha << 24) | (r << 16) | (g << 8) | b;
				}
			}
		}

		penX += (int)ceilf((float)(slot->advance.x) / 64.0f);
	}

	Invalidate();
}

void CTextBar::OnCreate()
{
	__SetFont(m_fontSize, m_isBold);
}

CTextBar::CTextBar(int fontSize, bool isBold)
{
	m_ftFace = nullptr;
	m_textColor = 0x00FFFFFF;  // White (RGB)
	m_fontSize = fontSize;
	m_isBold = isBold;
	m_ascender = 0;
	m_lineHeight = 0;
}

CTextBar::~CTextBar()
{
	if (m_ftFace)
	{
		FT_Done_Face(m_ftFace);
		m_ftFace = nullptr;
	}
}
