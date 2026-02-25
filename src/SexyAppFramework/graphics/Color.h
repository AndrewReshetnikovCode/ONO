#ifndef __COLOR_H__
#define __COLOR_H__

#include "Common.h"

namespace Sexy
{

class Color
{
public:
	int mRed;
	int mGreen;
	int mBlue;
	int mAlpha;

	static Color Black;
	static Color White;

public:
	Color();
	Color(int theColor);
	Color(int theColor, int theAlpha);
	Color(int theRed, int theGreen, int theBlue);
	Color(int theRed, int theGreen, int theBlue, int theAlpha);
	Color(const uchar* theElements);	
	Color(const int* theElements);

	int						GetRed() const;
	int						GetGreen() const;
	int						GetBlue() const;
	int						GetAlpha() const;
	uint32_t				ToInt() const;
	uint32_t				ToGLColor() const;

	int&					operator[](int theIdx);
	int						operator[](int theIdx) const;	
};

bool operator==(const Color& theColor1, const Color& theColor2);
bool operator!=(const Color& theColor1, const Color& theColor2);

}

#endif //__COLOR_H__