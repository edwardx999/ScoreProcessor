/*
Copyright 2018 Edward Xie

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef EXMATH_H
#define EXMATH_H
#include <type_traits>
#include <vector>
#include <functional>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <assert.h>
#include <cmath>
#ifdef _MSVC_LANG
#define _EXMATH_HAS_CPP_20 (_MSVC_LANG>=202000l)
#define _EXMATH_HAS_CPP_17 (_MSVC_LANG>=201700l)
#define _EXMATH_HAS_CPP_14 (_MSVC_LANG>=201400l)
#else
#define _EXMATH_HAS_CPP_20 (__cplusplus>=202000l)
#define _EXMATH_HAS_CPP_17 (__cplusplus>=201700l)
#define _EXMATH_HAS_CPP_14 (__cplusplus>=201400l)
#endif

#define EX_CONSTEXPR constexpr

#if _EXMATH_HAS_CPP_17
#define EX_CONSTIF constexpr
#else
#define EX_CONSTIF
#endif
namespace exlib {

	/*
		Represents the ring of integers mod mod
		Using Base as a base representation
	*/
	template<typename Base,unsigned long long mod>
	class mod_ring {
		Base _val;
	public:
		template<typename T>
		constexpr mod_ring(T&& val):_val(std::forward<T>(val))
		{
			_val%=mod;
		}
		constexpr mod_ring():_val()
		{}
	private:
		//used to construct when it is known that the input is already in modulo range
		struct no_mod_tag {};
		template<typename T>
		constexpr mod_ring(T&& val,no_mod_tag):_val(std::forward<T>(val))
		{}
	public:
#define comp(op) constexpr bool operator op (mod_ring const& o) const { return _val op o;}
		comp(<)
			comp(>)
			comp(<=)
			comp(>=)
			comp(==)
			comp(!=)
#undef comp
			constexpr operator Base const&() const
		{
			return _val;
		}
		constexpr mod_ring& operator*=(mod_ring const& o)
		{
			(_val*=o._val)%=mod;
			return *this;
		}
		constexpr mod_ring& operator+=(mod_ring const& o)
		{
			_val+=o._val;
			if(_val>=mod)
			{
				_val-=mod;
			}
			return *this;
		}
		constexpr mod_ring operator /=(mod_ring const& o) const
		{
			_val/=o._val;
			return *this;
		}
		constexpr mod_ring operator -=(mod_ring const& o) const
		{
			if(_val>=o._val)
			{
				_val-=o._val;
				return *this;
			}
			_val=mod-(o._val-_val);
			return *this;
		}
		constexpr mod_ring operator+(mod_ring const& o) const&
		{
			Base t=_val+o._val;
			if(t>=mod) t-=mod;
			return mod_ring{std::move(t),no_mod_tag{}};
		}
		constexpr mod_ring operator*(mod_ring const& o) const&
		{
			return mod_ring{o._val*_val};
		}
		constexpr mod_ring operator /(mod_ring const& o) const&
		{
			return mod_ring{_val/o._val,no_mod_tag{}};
		}
		constexpr mod_ring operator-(mod_ring const& o) const&
		{
			if(_val>=o._val)
			{
				return mod_ring{_val-o._val,no_mod_tag{}};
			}
			return mod_ring{mod-(o._val-_val),no_mod_tag{}};
		}
#define cassmd(op) constexpr mod_ring operator op(mod_ring const& o)&& { mod_ring ret(std::move(*this)); ret op ## =o; return ret;}
		cassmd(+)
		cassmd(-)
		cassmd(*)
		cassmd(/)
#undef cassmd
	};

	template<typename T>
	constexpr T additive_identity()
	{
		return T{0};
	}

	template<typename T>
	constexpr T multiplicative_identity()
	{
		return T{1};
	}

	//integral exponentiation a^n
	template<typename T>
	constexpr T ipow(T const& a,unsigned long long n)
	{
		if(n==0)
		{
			return multiplicative_identity<T>();
		}
		if(n==1)
		{
			return a;
		}
		auto x=a;
		auto y=multiplicative_identity<T>();
		do
		{
			if(n&1)
			{
				y*=x;
				x*=x;
				(n-=1)/=2;
			}
			else
			{
				x*=x;
				n/=2;
			}
		} while(n>1);
		return x*y;
	}

	template<typename T>
	constexpr T abs(T a)
	{
		if(a<0)
		{
			return -std::move(a);
		}
		return std::move(a);
	}

	namespace detail {
		template<typename R,typename T,typename U,typename V,typename W>
		struct min_modular_distance_type {
			typedef R type;
		};
		template<typename T,typename U,typename V,typename W>
		struct min_modular_distance_type<void,T,U,V,W> {
			typedef typename std::make_signed<typename std::common_type<T,U,V,W>::type>::type type;
		};
	}
	/*
		Finds the lowest magnitude c such that a+c=b in the modular group spanning [min,max),
		assuming a and b are in the modular group.
	*/
	template<typename R=void,typename T,typename U,typename V,typename W>
	constexpr auto min_modular_distance(T const& a,U const& b,V const& min,W const& max)
	{
		assert(min<=max);
		assert(a>=min&&a<=max);
		assert(b>=min&&b<=max);
		typedef typename detail::min_modular_distance_type<R,T,U,V,W>::type Ret;
		if(a==b)
		{
			return Ret{0};
		}
		if(a<b)
		{
			Ret forward_dist=b-a;
			Ret backward_dist=a-min+max-b;
			if(forward_dist<=backward_dist)
			{
				return forward_dist;
			}
			return backward_dist*=-1; //if they are allocating BigIntegers, prevents reallocation, hopefully
		}
		else
		{
			Ret forward_dist=max-a+b-min;
			Ret backward_dist=a-b;
			if(forward_dist<=backward_dist)
			{
				return forward_dist;
			}
			return backward_dist*=-1;
		}
	}

	/*
		Finds the lowest magnitude c such that a+c=b in the modular group spanning [0,max),
		assuming a and b are in the modular group.
	*/
	template<typename T,typename U,typename V>
	constexpr auto min_modular_distance(T const& a,U const& b,V const& max)
	{
		return min_modular_distance(a,b,V(0),max);
	}

	template<typename T,typename U>
	constexpr typename std::decay<U>::type clamp(T&& val,U&& min,U&& max)
	{
		assert(min<=max);
		if(val>max)
		{
			return std::forward<U>(max);
		}
		else if(val<min)
		{
			return std::forward<U>(min);
		}
		return static_cast<typename std::decay<U>::type>(std::forward<T>(val));
	}

	template<typename U,typename T>
	constexpr U clamp(T val)
	{
		if(val>std::numeric_limits<U>::max())
		{
			return std::numeric_limits<U>::max();
		}
		if(val<std::numeric_limits<U>::min())
		{
			return std::numeric_limits<U>::min();
		}
		return std::move(val);
	}

	template<typename T1,typename T2>
	constexpr auto abs_dif(T1 x,T2 y) -> decltype(x-y)
	{
		return (x>y?x-y:y-x);
	}

	//coerce forces VarType into FixedType by rounding
	template<typename FixedType,typename VarType>
	struct coerce_value {
		static constexpr FixedType coerce(VarType vt)
		{
			return FixedType(std::round(vt));
		}
	};

	//A value that has a fixed or variant value. If the value is not fixed (index!=fixed_index), its value is determined by multiplying
	//variant() by bases[index]
	//constexpr currently not allowed for unions so some of them do not work; just ignore for it as it doesn't cause a compiler error
	template<typename FixedType=int,typename VarType=float,typename IndexType=unsigned int,typename CoerceValue=coerce_value<FixedType,VarType>>
	struct maybe_fixed:protected CoerceValue {
	public:
		static constexpr IndexType const fixed_index=-1;
	private:
		struct ft_const {};
		struct vt_const {};
		IndexType _index;
		union {
			FixedType ft;
			VarType vt;
		};
	public:
		//fixed value at 0
		constexpr maybe_fixed():_index(fixed_index),ft(0)
		{}
		//creates a fixed value
		constexpr maybe_fixed(FixedType ft):_index(fixed_index),ft(ft)
		{}
		//creates a variable value with index
		constexpr maybe_fixed(VarType vt,IndexType index):_index(index),vt(vt)
		{}
		//whether the value is fixed
		constexpr bool fixed() const
		{
			return _index==fixed_index;
		}
		//the variable index
		constexpr IndexType index() const
		{
			return _index;
		}
		//sets the variable index
		constexpr void index(IndexType idx)
		{
			_index=idx;
		}
		//if the value is fixed, returns that fixed value. Otherwises returns bases[index()]*variant().
		//index value checked
		template<std::size_t N>
		constexpr FixedType value(std::array<FixedType,N> const& bases) const
		{
			if(_index==fixed_index)
			{
				return ft;
			}
			if(_index<N)
			{
				return CoerceValue::coerce(bases[_index]*vt);
			}
			throw std::out_of_range("Index too high");
		}
		//automatic conversion to array for convenience
		template<typename... Bases>
		constexpr FixedType value(Bases... bases) const
		{
			return value(std::array<FixedType,sizeof...(Bases)>{
				{
					bases...
				}});
		}

		//if the value is fixed, returns that fixed value. Otherwises returns bases[index()]*variant().
		//index value unchecked
		template<std::size_t N>
		constexpr FixedType operator()(std::array<FixedType,N> const& bases) const
		{
			if(_index==fixed_index)
			{
				return ft;
			}
			return CoerceValue::coerce(bases[_index]*vt);
		}
		//automatic conversion to array for convenience
		template<typename... Bases>
		constexpr FixedType operator()(Bases... bases) const
		{
			return operator()(std::array<FixedType,sizeof...(Bases)>{
				{
					bases...
				}});
		}

		//fixes the value to be ft
		constexpr void fix(FixedType ft)
		{
			_index=fixed_index;
			this->ft=ft;
		}

		//does nothing if value is already fixed, otherwises fixes the value to bases[index]*variant()
		template<std::size_t N>
		constexpr void fix_from(std::array<FixedType,N> const& bases)
		{
			if(_index==fixed_index) return;
			ft=operator()(bases);
			_index=fixed_index;
		}

		//automatic conversion to array for convenience
		template<typename... Bases>
		constexpr void fix_from(Bases... bases)
		{
			fix_from(std::array<FixedType,sizeof...(Bases)>{
				{
					bases...
				}});
		}

		//the variable amount, undefined if value is fixed
		constexpr VarType variant() const
		{
			return vt;
		}

		//sets the variable amount and basis to use
		constexpr void variant(VarType vt,IndexType index=1)
		{
			_index=index;
			this->vt=vt;
		}
	};
	enum class conv_error {
		none,out_of_range,invalid_characters
	};

	struct conv_res {
		conv_error ce;
		char const* last;
		operator bool() const
		{
			return ce!=conv_error::none;
		}
	};

	//the following parse algorithms take in null-terminated strings
	inline conv_res parse(char const* str,double& out)
	{
		int& err=errno;
		err=0;
		char* end;
		double res=std::strtod(str,&end);
		if(err==ERANGE)
		{
			return {conv_error::out_of_range,end};
		}
		if(end==str)
		{
			return {conv_error::invalid_characters,end};
		}
		out=res;
		return {conv_error::none,end};
	}

	inline conv_res parse(char const* str,float& out)
	{
		int& err=errno;
		err=0;
		char* end;
		float res=std::strtof(str,&end);
		if(err==ERANGE)
		{
			return {conv_error::out_of_range,end};
		}
		if(end==str)
		{
			return {conv_error::invalid_characters,end};
		}
		out=res;
		return {conv_error::none,end};
	}

	template<typename T>
	auto parse(char const* str,T& out,int base=10) -> typename std::enable_if<std::is_unsigned<T>::value,conv_res>::type
	{
		int& err=errno;
		err=0;
		char* end;
		unsigned long long res=std::strtoull(str,&end,base);
		if(err==ERANGE)
		{
			return {conv_error::out_of_range,end};
		}
		if(end==str)
		{
			return {conv_error::invalid_characters,end};
		}
		constexpr unsigned long long max=std::numeric_limits<T>::max();
		constexpr unsigned long long ullmax=std::numeric_limits<unsigned long long>::max();
		if EX_CONSTIF(max<ullmax)
		{
			if(res>max)
			{
				return {conv_error::out_of_range,end};
			}
		}
		out=res;
		return {conv_error::none,end};
	}

	template<typename T>
	auto parse(char const* str,T& out,int base=10) -> typename std::enable_if<std::is_signed<T>::value,conv_res>::type
	{
		int& err=errno;
		err=0;
		char* end;
		long long res=std::strtoll(str,&end,base);
		if(err==ERANGE)
		{
			return {conv_error::out_of_range,end};
		}
		if(end==str)
		{
			return {conv_error::invalid_characters,end};
		}

		constexpr long long max=std::numeric_limits<T>::max();
		constexpr long long llmax=std::numeric_limits<unsigned long long>::max();
		if EX_CONSTIF(max<llmax)
		{
			if(res>max)
			{
				return {conv_error::out_of_range,end};
			}
		}
		constexpr long long min=std::numeric_limits<T>::min();
		constexpr long long llmin=std::numeric_limits<unsigned long long>::min();
		if EX_CONSTIF(min>llmin)
		{
			if(res<min)
			{
				return {conv_error::out_of_range,end};
			}
		}
		out=res;
		return {conv_error::none,end};
	}


	namespace detail {
		template<typename Target,typename Base,bool is_unsigned=std::is_unsigned<Target>::value>
		struct bring_unsignedness {
			using type=Base;
		};

		template<typename Target,typename Base>
		struct bring_unsignedness<Target,Base,true> {
			using type=typename std::make_unsigned<Base>::type;
		};
	}

#if _EXMATH_HAS_CPP_14
	template<typename T>
	constexpr unsigned int num_digits(T num,typename detail::bring_unsignedness<T,int>::type base=10)
	{
		//static_assert(std::is_integral<T>::value,"Requires integral type");
		unsigned int num_digits=1;
		while((num/=base)!=0)
		{
			++num_digits;
		}
		return num_digits;
	}
#else
	template<typename T>
	constexpr unsigned int num_digits(T num,typename detail::bring_unsignedness<T,int>::type base=10)
	{
		return num<0?num_digits(-num):((num<base)?1:(1+num_digits(num/base)));
	}
#endif
#if _EXMATH_HAS_CPP_14

	namespace detail {
		template<typename CharType>
		constexpr auto make_digit_array()
		{
			std::array<CharType,'9'-'0'+1+'z'-'a'+1> arr{{}};
			std::size_t pos=0;
			for(CharType i='0';i<='9';++pos,++i)
			{
				arr[pos]=i;
			}
			for(CharType i='a';i<='z';++pos,++i)
			{
				arr[pos]=i;
			}
			return arr;
		}

		template<typename Char>
		struct digit_array_holder {
			constexpr static auto digits=make_digit_array<Char>();
		};

		template<typename Iter>
		using iter_val_t=typename std::iterator_traits<Iter>::value_type;

		template<typename Iter,typename Num,typename DigitsIter>
		constexpr Iter fill_num_array_unchecked_positive(Iter end,Num num,int base,DigitsIter digits)
		{
			--end;
			while(true)
			{
				*end=digits[num%base];
				num/=base;
				if(num==0)
				{
					return end;
				}
				--end;
			};
		}
		template<typename Iter,typename Num,typename DigitsIter>
		constexpr Iter fill_num_array_unchecked_negative(Iter end,Num num,int base,DigitsIter digits)
		{
			--end;
			while(true)
			{
				if EX_CONSTIF(-1%10==-1)
				{
					*end=digits[-(num%base)];
				}
				else
				{
					*end=digits[base-num%base];
				}
				num/=base;
				if(num==0)
				{
					return end;
				}
				--end;
			};
		}
	}

	//fills in a range working backwards from end and returns the start iter written to
	//unchecked, make sure your range can fit
	template<typename Iter,typename Num,typename DigitsIter=detail::iter_val_t<Iter> const*>
	constexpr Iter fill_num_array_unchecked(Iter end,Num num,int base=10,DigitsIter digits=detail::digit_array_holder<detail::iter_val_t<DigitsIter>>::digits.data())
	{
		if(num>=0)
		{
			return detail::fill_num_array_unchecked_positive(end,num,base,digits);
		}
		else
		{
			auto it=detail::fill_num_array_unchecked_negative(end,num,base,digits);
			*--it='-';
			return it;
		}
	}
	namespace detail {
		template<typename T,T val,unsigned int base,typename CharType,bool positive=(val>=0)>
		struct to_string_helper {
			template<typename DigitsIter>
			constexpr static auto get(DigitsIter digits)
			{
				std::array<CharType,num_digits(val,base)+1> number{{}};
				number.back()='\0';
				fill_num_array_unchecked(number.end()-1,val,base,digits);
				return number;
			}
		};

		template<typename T,T val,unsigned int base,typename CharType>
		struct to_string_helper<T,val,base,CharType,false> {
			template<typename DigitsIter>
			constexpr static auto get(DigitsIter digits)
			{
				std::array<CharType,num_digits(val,base)+2> number{{}};
				number.back()='\0';
				fill_num_array_unchecked(number.end()-1,val,base,digits);
				number.front()='-';
				return number;
			}
		};

		template<typename T,T val,unsigned int base,typename CharType,typename DigitsIter>
		constexpr auto to_string(DigitsIter digits)
		{
			return to_string_helper<T,val,base,CharType>::get(digits);
		}
	}
#endif
#if _EXMATH_HAS_CPP_17
	//converts to a compile-time const array of digits representing the number
	template<auto val,unsigned int base=10,typename CharType=char,typename DigitsIter=CharType const*>
	constexpr auto to_string(DigitsIter digits=detail::digit_array_holder<detail::iter_val_t<DigitsIter>>::digits.data())
	{
		return detail::to_string<decltype(val),val,base,CharType>(digits);
	}

	//converts to a compile-time const array of digits representing the number
	template<auto val,typename CharType,unsigned int base=10,typename DigitsIter=CharType const*>
	constexpr auto to_string(DigitsIter digits=detail::digit_array_holder<detail::iter_val_t<DigitsIter>>::digits.data())
	{
		return to_string<val,base,CharType>(digits);
	}
#endif
#if _EXMATH_HAS_CPP_14
	//converts to a compile-time const array of digits representing the number
	template<long long val,unsigned int base=10,typename CharType=char,typename DigitsIter=CharType const*>
	constexpr auto to_string_signed(DigitsIter digits=detail::digit_array_holder<detail::iter_val_t<DigitsIter>>::digits.data())
	{
		return detail::to_string<decltype(val),val,base,CharType>(digits);
	}
	//converts to a compile-time const array of digits representing the number
	template<unsigned long long val,unsigned int base=10,typename CharType=char,typename DigitsIter=CharType const*>
	constexpr auto to_string_unsigned(DigitsIter digits=detail::digit_array_holder<detail::iter_val_t<DigitsIter>>::digits.data())
	{
		return detail::to_string<decltype(val),val,base,CharType>(digits);
	}
#endif

	namespace detail {
#ifdef NDEBUG
		using std::min_element;
#else
		template<typename Iter,typename Comp> //in debug mode msvc complains about using <= as a comparison operator
		constexpr Iter min_element(Iter begin,Iter end,Comp c)
		{
			if(begin==end)
			{
				return begin;
			}
			auto min_el=begin;
			++begin;
			for(; begin!=end; ++begin)
			{
				if(c(*begin,*min_el))
				{
					min_el=begin;
				}
			}
			return min_el;
		}
#endif
		template<typename Iter,typename Comp>
		constexpr Iter get_end_swap_if(Iter it,Iter end,std::size_t radius,Iter& current_min,Comp c)
		{
			if(static_cast<std::size_t>(end-it)>radius)
			{
				auto const before_range_end=it+radius;
				if(c(*before_range_end,*current_min))
				{
					current_min=before_range_end;
				}
				return before_range_end+1;
			}
			else
			{
				return end;
			}
		}
		template<typename InputIter,typename OutputIter,typename Comp>
		constexpr auto get_fatten(InputIter begin,InputIter end,std::size_t radius,OutputIter out,Comp c) -> typename std::enable_if<std::is_convertible<typename std::iterator_traits<InputIter>::iterator_category&,std::random_access_iterator_tag&>::value,OutputIter>::type
		{
			auto current_min=std::min_element(begin,begin+radius,c);
			for(auto it=begin; it<end; ++it,++out)
			{
				//[) boundaries of search_range
				auto const range_begin=(static_cast<std::size_t>(it-begin))<=radius?begin:it-radius;
				//comparison to last element done with this function
				auto const range_end=get_end_swap_if(it,end,radius,current_min,c);
				if(current_min<range_begin)
				{
					current_min=min_element(range_begin,range_end,c);
				}
				*out=*current_min;
			}
			return out;
		}

		template<typename InputIter,typename OutputIter,typename Comp,typename... Extra>
		auto get_fatten(InputIter begin,InputIter end,std::size_t radius,OutputIter out,Comp c,Extra...)
		{
			auto range_begin=begin;
			auto range_end=begin;
			std::advance(range_end,radius);
			auto el=std::min_element(range_begin,range_end,c);
			std::size_t forepadding=0;
			for(auto it=begin; it!=end; ++it)
			{
				if(range_end!=end)
				{
					if(c(*range_end,*el))
					{
						el=range_end;
					}
					++range_end;
				}
				if(forepadding>=radius+1)
				{
					if(el==range_begin)
					{
						++range_begin;
						el=min_element(range_begin,range_end,c);
					}
					else
					{
						++range_begin;
					}
				}
				else
				{
					++forepadding;
				}
				*out=*el;
				++out;
			}
			return out;
		}
	}

	/*
		Places the min element within n elements at each position in OutputIter
		Ranges should not overlap
	*/
	template<typename InputIter,typename OutputIter,typename Comp=std::less_equal<typename std::iterator_traits<InputIter>::value_type>>
	constexpr OutputIter get_fatten(InputIter begin,InputIter end,std::size_t radius,OutputIter out,Comp c={})
	{
		if(begin==end)
		{
			return out;
		}
		if(radius==0)
		{
			return std::copy(begin,end,out);
		}
		std::size_t const distance=std::distance(begin,end);
		if(distance<radius+1) //all elements in the radius of every element
		{
			auto min=std::min_element(begin,end,c);
			return std::fill_n(out,distance,*min);
		}
		return detail::get_fatten(begin,end,radius,out,c);
	}

	/*
		Makes a container in which each element becomes the minimum element within hp of its index
	*/
	template<typename RandomAccessContainer,typename Comp>
	RandomAccessContainer fattened_profile(RandomAccessContainer const& prof,std::size_t hp,Comp comp)
	{
		RandomAccessContainer res(prof.size());
		get_fatten(prof.begin(),prof.end(),hp,res.begin(),comp);
		return res;
	}

	template<typename RandomAccessContainer>
	RandomAccessContainer fattened_profile(RandomAccessContainer const& prof,std::size_t hp)
	{
		typedef typename std::decay<decltype(*prof.begin())>::type T;
		if EX_CONSTIF(std::is_trivially_copyable<T>::value&&sizeof(T)<=sizeof(std::size_t))
		{
			return fattened_profile(prof,hp,[](auto a,auto b)
			{
				return a<=b;
			});
		}
		else
		{
			return fattened_profile(prof,hp,[](auto const& a,auto const& b)
			{
				return a<=b;
			});
		}
	}

	template<typename T,typename Alloc=std::allocator<T>>
	class LimitedSet:std::vector<T,Alloc> {
	private:
		using Base=std::vector<T,Alloc>;
	public:
		using Base::iterator;
		using Base::allocator_type;
		using Base::size_type;
		using Base::difference_type;
		using Base::const_reference;
		using Base::const_pointer;
		using Base::const_iterator;
		using Base::const_reverse_iterator;
	private:
		size_t _max_size;
	public:
		LimitedSet(size_t s):_max_size(s)
		{
			_data.reserve(s);
		}
		LimitedSet():LimitedSet(0)
		{}
		void max_size(size_t s)
		{
			_max_size=s;
			Base::reserve(s+1);
		}
		size_t max_size() const
		{
			return _max_size;
		}
		using Base::cbegin;
		using Base::cend;
		using Base::crbegin;
		using Base::crend;
		using Base::size;
		const_iterator begin() const
		{
			return Base::begin();
		}
		const_iterator end() const
		{
			return Base::begin();
		}
		const_iterator rbegin() const
		{
			return Base::rbegin();
		}
		const_iterator rend() const
		{
			return Base::rbegin();
		}
		using Base::empty;
		using Base::clear;
		using Base::pop_back;
		using Base::erase;
	private:
		template<typename U,typename Comp>
		void _insert(U&& in,Comp comp)
		{
			if(max_size)
			{
				auto const loc=std::lower_bound(Base::begin(),Base::end(),in,comp);
				if(size()>=max_size())
				{
					if(loc!=Base::end())
					{
						Base::erase(Base::end()-1);
						Base::insert(loc,std::move(in));
					}
				}
				else
				{
					Base::insert(loc,std::move(in));
				}
			}
		}
	public:
		template<typename Compare=std::less<T>>
		void insert(T&&in,Compare comp={})
		{
			_insert(std::move(in),comp);
		}
		template<typename Compare=std::less<T>>
		void insert(T const& in,Compare comp={})
		{
			_insert(in,comp);
		}
	};
}
#undef EX_CONSTIF
#undef EX_CONSTEXPR
#endif