/*
Copyright(C) 2017 Edward Xie

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef EXSTDSTRING_H
#define EXSTDSTRING_H
#include <string>
#include <array>
#include <iostream>
namespace exlib {
	template<typename stringtype,typename traits,typename allocator>
	std::basic_string<stringtype,traits,allocator> front_padded_string(std::basic_string<stringtype,traits,allocator> const& in,size_t desired_length,stringtype padding);

	template<typename stringtype,typename traits,typename allocator>
	std::basic_string<stringtype,traits,allocator> back_padded_string(std::basic_string<stringtype,traits,allocator> const& in,size_t desired_length,stringtype padding);

	template<typename stype_dest,typename stype_src,typename straits_src,typename salloc_src>
	stype_dest string_cast(std::basic_string<stype_src,straits_src,salloc_src> const&);

	std::string letter_numbering(size_t num);

	template<typename stringtype,typename traits,typename allocator>
	std::basic_string<stringtype,traits,allocator> front_padded_string(std::basic_string<stringtype,traits,allocator> const& in,size_t numpadding,stringtype padding) {
		typedef std::basic_string<stringtype,traits,allocator> bs;
		if(in.size()>=numpadding) return in;
		size_t padding_needed=numpadding-in.size();
		return bs(padding_needed,padding)+in;
	}

	template<typename stringtype,typename traits,typename allocator>
	std::basic_string<stringtype,traits,allocator> back_padded_string(std::basic_string<stringtype,traits,allocator> const& in,size_t numpadding,stringtype padding) {
		typedef std::basic_string<stringtype,traits,allocator> bs;
		if(in.size()>=numpadding) return in;
		size_t padding_needed=numpadding-in.size();
		return in+bs(padding_needed,padding);
	}

	template<typename stype_dest,typename stype_src,typename straits_src,typename salloc_src>
	stype_dest string_cast(std::basic_string<stype_src,straits_src,salloc_src> const& in) {
		stype_dest out;
		out.resize(in.size());
		for(size_t i=0;i<in.size();++i)
		{
			out[i]=in[i];
		}
		return out;
	}
}
#endif