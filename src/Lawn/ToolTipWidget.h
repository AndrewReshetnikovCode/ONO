#ifndef __TOOLTIPWIDGET_H__ 
#define __TOOLTIPWIDGET_H__

#include "../SexyAppFramework/Common.h"
namespace Sexy
{
    class Graphics;
}

class ToolTipWidget
{
public:
    std::string			mTitle;                 //+0x0
    std::string			mLabel;                 //+0x1C
    std::string			mWarningText;           //+0x28
    int                 mX;                     //+0x54
    int                 mY;                     //+0x58
    int                 mWidth;                 //+0x5C
    int                 mHeight;                //+0x60
    bool                mVisible;               //+0x64
    bool                mCenter;                //+0x65
    int                 mMinLeft;               //+0x68
    int                 mMaxBottom;             //+0x6C
    int                 mGetsLinesWidth;        //+0x70
    int                 mWarningFlashCounter;   //+0x74

public:
    ToolTipWidget();

    void                Draw(Sexy::Graphics* g);
    void                SetLabel(const std::string& theLabel);
    void                SetTitle(const std::string& theTitle);
    void                SetWarningText(const std::string& theWarningText);
    void                CalculateSize();
    void                GetLines(std::vector<std::string>& theLines);
    inline void         FlashWarning() { mWarningFlashCounter = 70; }
    inline void         Update() { if (mWarningFlashCounter > 0) mWarningFlashCounter--; }
    inline void         SetPosition(int theX, int theY) { mX = theX; mY = theY; }
};

#endif