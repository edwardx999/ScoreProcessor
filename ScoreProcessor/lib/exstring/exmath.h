#ifndef EXMATH_H
#define EXMATH_H
#include <type_traits>
#include <vector>
#include <functional>
#include <algorithm>
namespace exlib {
	template<typename T>
	unsigned int num_digits(T n,T base=10);

	template<typename T>
	unsigned int num_digits(T num,T base)
	{
		static_assert(std::is_integral<typename T>::value,"Requires integral type");
		unsigned int num_digits=1;
		while((num/=base)!=0)
		{
			++num_digits;
		}
		return num_digits;
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
				el=std::min_element(begin,end);
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
		template<typename U>
		void _insert(U&& in,std::function<bool(T const&,T const&)> const& comp=std::less<T>())
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
			{
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
							break;
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
			}
		end:
			if(_data.size()>_max_size)
			{
				_data.erase(_data.end()-1);
			}
		}
	public:
		void insert(T&& in,std::function<bool(T const&,T const&)> const& comp=std::less<T>())
		{
			_insert(std::move(in),comp);
		}
		void insert(T const& in,std::function<bool(T const&,T const&)> const& comp=std::less<T>())
		{
			_insert(in,comp);
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
#endif