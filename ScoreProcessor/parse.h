/*
Copyright(C) 2017-2018 Edward Xie

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
#ifndef SCORE_PARSE_H
#define SCORE_PARSE_H
#include <numeric>
#include <cctype>
#include <charconv>
namespace ScoreProcessor {
	//I really need MSVC to implement from_chars.
	int parse_str(double& out,char const* str)
	{
		int& errno_ref=errno;
		errno_ref=0;
		char* end;
		out=std::strtod(str,&end);
		return errno_ref||str==end||(*end!='\0'&&!std::isspace(*end));
	}
	int parse_str(float& out,char const* str)
	{
		int& errno_ref=errno;
		errno_ref=0;
		char* end;
		out=std::strtof(str,&end);
		return errno_ref||str==end||(*end!='\0'&&!std::isspace(*end));
	}
	int parse_str(unsigned long& out,char const* str)
	{
		int& errno_ref=errno;
		errno_ref=0;
		char* end;
		out=std::strtoul(str,&end,10);
		return errno_ref||str==end||(*end!='\0'&&!std::isspace(*end));
	}
	int parse_str(long& out,char const* str)
	{
		int& errno_ref=errno;
		errno_ref=0;
		char* end;
		out=std::strtol(str,&end,10);
		return errno_ref||str==end||(*end!='\0'&&!std::isspace(*end));
	}
#define make_parse_str_signed(type)\
	int parse_str(##type##& out,char const* str){\
		int& errno_ref=errno;\
		errno_ref=0;\
		char* end;\
		auto temp=std::strtol(str,&end,10);\
		if(errno_ref)\
		{\
			return errno_ref;\
		}\
		if(end==str||(*end!='\0'&&!std::isspace(*end))||temp>std::numeric_limits<##type##>::max()||temp<std::numeric_limits<##type>::min())\
		{\
			return 1;\
		}\
		out=temp;\
		return 0;\
	}
	make_parse_str_signed(char)
		make_parse_str_signed(short)
		make_parse_str_signed(int)
#undef make_parse_str_signed
#define make_parse_str_unsigned(type)\
	int parse_str(##type##& out,char const* str){\
		int& errno_ref=errno;\
		errno_ref=0;\
		char* end;\
		auto temp=std::strtoul(str,&end,10);\
		if(errno_ref)\
		{\
			return errno_ref;\
		}\
		if(end==str||(*end!='\0'&&!std::isspace(*end))||*str=='-'||temp>std::numeric_limits<##type##>::max())\
		{\
			return 1;\
		}\
		out=temp;\
		return 0;\
	}
		make_parse_str_unsigned(unsigned char)
		make_parse_str_unsigned(unsigned short)
		make_parse_str_unsigned(unsigned int)
		//-1 is success
		//err val 0 is too few arguments
		//err val 1 is too many arguments
		//err val n in [2,num_args+1] mean missing (n-2)th argument 
		//err val n in [num_args+2,2*num_args+1] means invalid (n-num_args-2)th argument
		//other err vals are defined by the given constraints function
		template<typename T,size_t num_args,typename Func>
	int parse_range(
		std::array<T,num_args>& out,
		std::string& str,
		std::array<std::optional<T>,num_args> const& default_values,
		Func constraints) noexcept
	{
		static_assert(num_args,"Must have positive number of args");
		size_t comma_pos=0;
		for(size_t i=0;i<num_args-1;++i)
		{
			auto new_comma_pos=str.find(',',comma_pos);
			if(new_comma_pos==std::string::npos)
			{
				return 0;
			}
			str[new_comma_pos]='\0';
			if(new_comma_pos==comma_pos)
			{
				if(default_values[i].has_value())
				{
					out[i]=default_values[i].value();
				}
				else
				{
					str[new_comma_pos]=',';
					return i+2;
				}
			}
			else
			{
				if(parse_str(out[i],str.data()+comma_pos))
				{
					str[new_comma_pos]=',';
					return num_args+i+2;
				}
			}
			comma_pos=new_comma_pos+1;
		}
		auto last=str.find(',',comma_pos);
		if(last!=std::string::npos)
		{
			return 1;
		}
		if(comma_pos==str.size())
		{
			if(default_values.back().has_value())
			{
				out.back()=default_values.back().value();
			}
			else
			{
				return num_args+1;
			}
		}
		else
		{
			if(parse_str(out.back(),str.data()+comma_pos))
			{
				return 2*num_args+1;
			}
		}
		return constraints(out);
	}
}
#endif