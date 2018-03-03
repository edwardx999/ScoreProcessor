#ifndef EXMATH_H
#define EXMATH_H
#include <type_traits>
namespace exlib {
	template<typename T,typename Out>
	Out num_digits(T n,T base=10);

	template<typename T,typename Out=std::enable_if<std::is_integral<T>::value,unsigned int>::type>
	Out num_digits(T num,T base)
	{
		Out num_digits=1;
		while((num/=base)!=0)
		{
			++num_digits;
		}
		return num_digits;
	}
}
#endif