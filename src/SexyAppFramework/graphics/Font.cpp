#include "Font.h"
#include "Image.h"

using namespace Sexy;

_Font::_Font()
{	
	mAscent = 0;
	mAscentPadding = 0;
	mHeight = 0;
	mLineSpacingOffset = 0;
}

_Font::_Font(const _Font& theFont) :
	mAscent(theFont.mAscent),
	mAscentPadding(theFont.mAscentPadding),
	mHeight(theFont.mHeight),
	mLineSpacingOffset(theFont.mLineSpacingOffset)
{
}

_Font::~_Font()
{
}

int	_Font::GetAscent()
{
	return mAscent;
}

int	_Font::GetAscentPadding()
{
	return mAscentPadding;
}

int	_Font::GetDescent()
{
	return mHeight - mAscent;
}

int	_Font::GetHeight()
{
	return mHeight;
}

int _Font::GetLineSpacingOffset()
{
	return mLineSpacingOffset;
}

int _Font::GetLineSpacing()
{
	return mHeight + mLineSpacingOffset;
}


int _Font::StringWidth(const std::string&)
{
	return 0;
}


int _Font::CharWidth(char theChar)
{
	std::string aString(1, theChar);
	return StringWidth(aString);
}

int _Font::CharWidthKern(char theChar, char)
{
	return CharWidth(theChar);
}

void _Font::DrawString(Graphics*, int, int, const std::string&, const Color&, const Rect&){}

