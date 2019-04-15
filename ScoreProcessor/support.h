#ifndef SUPPORT_H
#define SUPPORT_H
#include "lib\exstring\exstring.h"
enum class support_type {
	no,png,jpeg,bmp,tiff
};
template<typename Extension>
support_type supported(Extension ext)
{
	if(*ext==0)
	{
		return support_type::no;
	}
	if(!exlib::strncmp_nocase(ext,"png"))
	{
		return support_type::png;
	}
	if(*ext==L'j'||*ext==L'J')
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
	if(!exlib::strncmp_nocase(ext,"tiff")||!exlib::strncmp_nocase(ext,"tif"))
	{
		return support_type::tiff;
	}
	return support_type::no;
}
#endif