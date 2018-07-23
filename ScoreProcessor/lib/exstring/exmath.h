#ifndef EXMATH_H
#define EXMATH_H
#include <type_traits>
#include <vector>
#include <functional>
#include <algorithm>
#include <array>
#if defined(_CONSTEXPR17)
#define EX_CONSTEXPR _CONSTEXPR17
#elif defined(_HAS_CXX17) || __cplusplus>201100L
#define EX_CONSTEXPR constexpr
#else
#define EX_CONSTEXPR
#endif

#define CONSTEXPR
#ifdef _CONSTEXPR_IF
#define EX_CONSTIF _CONSTEXPR_IF
#elif defined(_HAS_CXX17) || __cplusplus>201700L
#define EX_CONSTIF constexpr
#else
#define EX_CONSTIF
#endif
namespace exlib {

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
	auto parse(char const* str,T& out,int base=10) -> decltype(std::enable_if<std::is_unsigned<T>::value,conv_res>::type{})
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
	auto parse(char const* str,T& out,int base=10) -> decltype(std::enable_if<std::is_signed<T>::value,conv_res>::type{})
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
		constexpr long long min=std::numeric_limits<T>::max();
		constexpr long long llmin=std::numeric_limits<unsigned long long>::max();
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

	template<typename T>
	EX_CONSTEXPR unsigned int num_digits(T num,unsigned int base=10)
	{
		static_assert(std::is_integral<typename T>::value,"Requires integral type");
		unsigned int num_digits=1;
		while((num/=base)!=0)
		{
			++num_digits;
		}
		return num_digits;
	}

	template<unsigned long long val,typename CharType=char>
	constexpr auto to_string_unsigned()
	{
		std::array<CharType,num_digits(val)+1> number{{}};
		auto v=val;
		number.back()='\0';
		auto it=number.end()-2;
		do
		{
			*it=v%10+'0';
			--it;
			v/=10;
		} while(v);
		return number;
	}

	template<long long val,typename CharType=char>
	constexpr auto to_string()
	{
		if constexpr(val<0)
		{
			std::array<CharType,num_digits(val)+2> number{{}};
			auto v=val;
			number.back()='\0';
			number.front()='-';
			auto it=number.end()-2;
			do
			{
				if constexpr(-1%10==-1)
				{
					*it=-(v%10)+'0';
				}
				else
				{
					*it=10-(v%10)+'0';
				}
				--it;
				v/=10;
			} while(v);
			return number;
		}
		else
		{
			return to_string_unsigned<val,CharType>();
		}
	}

	/*
	Makes a container in which each element becomes the minimum element within hp of its index
	*/
	template<typename RandomAccessContainer,typename Comp>
	RandomAccessContainer fattened_profile(RandomAccessContainer const& prof,size_t hp,Comp comp)
	{
		RandomAccessContainer res(prof.size());
		auto el=prof.begin()-1;
		auto rit=res.begin();
		for(auto it=prof.begin();it<prof.end();++it,++rit)
		{
			decltype(it) begin,end;
			if((it-prof.begin())<=hp)
			{
				begin=prof.begin();
			}
			else
			{
				begin=it-hp;
			}
			end=it+hp+1;
			if(end>prof.end())
			{
				end=prof.end();
			}
			if(el<begin)
			{
				el=std::min_element(begin,end,comp);
			}
			else
			{
				el=std::min(el,end-1,[=](auto const a,auto const b)
				{
					return comp(*a,*b);
				});
			}
			*rit=*el;
		}
		return res;
	}

	template<typename RandomAccessContainer>
	RandomAccessContainer fattened_profile(RandomAccessContainer const& prof,size_t hp)
	{
		typedef std::decay<decltype(*prof.begin())>::type T;
		if
#if __cplusplus > 201700L
			constexpr
#endif
			(std::is_trivially_copyable<T>::value&&sizeof(T)<=2*sizeof(size_t))
		{
			return fattened_profile(prof,hp,[](auto a,auto b)
			{
				return a<b;
			});
		}
		else
		{
			return fattened_profile(prof,hp,[](auto const& a,auto const& b)
			{
				return a<b;
			});
		}
	}

	template<typename T>
	class LimitedSet {
	public:
		typedef typename ::std::vector<typename T>::iterator iterator;
		typedef typename ::std::vector<typename T>::const_iterator const_iterator;
	private:
		size_t _max_size;
		::std::vector<T> _data;
	public:
		LimitedSet(size_t s):_data(),_max_size(s)
		{
			_data.reserve(s);
		}
		LimitedSet():LimitedSet(0)
		{}
		void max_size(size_t s)
		{
			_max_size=s;
		}
		size_t max_size() const
		{
			return _max_size;
		}
		iterator begin()
		{
			return _data.begin();
		}
		iterator end()
		{
			return _data.end();
		}
		const_iterator begin() const
		{
			return _data.begin();
		}
		const_iterator end() const
		{
			return _data.end();
		}
		size_t size() const
		{
			return _data.size();
		}
	private:
		template<typename U,typename Comp>
		void _insert(U&& in,Comp comp)
		{
			if(_data.empty()&&_max_size)
			{
				_data.insert(_data.begin(),std::forward<U>(in));
				return;
			}
			if(comp(in,_data.front()))
			{
				_data.insert(_data.begin(),std::forward<U>(in));
				goto end;
			}
			if(!(comp(in,_data.back())))
			{
				_data.insert(_data.end(),std::forward<U>(in));
				goto end;
			}
			auto b=_data.begin();
			auto e=_data.end();
			decltype(b) m;
			while(b<e)
			{
				m=b+std::distance(b,e)/2;
				if(comp(in,*m))
				{
					if(!(comp(in,*(m-1))))
					{
						_data.insert(m,std::forward<U>(in));
						goto end;
					}
					else
					{
						e=m;
					}
				}
				else
				{
					b=m+1;
				}
			}
		end:
			if(_data.size()>_max_size)
			{
				_data.erase(_data.end()-1);
			}
		}
	public:
		template<typename Compare>
		void insert(T&& in,Compare comp)
		{
			_insert(std::move(in),comp);
		}
		template<typename Compare>
		void insert(T const& in,Compare comp)
		{
			_insert(in,comp);
		}
		void insert(T&& in)
		{
			_insert(std::move(in),std::less<T>());
		}
		void insert(T const& in)
		{
			_insert(in,std::less<T>());
		}
	};
	/*
	Little-endian arbitrarily sized 2's complement integer.
	*/
	class BigInteger {
		std::vector<int64_t> data;
	public:
		struct div_t;

		BigInteger(int64_t);

		template<typename T>
		friend void operator<<(std::basic_ostream<T>,BigInteger const&);

		size_t size() const;
		size_t capacity() const;
		void reserve(size_t);

		bool operator==(BigInteger const&) const;
		bool operator<(BigInteger const&) const;
		bool operator>(BigInteger const&) const;

		BigInteger operator+(BigInteger const&) const;
		BigInteger& operator+=(BigInteger const&);
		BigInteger operator-(BigInteger const&) const;
		BigInteger& operator-=(BigInteger const&);
		BigInteger operator-() const;
		BigInteger& negate();
		BigInteger operator*(BigInteger const&) const;
		BigInteger& operator*=(BigInteger const&);
		BigInteger operator/(BigInteger const&) const;
		BigInteger& operator/=(BigInteger const&);
		BigInteger operator%(BigInteger const&) const;
		BigInteger& operator%=(BigInteger const&);
		static div_t div(BigInteger const&,BigInteger const&);

		BigInteger operator>>(size_t) const;
		BigInteger& operator>>=(size_t);
		BigInteger operator<<(size_t) const;
		BigInteger& operator<<=(size_t);
		BigInteger operator&(BigInteger const&) const;
		BigInteger& operator&=(BigInteger const&);
		BigInteger operator|(BigInteger const&) const;
		BigInteger& operator|=(BigInteger const&);
		BigInteger operator^(BigInteger const&) const;
		BigInteger& operator^=(BigInteger const&);
		BigInteger operator~() const;
		BigInteger& bitwise_negate();

		int signum() const;

		operator int64_t() const;
		operator double() const;
	};

	struct BigInteger::div_t {
		BigInteger quot;
		BigInteger rem;
	};
}

namespace std {
	template<>
	exlib::BigInteger const& max<exlib::BigInteger>(exlib::BigInteger const&,exlib::BigInteger const&);

	template<>
	struct is_integral<exlib::BigInteger>:public std::true_type {};

	template<>
	struct is_signed<exlib::BigInteger>:public std::true_type {};
}

namespace exlib {
	inline BigInteger::BigInteger(int64_t val):data(5)
	{
		data.back()|=val;
		data.back()&=0x8000'0000'0000'0000;
		data.front()=val&0x7FFF'FFFF'FFFF'FFFF;
	}
	inline size_t BigInteger::size() const
	{
		return data.size();
	}
	inline size_t BigInteger::capacity() const
	{
		return data.capacity();
	}
	inline void BigInteger::reserve(size_t n)
	{
		auto old_back_val=data.back();
		auto& old_back=data.back();
		data.reserve(n);
		old_back&=0x7FFF'FFFF'FFFF'FFFF;
		data.back()|=old_back_val;
		data.back()&=0x8000'0000'0000'0000;
	}
	inline bool BigInteger::operator==(BigInteger const& other) const
	{
		if(signum()!=other.signum())
		{
			return false;
		}
		size_t limit=std::min(other.size(),size());
	}
}
#undef EX_CONSTIF
#undef EX_CONSTEXPR
#endif