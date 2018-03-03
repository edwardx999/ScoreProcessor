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
#ifndef EXFILES_H
#define EXFILES_H
#include <vector>
#ifdef _WINDOWS
#include <Windows.h>
#endif
#include <stdio.h>
namespace exlib {
	/*
	Returns a vector containing the filenames of all files in the first level of the given directory.
	*/
	template<typename String>
	std::vector<String> files_in_dir(String const& path);

	/*
	Returns a String with any consecutive slashes replaced by a single slash.
	*/
	template<typename String,typename U>
	String clean_multislashes(U const* input);

	/*
	Returns a String with any consecutive slashes replaced by a single slash.
	*/
	template<typename String>
	String clean_multislashes(String const& input);

	/*
	Returns any consecutive slahes from the input, and returns the new size of the input string.
	*/
	template<typename T>
	size_t remove_multislashes(T* input);

#ifdef _WINDOWS
	template<typename String>
	std::vector<String> files_in_dir(String const& path)
	{
		String search=path+"*.*";
		HANDLE hFind;
		WIN32_FIND_DATAA fdata;
		hFind=FindFirstFileA(search.c_str(),&fdata);
		std::vector<String> files;
		if(hFind!=INVALID_HANDLE_VALUE)
		{
			FindNextFileA(hFind,&fdata);
			FindNextFileA(hFind,&fdata);//gets rid of . and ..
			do
			{
				if(!(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
				{
					files.emplace_back(fdata.cFileName);
				}
			} while(FindNextFileA(hFind,&fdata));
		}
		FindClose(hFind);
		return files;
	}
#endif

	template<typename String,typename U>
	String clean_multislashes(U const* input)
	{
		String out;
		out.reserve(30);
		bool found=false;
		while(*input)
		{
			if(found)
			{
				if(*input!='\\'&&*input!='/')
				{
					found=false;
					out.push_back(*input);
				}
			}
			else
			{
				if(*input=='\\'||*input=='/')
				{
					found=true;
				}
				out.push_back(*input);
			}
			++input;
		}
		return out;
	}

	template<typename String>
	String clean_multislashes(String const& input)
	{
		return clean_multislahes<String>(input.c_str());
	}

	template<typename T>
	size_t remove_multislashes(T* input)
	{
		T* it=input;
		struct keep {
			T* start;
			T* end;
		};
		std::vector<keep> keeps;
		keeps.push_back({it,nullptr});
		bool found=false;
		int i=0;
		while(*it)
		{
			if(keeps.back().end)//still in valid territory
			{
				if(*it!='/'&&*it!='\\')
				{
					keeps.push_back({it,nullptr});
				}
			}
			else
			{
				if(*it=='/'||*it=='\\')
				{
					keeps.back().end=it+1;
				}
			}
			++it;
		}
		keeps.back().end=it;
		it=input;
		for(auto& k:keeps)
		{
			while(k.start!=k.end)
			{
				*it=*k.start;
				++it;
				++k.start;
			}
		}
		*it=0;
		return it-input;
	}

}
#endif