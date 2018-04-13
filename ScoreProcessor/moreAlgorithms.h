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
#ifndef MORE_ALGORITHMS_H
#define MORE_ALGORITHMS_H
#include <vector>
#include <limits>
#include <algorithm>
#include "ImageUtils.h"
#include "CImg.h"
namespace misc_alg {

/*
	Returns absolute difference of two numbers
*/
	template<typename T1,typename T2> constexpr auto abs_dif(T1 x,T2 y) ->
		decltype(x-y)
	{
		return (x>y?x-y:y-x);
	}
/**
	Rounds number to nearest radix
	@param x, the number to be rounded
	@param radix
	@return the number round with same type as the radix
*/
	template<typename T1,typename T2> inline T2 round_to_nearest(T1 const x,T2 const radix)
	{
		return ::std::round(static_cast<double>(x)/radix)*radix;
	}
	template<typename T1,typename T2,typename T3> bool within_perc_tolerance(T1 const num1,T2 const num2,T3 const tolerance);
	template<typename T,typename val> int shortest_path_one_dir(::std::vector<T,::std::allocator<T>>& resultContainer,::std::vector<::std::vector<T,::std::allocator<T>>,::std::allocator<::std::vector<T,::std::allocator<T>>>>& nodes,val(*nodeDistance)(T&,T&));
	/*
		Returns the indices of the minimum n in a given vector
	*/
	template<typename T,typename alloc> ::std::vector<unsigned int> min_n_ind(::std::vector<T,alloc> const& values,size_t const num_values);
	template<typename T1,typename T2,typename T3> inline bool within_perc_tolerance(T1 const num1,T2 const num2,T3 const tolerance)
	{
		return (abs_dif(num1,num2)/num1)<tolerance;
	}
	template<typename T,typename val>
	/*
		Greedy shortest path if you can only go from a node in one layer to a node in the next layer
	*/
	int shortest_path_one_dir(::std::vector<T,::std::allocator<T>>& resultContainer,::std::vector<::std::vector<T,::std::allocator<T>>,::std::allocator<::std::vector<T,::std::allocator<T>>>>& nodes,val(*nodeDistance)(T&,T&))
	{
#ifdef USESAFETYCHECKS
		if(nodes.size()<1||nodes[0].size()<1)
		{
			return 1;
		}
#endif // USESAFETYCHECKS
		struct DynamicNode {
			val distance;
			unsigned int prevNodeIndex;
		};
		::std::vector<::std::vector<DynamicNode>> dynamicPather;
		dynamicPather.resize(nodes.size());
		dynamicPather[0].resize(nodes[0].size());
		for(unsigned int n=0;n<nodes[0].size();++n)
		{
			dynamicPather[0][n].distance=0;
		}
		for(unsigned int x=1;x<nodes.size();++x)
		{
#ifdef USESAFETYCHECKS
			if(nodes[x].size()<1)
			{
				return 1;
			}
#endif // USESAFETYCHECKS
			dynamicPather[x].resize(nodes[x].size());
			for(unsigned int n=0;n<nodes[x].size();++n)
			{
				val minValue=nodeDistance(nodes[x][n],nodes[x-1][0])+dynamicPather[x-1][0].distance;
				val minIndex=0;
				for(unsigned int nprev=1;nprev<nodes[x-1].size();++nprev)
				{
					val dist=nodeDistance(nodes[x][n],nodes[x-1][nprev])+dynamicPather[x-1][nprev].distance;
					if(dist<minValue)
					{
						minIndex=nprev;
						minValue=dist;
					}
				}
				dynamicPather[x][n].distance=minValue;
				dynamicPather[x][n].prevNodeIndex=minIndex;
			}
		}
		val minDistance=dynamicPather.back()[0].distance;
		unsigned int minIndex=0;
		for(unsigned int i=1;i<dynamicPather.back().size();++i)
		{
			if(dynamicPather.back()[i].distance<minDistance)
			{
				minDistance=dynamicPather.back()[i].distance;
				minIndex=i;
			}
		}
		resultContainer.resize(dynamicPather.size());
		unsigned int nextIndex=dynamicPather.back()[minIndex].prevNodeIndex;
		resultContainer.back()=nodes.back()[minIndex];
		for(unsigned int x=dynamicPather.size()-2;x<dynamicPather.size();--x)
		{
			resultContainer[x]=nodes[x][nextIndex];
			nextIndex=dynamicPather[x][nextIndex].prevNodeIndex;
		}
		return 0;
	}
	template<typename T,typename alloc> ::std::vector<unsigned int> min_n_ind(::std::vector<T,alloc> const& values,size_t const num_values)
	{
		vector<unsigned int> mins;
		if(values.size()==0)
		{
			return mins;
		}
		mins.reserve(num_values+1);
		mins.push_back(0);
		size_t max=values.size();
		for(auto i=1U;i<max;++i)
		{
			auto place=mins.end();
			do
			{
				if(values[i]>=values[*--place])
				{
					mins.insert(place+1,i);
					goto skip_insert_begin;
				}
			} while(place!=mins.begin());
			mins.insert(place,i);
		skip_insert_begin:
			if(mins.size()>num_values)
			{
				mins.erase(mins.end()-1);
			}
		}
		return mins;
	}
}
namespace cimg_library {
/*
	Traces a seam from right to left starting at y start_index
	@param map, energy map
	@param start_index, y coordinate to start seam
	@return vector containing y coordinates of seams, starts at x=0
*/
	template<typename T> ::std::vector<unsigned int> trace_back_seam(CImg<T> const& map,unsigned int start_index);
	template<typename T> ::std::vector<unsigned int> create_seam(CImg<T> const& map);
	template<typename T> void mark_seam(CImg<T>& img,T const* values,::std::vector<unsigned int> const& seam);
	/*
		MAP IS MODIFIED
	*/
	template<typename T> void min_energy_to_right(CImg<T>& map);
	template<typename T,typename Selector> 
	void accumulate_to_right(CImg<T>& dst,CImg<T> const& src,ImageUtils::Rectangle<unsigned int> const area,Selector s);
	template<typename T> void sandpile(CImg<T>& sandPiles,T maxSize);

	template<typename T>
	::std::vector<unsigned int> create_seam(CImg<T> const& energyMap)
	{
		unsigned int x=energyMap._width-1;
		T minValue=energyMap(x,0);
		unsigned int index=0;
		for(unsigned int y=1;y<energyMap._height;++y)
		{
			if(energyMap(x,y)<minValue)
			{
				minValue=energyMap(x,y);
				index=y;
			}
		}
		return trace_back_seam(energyMap,index);
	}
	template<typename T>
	void min_energy_to_right(CImg<T>& energyGraph)
	{
		ImageUtils::Rectangle<unsigned int> area={0,energyGraph._width,0,energyGraph._height};
		accumulate_to_right(energyGraph,energyGraph,area,[](T a,T b)
		{
			return std::min(a,b);
		});
	}
	template<typename T,typename Selector>
	void accumulate_to_right(CImg<T>& dst,CImg<T> const& src,ImageUtils::Rectangle<unsigned int> const area,Selector s)
	{
		unsigned int const width=dst._width;
		unsigned int const foffset=area.top*width;
		T* const dst_first_row=dst._data+foffset;
		T const* const src_first_row=src._data+foffset;
		T const* const src_first_row1=src_first_row+width;
		unsigned int const offset=(area.bottom-1)*width;
		T* const dst_last_row=dst._data+offset;
		T const* const src_last_row=src._data+offset;
		T const* const src_last_row1=src_last_row-width;
		for(unsigned int x=area.left+1;x<area.right;++x)
		{
			unsigned int const prev=x-1;
			dst_first_row[x]=src_first_row[x]+s(src_first_row[prev],src_first_row1[prev]);
			T* drow=dst_first_row+width+x;
			T const* srow=src_first_row1+prev;
			for(;;)
			{
				*drow=srow[1]+s(s(*(srow-width),*srow),*(srow+width));
				drow+=width;
				if(drow>=dst_last_row)
				{
					break;
				}
				srow+=width;
			}
			dst_last_row[x]=src_last_row[x]+s(src_last_row[prev],src_last_row1[prev]);
		}
	}
	template<typename T> ::std::vector<unsigned int> trace_back_seam(CImg<T> const& energyMap,unsigned int startIndex)
	{
		::std::vector<unsigned int> resultPath(energyMap._width);
		T minValue,pathValue=minValue=energyMap(energyMap._width-1,startIndex);
		resultPath.back()=startIndex;
		for(unsigned int x=energyMap._width-2;x<energyMap._width;--x)
		{
			unsigned int y=(startIndex==0?0:startIndex-1);
			unsigned int yLim=(startIndex<energyMap._height-1?startIndex+1:startIndex);
			minValue=energyMap(x,startIndex=y++);
			for(;y<=yLim;++y)
			{
				if(energyMap(x,y)<=minValue)
				{
					minValue=energyMap(x,y);
					startIndex=y;
				}
			}
			resultPath[x]=startIndex;
		}
		return resultPath;
	}
	template<typename T> void mark_seam(CImg<T>& img,T const* values,::std::vector<unsigned int> const& seam)
	{
		for(unsigned int x=0;x<seam.size();++x)
		{
			for(unsigned int l=0;l<img._spectrum;++l)
			{
				img(x,seam[x],l)=values[l];
			}
		}
	}
	template<typename T>
	void sandpile(CImg<T>& sandPiles,T maxSize)
	{
		while(true)
		{
			bool allFallen=true;
			for(unsigned int x=0;x<sandPiles._width;++x)
			{
				for(unsigned int y=0;y<sandPiles._height;++y)
				{
					if(sandPiles(x,y)>maxSize)
					{
						sandPiles(x,y)-=5;
						allFallen=false;
						for(int dx=-1;dx<=1;++dx)
						{
							for(int dy=-1;dy<=1;++dy)
							{
								unsigned int changedX=x+dx;
								unsigned int changedY=y+dy;
								if(changedX<sandPiles._width&&changedY<sandPiles._height)
								{
									sandPiles(changedX,changedY)+=1;
								}
							}
						}
					}
				}
			}
			if(allFallen) return;
		}
	}
}
#endif // !MORE_ALGORITHMS_H
