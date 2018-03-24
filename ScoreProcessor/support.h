#ifndef SUPPORT_H
#define SUPPORT_H
#include "lib\exstring\exstring.h"
enum class support_type {
	no,png,jpeg,bmp
};
inline support_type supported(wchar_t const* ext)
{
	if(*ext==0)
	{
		return support_type::no;
	}
	if(!exlib::strncmp_nocase(ext,L"png"))
	{
		return support_type::png;
	}
	if(*ext==L'j')
	{
		auto off=ext+1;
		if(!(exlib::strncmp_nocase(off,L"pg")&&
			exlib::strncmp_nocase(off,L"peg")&&
			exlib::strncmp_nocase(off,L"pe")&&
			exlib::strncmp_nocase(off,L"fif")&&
			exlib::strncmp_nocase(off,L"if")))
		{
			return support_type::jpeg;
		}
	}
	if(!exlib::strncmp_nocase(ext,L"bmp"))
	{
		return support_type::bmp;
	}
	return support_type::no;
}
inline support_type supported(char const* ext)
{
	if(*ext==0)
	{
		return support_type::no;
	}
	if(!exlib::strncmp_nocase(ext,"png"))
	{
		return support_type::png;
	}
	if(*ext==L'j')
	{
		auto off=ext+1;
		if(!(exlib::strncmp_nocase(off,"pg")&&
			exlib::strncmp_nocase(off,"peg")&&
			exlib::strncmp_nocase(off,"pe")&&
			exlib::strncmp_nocase(off,"fif")&&
			exlib::strncmp_nocase(off,"if")))
		{
			return support_type::jpeg;
		}
	}
	if(!exlib::strncmp_nocase(ext,"bmp"))
	{
		return support_type::bmp;
	}
	return support_type::no;
}
#endif