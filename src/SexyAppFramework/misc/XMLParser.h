#ifndef __XMLPARSER_H__
#define __XMLPARSER_H__

#include "Common.h"

#include <list>

#include "PerfTimer.h"

struct PFILE;

namespace Sexy
{

class XMLParam
{
public:
	std::string				mKey;
	std::string				mValue;
};

typedef std::map<std::string, std::string>	XMLParamMap;
typedef std::list<XMLParamMap::iterator>	XMLParamMapIteratorList;

typedef std::vector<char> XMLParserBuffer;

class XMLElement
{
public:
	enum
	{
		TYPE_NONE,
		TYPE_START,
		TYPE_END,
		TYPE_ELEMENT,
		TYPE_INSTRUCTION,
		TYPE_COMMENT
	};
public:
	
	int						mType;
	std::string				mSection;
	std::string				mValue;
	std::string				mInstruction;
	XMLParamMap				mAttributes;
	XMLParamMapIteratorList	mAttributeIteratorList; // stores attribute iterators in their original order
};

class XMLParser
{
protected:
	std::string				mFileName;
	std::string				mErrorText;
	int						mLineNum;
	PFILE*					mFile;
	bool					mHasFailed;
	bool					mAllowComments;
	XMLParserBuffer			mBufferedText;
	std::string				mSection;
	bool					(XMLParser::*mGetCharFunc)(char* theChar, bool* error);
	bool					mForcedEncodingType;
	bool					mFirstChar;
	bool					mByteSwap;

protected:
	void					Fail(const std::string& theErrorText);
	void					Init();

	bool					AddAttribute(XMLElement* theElement, const std::string& aAttributeKey, const std::string& aAttributeValue);

	bool					GetAsciiChar(char* theChar, bool* error);
	bool					GetUTF8Char(char* theChar, bool* error);
	bool					GetUTF16Char(char* theChar, bool* error);
	bool					GetUTF16LEChar(char* theChar, bool* error);
	bool					GetUTF16BEChar(char* theChar, bool* error);

public:
	enum XMLEncodingType
	{
		ASCII,
		UTF_8,
		UTF_16,
		UTF_16_LE,
		UTF_16_BE
	};

public:
	XMLParser();
	virtual ~XMLParser();

	void					SetEncodingType(XMLEncodingType theEncoding);
	bool					OpenFile(const std::string& theFilename);
	void					SetStringSource(const std::string& theString);
	bool					NextElement(XMLElement* theElement);
	std::string				GetErrorText();
	int						GetCurrentLineNum();
	std::string				GetFileName();

	inline void				AllowComments(bool doAllow) { mAllowComments = doAllow; }

	bool					HasFailed();
	bool					EndOfFile();
};

};

#endif //__XMLPARSER_H__
