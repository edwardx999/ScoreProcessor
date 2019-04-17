#ifndef SUPPORT_H
#define SUPPORT_H
#include "lib\exstring\exstring.h"
#include "lib\exstring\exfiles.h"
#include "lib\exstring\exalg.h"
enum class support_type {
	no,png,jpeg,bmp,tiff
};

struct no_case_compare {
	template<typename T,typename U>
	constexpr int operator()(T a,U b) const
	{
		return exlib::strncmp_nocase(a,b);
	}
};
using support_map_kp=exlib::map_pair<char const*,support_type>;
inline constexpr auto CTMap=exlib::make_ct_map<no_case_compare>(
	support_map_kp{"png",support_type::png},
	support_map_kp{"jpeg",support_type::jpeg},
	support_map_kp{"jpg",support_type::jpeg},
	support_map_kp{"jpe",support_type::jpeg},
	support_map_kp{"jfif",support_type::jpeg},
	support_map_kp{"jif",support_type::jpeg},
	support_map_kp{"bmp",support_type::bmp},
	support_map_kp{"tiff",support_type::tiff},
	support_map_kp{"tif",support_type::tiff}
	);

template<typename Extension>
support_type supported(Extension ext)
{
	auto const iter=CTMap.find(ext);
	if(iter==CTMap.end())
	{
		return support_type::no;
	}
	return iter->value();
}

template<typename Extension>
support_type validate_extension(Extension ext)
{
	auto const iter=CTMap.find(ext);
	if(iter==CTMap.end())
	{
		auto const strlen=exlib::strlen(ext);
		throw std::invalid_argument{std::string{"Unsupported file type: "}.append(ext,ext+strlen)};
	}
	return iter->value();
}

template<typename Path>
support_type supported_path(Path begin,Path end)
{
	return supported(exlib::find_extension(begin,end));
}

template<typename Path>
support_type supported_path(Path begin)
{
	auto const strlen=exlib::strlen(begin);
	return supported(exlib::find_extension(begin,begin+strlen));
}

template<typename Path>
support_type validate_path(Path begin)
{
	auto const strlen=exlib::strlen(begin);
	return validate_extension(exlib::find_extension(begin,begin+strlen));
}

template<typename Path>
support_type validate_path(Path begin,Path end)
{
	return validate_extension(exlib::find_extension(begin,end));
}
#endif