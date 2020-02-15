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
#ifndef EXALG_H
#define EXALG_H
#include <utility>
#include <array>
#include <functional>
#include <stddef.h>
#ifdef _MSVC_LANG
#define _EXALG_HAS_CPP_20 (_MSVC_LANG>=202000L)
#define _EXALG_HAS_CPP_17 (_MSVC_LANG>=201700L)
#define _EXALG_HAS_CPP_14 (_MSVC_LANG>=201400L)
#else
#define _EXALG_HAS_CPP_20 (__cplusplus>=202000L)
#define _EXALG_HAS_CPP_17 (__cplusplus>=201700L)
#define _EXALG_HAS_CPP_14 (__cplusplus>=201400L)
#endif
#if _EXALG_HAS_CPP_17
#define _EXALG_NODISCARD [[nodiscard]]
#define _EXALG_CONSTEXPRIF constexpr
#else
#define _EXALG_NODISCARD
#define _EXALG_CONSTEXPRIF
#endif
#if _EXALG_HAS_CPP_14
#define _EXALG_SIMPLE_CONSTEXPR constexpr
#else
#define _EXALG_SIMPLE_CONSTEXPR
#endif
#include <exception>
#include <stddef.h>
#include <tuple>
#include <iterator>
#include "exretype.h"

#if _EXALG_HAS_CPP_17
namespace std {
	template<typename... Types>
	class variant;
}
#endif
namespace exlib {

	namespace fill_detail {
		template<typename Iter,typename Val>
		constexpr void fill_with_copy(Iter begin,Iter end,Val const& val)
		{
			for(;begin!=end;++begin)
			{
				*begin=val;
			}
		}
	}

	template<typename Iter,typename Val=typename std::iterator_traits<Iter>::value_type>
	constexpr auto fill(Iter begin,Iter end,Val&& val) noexcept(noexcept(*begin=val)&&noexcept(++begin)&&noexcept(begin==end)) -> typename std::enable_if<std::is_rvalue_reference<Val&&>::value&&!std::is_trivially_assignable<decltype(*begin),Val&&>::value>::type
	{
		if(begin==end) return;
		auto copy_begin=begin;
		*begin=std::move(val);
		++begin;
		fill_detail::fill_with_copy(begin,end,*copy_begin);
	}

	template<typename Iter,typename Val=typename std::iterator_traits<Iter>::value_type>
	constexpr void fill(Iter begin,Iter end,Val const& val) noexcept(noexcept(*begin=val)&&noexcept(++begin)&&noexcept(begin==end))
	{
		fill_detail::fill_with_copy(begin,end,val);
	}
	
	template<typename A,typename B,typename Compare=std::less<void>>
	_EXALG_NODISCARD constexpr typename max_cref<A,B>::type min(A&& a,B&& b,Compare c={}) noexcept(noexcept(c(a,b)))
	{
		if(c(a,b))
		{
			return std::forward<A>(a);
		}
		return std::forward<B>(b);
	}

	template<typename A,typename B,typename Compare=std::less<void>>
	_EXALG_NODISCARD constexpr typename max_cref<A,B>::type max(A&& a,B&& b,Compare c={}) noexcept (noexcept(c(a,b)))
	{
		if(c(a,b))
		{
			return std::forward<B>(b);
		}
		return std::forward<A>(a);
	}

	template<typename A,typename B,typename Compare=std::less<void>>
	_EXALG_NODISCARD constexpr std::pair<typename max_cref<A,B>::type,typename max_cref<A,B>::type> minmax(A&& a,B&& b,Compare c={}) noexcept(noexcept(c(a,b)))
	{
		if(c(a,b))
		{
			return {std::forward<A>(a),std::forward<B>(b)};
		}
		return {std::forward<B>(b),std::forward<A>(a)};
	}

	namespace min_detail {

		template<typename Out,typename A,typename B>
		_EXALG_NODISCARD constexpr Out multi_min(A&& a,B&& b) noexcept(noexcept(a<b))
		{
			return exlib::min(std::forward<A>(a),std::forward<B>(b));
		}

		template<typename Out,typename A,typename B>
		_EXALG_NODISCARD constexpr Out multi_max(A&& a,B&& b) noexcept(noexcept(a<b))
		{
			return exlib::max(std::forward<A>(a),std::forward<B>(b));
		}

		template<typename Out,typename A,typename B,typename... Rest>
		_EXALG_NODISCARD constexpr Out multi_min(A&& a,B&& b,Rest&&... r) noexcept(noexcept(a<b))
		{
			return min_detail::multi_min<Out>(exlib::min(std::forward<A>(a),std::forward<B>(b)),std::forward<Rest>(r)...);
		}

		template<typename Out,typename A,typename B,typename... Rest>
		_EXALG_NODISCARD constexpr Out multi_max(A&& a,B&& b,Rest&&... r) noexcept(noexcept(a<b))
		{
			return min_detail::multi_max<Out>(exlib::max(std::forward<A>(a),std::forward<B>(b)),std::forward<Rest>(r)...);
		}

		template<typename Out,typename Compare,typename A,typename B>
		_EXALG_NODISCARD constexpr Out cmulti_min(Compare c,A&& a,B&& b) noexcept(noexcept(c(a,b)))
		{
			return exlib::min(std::forward<A>(a),std::forward<B>(b),c);
		}

		template<typename Out,typename Compare,typename A,typename B>
		_EXALG_NODISCARD constexpr Out cmulti_max(Compare c,A&& a,B&& b) noexcept(noexcept(c(a,b)))
		{
			return exlib::max(std::forward<A>(a),std::forward<B>(b),c);
		}

		template<typename Out,typename Compare,typename A,typename B,typename... Rest>
		_EXALG_NODISCARD constexpr Out cmulti_min(Compare c,A&& a,B&& b,Rest&&... r) noexcept(noexcept(c(a,b)))
		{
			return min_detail::cmulti_min<Out>(c,exlib::min(std::forward<A>(a),std::forward<B>(b),c),std::forward<Rest>(r)...);
		}

		template<typename Out,typename Compare,typename A,typename B,typename... Rest>
		_EXALG_NODISCARD constexpr Out cmulti_max(Compare c,A&& a,B&& b,Rest&&... r) noexcept(noexcept(c(a,b)))
		{
			return min_detail::cmulti_max<Out>(c,exlib::max(std::forward<A>(a),std::forward<B>(b),c),std::forward<Rest>(r)...);
		}
	}

	template<typename A,typename B>
	_EXALG_NODISCARD constexpr typename max_cref<A,B>::type multi_min(A&& a,B&& b) noexcept(noexcept(a<b))
	{
		return exlib::min(std::forward<A>(a),std::forward<B>(b));
	}

	template<typename A,typename B>
	_EXALG_NODISCARD constexpr typename max_cref<A,B>::type multi_max(A&& a,B&& b) noexcept(noexcept(a<b))
	{
		return exlib::max(std::forward<A>(a),std::forward<B>(b));
	}

	template<typename A,typename B,typename C,typename... Rest>
	_EXALG_NODISCARD constexpr auto multi_min(A&& a,B&& b,C&& c,Rest&&... r) noexcept(noexcept(a<b)) -> typename max_cref<A,B,C,Rest...>::type
	{
		return min_detail::multi_min<typename max_cref<A,B,C,Rest...>::type>(std::forward<A>(a),std::forward<B>(b),std::forward<C>(c),std::forward<Rest>(r)...);
	}

	template<typename A,typename B,typename C,typename... Rest>
	_EXALG_NODISCARD constexpr auto multi_max(A&& a,B&& b,C&& c,Rest&&... r) noexcept(noexcept(a<b)) -> typename max_cref<A,B,C,Rest...>::type
	{
		return min_detail::multi_max<typename max_cref<A,B,C,Rest...>::type>(std::forward<A>(a),std::forward<B>(b),std::forward<C>(c),std::forward<Rest>(r)...);
	}

	template<typename Compare,typename A,typename B,typename... Rest>
	_EXALG_NODISCARD constexpr auto multi_min(Compare c,A&& a,B&& b,Rest&&... r) noexcept(noexcept(c(a,b))) -> typename std::enable_if<is_predicate<Compare,A,B>::value,typename max_cref<A,B,Rest...>::type>::type
	{
		return min_detail::cmulti_min<typename max_cref<A,B,Rest...>::type>(c,std::forward<A>(a),std::forward<B>(b),std::forward<Rest>(r)...);
	}

	template<typename Compare,typename A,typename B,typename... Rest>
	_EXALG_NODISCARD constexpr auto multi_max(Compare c,A&& a,B&& b,Rest&&... r) noexcept(noexcept(c(a,b))) -> typename std::enable_if<is_predicate<Compare,A,B>::value,typename max_cref<A,B,Rest...>::type>::type
	{
		return min_detail::cmulti_max<typename max_cref<A,B,Rest...>::type>(c,std::forward<A>(a),std::forward<B>(b),std::forward<Rest>(r)...);
	}

	template<typename Iter,typename Transform,typename Compare=std::less<void>>
	_EXALG_NODISCARD constexpr Iter min_keyed_element(Iter begin,Iter end,Transform transform,Compare c={}) noexcept(noexcept(c(transform(*begin),transform(*begin))))
	{
		if(begin==end)
		{
			return end;
		}
		auto min_val=transform(*begin);
		auto min_iter=begin;
		++begin;
		for(;begin!=end;++begin)
		{
			auto value=transform(*begin);
			if(c(value,min_val))
			{
				min_iter=begin;
				min_val=std::move(value);
			}
		}
		return min_iter;
	}

	template<typename Iter,typename Transform,typename Compare=std::less<void>>
	_EXALG_NODISCARD constexpr Iter max_keyed_element(Iter begin,Iter end,Transform transform,Compare c={}) noexcept(noexcept(c(transform(*begin),transform(*begin))))
	{
		using type=decltype(transform(*begin));
		struct inverter {
			Compare c;
			constexpr bool operator()(type const& a,type const& b) const noexcept(noexcept(c(a,b)))
			{
				return !(c(a,b));
			}
		};
		return min_keyed_element(begin,end,std::move(transform),inverter{std::move(c)});
	}

	template<typename Iter,typename Transform,typename Compare=std::less<void>>
	_EXALG_NODISCARD constexpr std::pair<Iter,Iter> minmax_keyed_element(Iter begin,Iter end,Transform transform,Compare c={}) noexcept(noexcept(c(transform(*begin),transform(*begin))))
	{
		if(begin==end)
		{
			return end;
		}
		auto min_val=transform(*begin);
		auto max_val=min_val;
		auto min_iter=begin;
		auto max_iter=begin;
		++begin;
		for(;begin!=end;++begin)
		{
			auto value=transform(*begin);
			if _EXALG_CONSTEXPRIF(std::is_trivially_copyable<decltype(value)>::value)
			{
				if(c(value,min_val))
				{
					min_iter=begin;
					min_val=value;
				}
				if(!c(value,max_val))
				{
					max_iter=begin;
					max_val=value;
				}
			}
			else
			{
				if(c(value,min_val))
				{
					min_iter=begin;
					if(!c(value,max_val))
					{
						max_iter=begin;
						max_val=value;
					}
					min_val=std::move(value);
				}
				else if(!c(value,max_val))
				{
					max_iter=begin;
					max_val=std::move(value);
				}
			}
		}
		return {min_iter,max_iter};
	}
}


namespace get_impl {
#if !(_EXALG_HAS_CPP_20)
	using std::get;
#endif
	template<std::size_t I,typename Container>
	constexpr auto do_get(Container&& container) noexcept(noexcept(get<I>(container))) -> decltype(exlib::forward_like<Container&&>(get<I>(container)))
	{
		return exlib::forward_like<Container&&>(get<I>(container));
	}
	template<std::size_t I,typename Container,typename... Extra>
	constexpr auto do_get(Container&& container,Extra...) noexcept(noexcept(container[I])) -> decltype(exlib::forward_like<Container&&>(container[I]))
	{
		return exlib::forward_like<Container&&>(container[I]);
	}
	template<std::size_t I,typename T,std::size_t N>
	constexpr T&& do_get(T(&& arr)[N]) noexcept
	{
		return std::move(arr[I]);
	}
}

namespace exlib {

	template<std::size_t I,typename Container,typename... Extra>
	constexpr auto generic_get(Container&& cont,Extra...) noexcept(noexcept(get_impl::do_get<I>(std::forward<Container>(cont)))) -> decltype(get_impl::do_get<I>(std::forward<Container>(cont)))
	{
		return get_impl::do_get<I>(std::forward<Container>(cont));
	}

#define _EXALG_MEM_FN_BODY (std::forward<T>(obj).*mem_fn)(std::forward<Args>(args)...)
	/*
		Call the member function as if it is a global function.
	*/
	template<typename T,typename MemFn,typename... Args>
	constexpr auto apply_mem_fn(T&& obj,MemFn mem_fn,Args&&... args) noexcept(noexcept(_EXALG_MEM_FN_BODY)) -> decltype(_EXALG_MEM_FN_BODY)
	{
		return _EXALG_MEM_FN_BODY;
	}
	/*
		Call the member function as if it is a global function.
	*/
	template<typename MemFn,MemFn mem_fn,typename T,typename... Args>
	constexpr auto apply_mem_fn(T&& obj,Args&& ... args) noexcept(noexcept(_EXALG_MEM_FN_BODY)) -> decltype(_EXALG_MEM_FN_BODY)
	{
		return _EXALG_MEM_FN_BODY;
	}
#if	_EXALG_HAS_CPP_17
	/*
		Version that has the member function constexpr bound to it,
		MemFn is a member function pointer of T
	*/
	template<auto mem_fn,typename T,typename... Args>
	constexpr auto apply_mem_fn(T&& obj,Args&&... args) noexcept(noexcept(_EXALG_MEM_FN_BODY)) -> decltype(_EXALG_MEM_FN_BODY)
	{
		return _EXALG_MEM_FN_BODY;
	}
#endif

#define _EXALG_MEM_FN_BODY2(quals) (static_cast<T quals>(obj).*mem_fn)(std::forward<Args>(args)...)

#define _EXALG_DEFINE_APPLY_MEM_FN(name_suffix,ref_quals,func_quals)\
	template<typename T,typename Ret,typename B,typename... Args>\
	constexpr auto apply_mem_fn ## _ ## name_suffix(T ref_quals obj,Ret(B::* mem_fn)(Args...) func_quals,Args&&... args)  noexcept(noexcept(_EXALG_MEM_FN_BODY2(ref_quals))) -> decltype(_EXALG_MEM_FN_BODY2(ref_quals)) \
	{\
		return _EXALG_MEM_FN_BODY2(ref_quals);\
	}\

	_EXALG_DEFINE_APPLY_MEM_FN(mutable,&,)

	_EXALG_DEFINE_APPLY_MEM_FN(const,const&,const)
#if _EXALG_HAS_CPP_20
	_EXALG_DEFINE_APPLY_MEM_FN(const,const&,const&)
#endif

	_EXALG_DEFINE_APPLY_MEM_FN(volatile,volatile&,volatile)

	_EXALG_DEFINE_APPLY_MEM_FN(const_volatile,const volatile&,const volatile)

#if __cpp_noexcept_function_type
	_EXALG_DEFINE_APPLY_MEM_FN(mutable,&,noexcept)
	_EXALG_DEFINE_APPLY_MEM_FN(const,const&,const noexcept)
	_EXALG_DEFINE_APPLY_MEM_FN(volatile,volatile&,volatile noexcept)
	_EXALG_DEFINE_APPLY_MEM_FN(const_volatile,const volatile&,const volatile noexcept)
#endif

#if _EXALG_ENABLE_REF_QUALIFIED_FUNCTION_PTRS
	_EXALG_DEFINE_APPLY_MEM_FN(mutable,&,&)
	_EXALG_DEFINE_APPLY_MEM_FN(volatile,volatile&,volatile&)
	_EXALG_DEFINE_APPLY_MEM_FN(const_volatile,const volatile&,const volatile&)
		
	_EXALG_DEFINE_APPLY_MEM_FN(rvalue,&&,&&)
	_EXALG_DEFINE_APPLY_MEM_FN(const_rvalue,const&&,const&&)
	_EXALG_DEFINE_APPLY_MEM_FN(volatile_rvalue,volatile&&,volatile&&)
	_EXALG_DEFINE_APPLY_MEM_FN(const_volatile_rvalue,const volatile&&,const volatile&&)
#endif

#undef _EXALG_DEFINE_APPLY_MEM_FN
#undef _EXALG_MEM_FN_BODY2
#undef _EXALG_MEM_FN_BODY
}
namespace exlib_swap_detail {

	template<typename A,typename B,typename... Extra>
	constexpr auto fallback_swap(A& a,B& b,Extra...) ->
		typename std::enable_if<
			std::is_move_constructible<B>::value&&
			std::is_assignable<A&,B&&>::value&&
			std::is_assignable<B&,A&&>::value>::type
	{
		B temp{std::move(b)};
		b=std::move(a);
		a=std::move(temp);
	}

	template<typename A,typename B>
	constexpr auto fallback_swap(A& a,B& b) ->
		typename std::enable_if<
			std::is_move_constructible<A>::value&&
			std::is_assignable<A&,B&&>::value&&
			std::is_assignable<B&,A&&>::value>::type
	{
		A temp{std::move(a)};
		a=std::move(b);
		b=std::move(temp);
	}

	template<typename A,typename B,typename... Extra>
	constexpr auto adl_swap2(A& a,B& b,Extra...) -> decltype(exlib_swap_detail::fallback_swap(a,b))
	{
		exlib_swap_detail::fallback_swap(a,b);
	}

	template<typename A,typename B>
	constexpr auto adl_swap2(A& a,B& b) -> decltype(swap(b,a),void())
	{
		swap(b,a);
	}

	template<typename A,typename B,typename... Extra>
	constexpr auto adl_swap1(A& a,B& b,Extra...) -> decltype(exlib_swap_detail::adl_swap2(a,b))
	{
		exlib_swap_detail::adl_swap2(a,b);
	}

	template<typename A,typename B>
	constexpr auto adl_swap1(A& a,B& b) -> decltype(swap(a,b),void())
	{
		swap(a,b);
	}

	template<typename A,typename B,typename... Extra>
	constexpr auto member_swap2(A& a,B& b,Extra...) -> decltype(exlib_swap_detail::adl_swap1(a,b))
	{
		exlib_swap_detail::adl_swap1(a,b);
	}

	template<typename A,typename B>
	constexpr auto member_swap2(A& a,B& b) -> decltype(b.swap(a),void())
	{
		b.swap(a);
	}

	template<typename A,typename B,typename... Extra>
	constexpr auto member_swap1(A& a,B& b,Extra...) -> decltype(exlib_swap_detail::member_swap2(a,b))
	{
		exlib_swap_detail::member_swap2(a,b);
	}

	template<typename A,typename B>
	constexpr auto member_swap1(A& a,B& b) -> decltype(a.swap(b),void())
	{
		a.swap(b);
	}
}
namespace exlib {

	/*
		Swaps by using the swaps (in priority order), SFINAE-friendly
			member swaps
				a.swap(b)
				b.swap(a)
			adl swaps
				swap(a,b)
				swap(b,a)
			fallback swaps
				swap using temp of type A
				swap using temp of type B
	*/
	template<typename A,typename B>
	constexpr auto adl_swap(A& a,B& b) noexcept -> decltype(exlib_swap_detail::member_swap1(a,b))
	{
		exlib_swap_detail::member_swap1(a,b);
	}

	template<typename A,typename B>
	constexpr auto simple_swap(A& a,B& b) noexcept -> decltype(exlib_swap_detail::fallback_swap(a,b))
	{
		exlib_swap_detail::fallback_swap(a,b);
	}

	template<typename T=void>
	struct less;

	template<typename T>
	struct less {
		constexpr bool operator()(T const& a,T const& b) const
		{
			return a<b;
		}
	};


	namespace detail {
		struct for_each_in_tuple_h {
			template<typename Tpl,typename Func>
			constexpr static void apply(Tpl&& tpl,Func&& f,index_sequence<>)
			{

			}
			template<typename Tpl,typename Func,std::size_t I,std::size_t... Rest>
			constexpr static void apply(Tpl&& tpl,Func& f,index_sequence<I,Rest...>)
			{
				f(generic_get<I>(std::forward<Tpl>(tpl)));
				for_each_in_tuple_h::apply(std::forward<Tpl>(tpl),f,index_sequence<Rest...>{});
			}
		};
	}

	template<typename Tpl,typename Func>
	constexpr void for_each_in_tuple(Tpl&& tpl,Func&& f)
	{
		detail::for_each_in_tuple_h::apply(std::forward<Tpl>(tpl),f,make_index_sequence<std::tuple_size<typename std::remove_reference<Tpl>::type>::value>{});
	}

	template<>
	struct less<char const*> {
		_EXALG_SIMPLE_CONSTEXPR bool operator()(char const* a,char const* b) const
		{
			for(std::size_t i=0;;++i)
			{
				if(a[i]<b[i])
				{
					return true;
				}
				if(b[i]<a[i])
				{
					return false;
				}
				if(a[i]==0)
				{
					return false;
				}
			}
		}
	};

	template<>
	struct less<char*>:public less<char const*> {};

	template<>
	struct less<void>:public less<char const*> {
		using less<char const*>::operator();
		template<typename A,typename B>
		constexpr auto operator()(A const& a,B const& b) const -> typename std::enable_if<!std::is_convertible<A,char const*>::value||!std::is_convertible<B,char const*>::value,bool>::type
		{
			return a<b;
		}
	};

	//comp is two-way "less-than" operator
	template<typename TwoWayIter,typename Comp>
	constexpr void qsort(TwoWayIter begin,TwoWayIter end,Comp comp)
	{
		if(begin!=end)
		{
			auto pivot=begin;
			auto it=begin;
			++it;
			for(;it!=end;++it)
			{
				if(comp(*it,*begin))
				{
					exlib::adl_swap(*it,*(++pivot));
				}
			}
			exlib::adl_swap(*begin,*pivot);
			qsort(begin,pivot,comp);
			qsort(pivot+1,end,comp);
		}
	}

	//sort by exlib::less
	template<typename iter>
	constexpr void qsort(iter begin,iter end)
	{
		using T=typename std::decay<decltype(*begin)>::type;
		qsort(begin,end,less<T>());
	}

	namespace detail {
		template<typename Iter,typename Comp>
		constexpr void sift_down(Iter begin,std::size_t parent,std::size_t dist,Comp c)
		{
			while(true)
			{
				std::size_t const child1=2*parent+1;
				if(child1<dist)
				{
					std::size_t const child2=child1+1;
					if(child2<dist)
					{
						std::size_t const max=c(begin[child1],begin[child2])?child2:child1;
						if(c(begin[parent],begin[max]))
						{
							swap(begin[parent],begin[max]);
							parent=max;
						}
						else
						{
							break;
						}
					}
					else
					{
						if(c(begin[parent],begin[child1]))
						{
							swap(begin[parent],begin[child1]);
							parent=child1;
						}
						else
						{
							break;
						}
					}
				}
				else
				{
					break;
				}
			}
		}

		template<typename Iter,typename Comp>
		constexpr void make_heap(Iter begin,std::size_t dist,Comp c)
		{
			for(std::size_t i=dist/2;i>0;)
			{
				--i;
				detail::sift_down(begin,i,dist,c);
			}
		}
	}

	template<typename Iter,typename Comp=std::less<typename std::iterator_traits<Iter>::value_type>>
	constexpr void make_heap(Iter begin,Iter end,Comp c={})
	{
		std::size_t const dist=end-begin;
		detail::make_heap(begin,dist,c);
	}

	template<typename Iter,typename Comp=std::less<typename std::iterator_traits<Iter>::value_type>>
	constexpr void pop_heap(Iter begin,Iter end,Comp c={})
	{
		std::size_t const dist=end-begin;
		if(dist==0)
		{
			return;
		}
		swap(*begin,*(end-1));
		detail::sift_down(begin,0,dist-1,c);
	}

	/*
		heapsort
		sorts between [begin,end) heapsort using c as a less-than comparison function
		@param begin random access iter pointing to beginning of range to sort
		@param end random access iter pointing 1 beyond valid range to sort
		@param c comparison function accepting type pointed to by Iter
	*/
	template<typename Iter,typename Comp=std::less<typename std::iterator_traits<Iter>::value_type>>
	constexpr void heapsort(Iter begin,Iter end,Comp c={})
	{
		std::size_t const n=end-begin;
		if(n<2)
		{
			return;
		}
		detail::make_heap(begin,n,c);
		for(std::size_t i=n;i>0;)
		{
			--i;
			//put max at end
			swap(begin[i],begin[0]);
			//sift new top down
			detail::sift_down(begin,0,i,c);
		}
	}

	//inserts the element AT elem into the range [begin,elem] according to comp assuming the range is sorted
	template<typename TwoWayIter,typename Comp>
	constexpr TwoWayIter insert_back(TwoWayIter const begin,TwoWayIter elem,Comp comp)
	{
		while(elem!=begin)
		{
			auto before=std::prev(elem);
			if(comp(*before,*elem))
			{
				return elem;
			}
			else
			{
				exlib::adl_swap(*before,*elem);
				elem=before;
			}
		}
		return elem;
	}

	template<typename TwoWayIter>
	constexpr TwoWayIter insert_back(TwoWayIter const begin,TwoWayIter elem)
	{
		using T=typename std::decay<decltype(*begin)>::type;
		return insert_back(begin,elem,less<T>());
	}

	//comp is two-way "less-than" operator
	template<typename TwoWayIter,typename Comp>
	constexpr void isort(TwoWayIter const begin,TwoWayIter end,Comp comp)
	{
		if(begin==end) return;
		auto i=begin;
		++i;
		for(;i!=end;++i)
		{
			insert_back(begin,i,comp);
		}
	}

	//sort by exlib::less
	template<typename TwoWayIter>
	constexpr void isort(TwoWayIter begin,TwoWayIter end)
	{
		using T=typename std::decay<decltype(*begin)>::type;
		isort(begin,end,less<T>());
	}

	//comp is two-way "less-than" operator
	template<typename T,std::size_t N,typename Comp>
	constexpr std::array<T,N> sorted(std::array<T,N> const& arr,Comp c)
	{
		auto sorted=arr;
		if _EXALG_CONSTEXPRIF(N<10) isort(sorted.begin(),sorted.end(),c);
		else qsort(sorted.begin(),sorted.end(),c);
		return sorted;
	}

	template<typename T,std::size_t N>
	constexpr std::array<T,N> sorted(std::array<T,N> const& arr)
	{
		return sorted(arr,less<T>());
	}

	namespace detail {
		template<typename B,typename... R>
		struct ma_ret {
			using type=B;
		};

		template<typename... R>
		struct ma_ret<void,R...> {
			using type=typename std::common_type<R...>::type;
		};

		template<typename Type,typename... Arrays>
		struct concat_value_type:ma_ret<Type,array_type_t<Arrays>...> {};

		template<typename Type,typename... Arrays>
		struct concat_type:
			std::enable_if<
			exlib::conjunction<exlib::is_sized_array<Arrays>...>::value,
			std::array<
			typename concat_value_type<Type,Arrays...>::type,
			exlib::sum_type_value<std::size_t,array_size<Arrays>...>::value>> {};

		template<typename Array>
		struct string_array_size:
			std::conditional<
			array_size<Array>::value==0,
			std::integral_constant<std::size_t,0>,
			std::integral_constant<std::size_t,array_size<Array>::value-1>>::type{};

		template<typename Type,typename... Arrays>
		struct str_concat_type:
			std::enable_if<
			conjunction<is_sized_array<Arrays>...>::value,
			std::array<
			typename concat_value_type<Type,Arrays...>::type,
			sum_type_value<std::size_t,string_array_size<Arrays>...>::value+1>> {};

		template<typename Ret,typename Arr1,size_t... I>
		constexpr Ret fill_copy(Arr1 const& a,index_sequence<I...>)
		{
			Ret ret{{a[I]...}};
			return ret;
		}

		template<typename Ret,typename Arr1,typename Arr2,size_t... I,size_t... J>
		constexpr Ret concat(Arr1 const& a,Arr2 const& b,index_sequence<I...>,index_sequence<J...>)
		{
			Ret ret{{a[I]...,b[J]...}};
			return ret;
		}

		template<typename Ret,typename A,typename B>
		constexpr Ret str_concat(A const& a,B const& b)
		{
			constexpr size_t N=array_size<A>::value;
			constexpr size_t M=array_size<B>::value;
			constexpr size_t Nf=N==0?0:N-1;
			return concat<Ret>(a,b,make_index_sequence<Nf>(),make_index_sequence<M>());
		}
	}

	template<typename ValueType>
	constexpr std::array<ValueType,0> concat()
	{
		return std::array<ValueType,0>{};
	}

	template<typename ValueType=void,typename A>
	constexpr auto concat(A const& a) -> typename detail::concat_type<ValueType,A>::type
	{
		return detail::fill_copy<typename detail::concat_type<ValueType,A>::type>(a,make_index_sequence<array_size<A>::value>{});
	}

	//concatenate arrays (std::array<T,N> or T[N]) and returns an std::array<T,CombinedLen> of the two
	template<typename ValueType=void,typename A,typename B>
	constexpr auto concat(A const& a,B const& b) -> typename detail::concat_type<ValueType,A,B>::type
	{
		constexpr size_t N=array_size<A>::value;
		constexpr size_t M=array_size<B>::value;
		return detail::concat<typename detail::concat_type<ValueType,A,B>::type>(a,b,make_index_sequence<N>(),make_index_sequence<M>());
	}

	//concatenate arrays (std::array<T,N> or T[N]) and returns an std::array<T,CombinedLen> of the arrays
	template<typename ValueType=void,typename A,typename B,typename... C>
	constexpr auto concat(A const& a,B const& b,C const& ... c)  -> typename detail::concat_type<ValueType,A,B,C...>::type
	{
		return concat<ValueType>(concat<ValueType>(a,b),c...);
	}

	template<typename ValueType>
	constexpr std::array<ValueType,1> str_concat()
	{
		return {{0}};
	}

	template<typename ValueType=void,typename A>
	constexpr auto str_concat(A const& a) -> decltype(concat<ValueType>(a))
	{
		return concat<ValueType>(a);
	}

	//concatenate str arrays (std::array<T,N> or T[N]) and returns an std::array<T,CombinedLen> of the arrays
	//(just takes off null-terminator of arrays)
	template<typename ValueType=void,typename A,typename C>
	constexpr auto str_concat(A const& a,C const& c) -> typename detail::str_concat_type<ValueType,A,C>::type
	{
		return detail::str_concat<typename detail::str_concat_type<ValueType,A,C>::type>(a,c);
	}

	//concatenate str arrays (std::array<T,N> or T[N]) and returns an std::array<T,CombinedLen> of the arrays
	//(just takes off null-terminator of arrays)
	template<typename ValueType=void,typename A,typename B,typename... C>
	constexpr auto str_concat(A const& a,B const& b,C const& ... c) -> typename detail::str_concat_type<ValueType,A,B,C...>::type
	{
		return str_concat<ValueType>(str_concat<ValueType>(a,b),c...);
	}

	namespace detail {
		template<typename Container,typename SizeT>
		auto reserve_if_able(Container& a,SizeT s) -> decltype(a.reserve(s))
		{
			return a.reserve(s);
		}
		template<typename Container,typename SizeT,typename... Extra>
		void reserve_if_able(Container& a,SizeT s,Extra...)
		{}
	}

#ifndef __cpp_fold_expressions

	namespace detail {
		template<typename Container>
		void container_concat_help(Container& c)
		{}
		template<typename Container,typename First,typename... Rest>
		void container_concat_help(Container& c,First const& f,Rest const&... rest)
		{
			c.insert(c.end(),f.begin(),f.end());
			container_concat_help(c,rest...);
		}
		template<typename Container>
		std::size_t container_total_size(Container const& a)
		{
			return a.size();
		}
		template<typename Container,typename... Rest>
		std::size_t container_total_size(Container const& a,Rest const&... r)
		{
			return a.size()+container_total_size(r...);
		}
	}
	template<typename Container,typename... Rest>
	Container container_concat(Container&& c,Rest const&... rest)
	{
		detail::reserve_if_able(c,detail::container_total_size(c,rest...));
		detail::container_concat_help(c,rest...);
		return std::move(c);
	}
	template<typename Container,typename... Rest>
	Container container_concat(Container const& c,Rest const&... rest)
	{
		Container copy;
		detail::reserve_if_able(copy,detail::container_total_size(c,rest...));
		detail::container_concat_help(copy,c,rest...);
		return c;
	}
#else
	template<typename Container>
	Container container_concat(Container&& cont)
	{
		return std::move(cont);
	}

	template<typename Container>
	Container container_concat(Container const& cont)
	{
		return cont;
	}

	template<typename Container,typename... Rest>
	Container container_concat(Container&& cont,Rest const&... rest)
	{
		detail::reserve_if_able(cont,(cont.size()+...+rest.size()));
		(cont.insert(cont.end(),rest.begin(),rest.end()),...);
		return std::move(cont);
	}

	template<typename Container,typename... Rest>
	Container container_concat(Container const& cont,Rest const&... rest)
	{
		Container copy;
		detail::reserve_if_able(copy,(cont.size()+...+rest.size()));
		copy.insert(cont.begin(),cont.end());
		(copy.insert(copy.end(),rest.begin(),rest.end()),...);
		return copy;
	}
#endif

	template<typename T=void>
	struct compare {
		constexpr int operator()(T const& a,T const& b) const
		{
			if(a<b)
			{
				return -1;
			}
			if(a==b)
			{
				return 0;
			}
			return 1;
		}
	};

	template<>
	struct compare<char const*> {
		_EXALG_SIMPLE_CONSTEXPR int operator()(char const* a,char const* b) const
		{
			for(std::size_t i=0;;++i)
			{
				if(a[i]<b[i])
				{
					return -1;
				}
				if(b[i]<a[i])
				{
					return 1;
				}
				if(a[i]==0)
				{
					return 0;
				}
			}
		}
	};

	template<>
	struct compare<char*>:public compare<char const*> {};

	template<>
	struct compare<void>:public compare<char const*> {
		using compare<char const*>::operator();
		template<typename A,typename B>
		constexpr auto operator()(A const& a,B const& b) const -> typename std::enable_if<!std::is_convertible<A,char const*>::value||!std::is_convertible<B,char const*>::value,bool>::type
		{
			if(a<b) return -1;
			if(a==b) return 0;
			return 1;
		}
	};

	/*
		Reverses given range.
	*/
	template<typename TwoWayIter>
	constexpr void reverse(TwoWayIter begin,TwoWayIter end)
	{
		while((begin!=end)&&(begin!=--end))
		{
			exlib::adl_swap(*begin,*end);
			++begin;
		}
	}

	/*
		Rotates range such that what was once at middle will be at begin.
	*/
	template<typename TwoWayIter>
	constexpr void rotate(TwoWayIter begin,TwoWayIter middle,TwoWayIter end)
	{
		exlib::reverse(begin,middle);
		exlib::reverse(middle,end);
		exlib::reverse(begin,end);
	}

	/*
		Rotates range such that what was once at middle will be at begin.
	*/
	template<typename TwoWayIter>
	constexpr void rotate_left(TwoWayIter begin,TwoWayIter middle,TwoWayIter end)
	{
		exlib::rotate(begin,middle,end);
	}

	/*
		Rotates range such that what was once at begin will be at middle.
	*/
	template<typename TwoWayIter>
	constexpr void rotate_right(TwoWayIter begin,TwoWayIter middle,TwoWayIter end)
	{
		exlib::reverse(begin,end);
		exlib::reverse(begin,middle);
		exlib::reverse(middle,end);
	}

	//returns the iterator it for which c(target,*it)==0
	//if this is not found, end is returned
	template<typename it,typename T,typename ThreeWayComp>
	constexpr it binary_find(it begin,it end,T const& target,ThreeWayComp c)
	{
		auto old_end=end;
		while(true)
		{
			it i=(end-begin)/2+begin;
			if(i>=end)
			{
				return old_end;
			}
			auto const res=c(target,*i);
			if(res==0)
			{
				return i;
			}
			if(res<0)
			{
				end=i;
			}
			else
			{
				begin=i+1;
			}
		}
	}

	template<typename it,typename T>
	constexpr it binary_find(it begin,it end,T const& target)
	{
		return binary_find(begin,end,target,compare<typename std::decay<decltype(*begin)>::type>());
	}

	//converts three way comparison into a less than comparison
	template<typename ThreeWayComp=compare<void>>
	struct lt_comp:private empty_store<ThreeWayComp> {
		template<typename A,typename B>
		constexpr bool operator()(A const& a,B const& b) const
		{
			return this->get()(a,b)<0;
		}
	};

	//converts three way comparison into a greater than comparison
	template<typename ThreeWayComp=compare<void>>
	struct gt_comp:private empty_store<ThreeWayComp> {
		template<typename A,typename B>
		constexpr bool operator()(A const& a,B const& b) const
		{
			return this->get()(a,b)>0;
		}
	};

	//converts three way comparison into a less than or equal to comparison
	template<typename ThreeWayComp=compare<void>>
	struct le_comp:private empty_store<ThreeWayComp> {
		template<typename A,typename B>
		constexpr bool operator()(A const& a,B const& b) const
		{
			return this->get()(a,b)<=0;
		}
	};

	//converts three way comparison into a greater than or equal to comparison
	template<typename ThreeWayComp=compare<void>>
	struct ge_comp:private empty_store<ThreeWayComp> {
		template<typename A,typename B>
		constexpr bool operator()(A const& a,B const& b) const
		{
			return this->get()(a,b)>=0;
		}
	};

	//converts three way comparison into equality comparison
	template<typename ThreeWayComp=compare<void>>
	struct eq_comp:private empty_store<ThreeWayComp> {
		template<typename A,typename B>
		constexpr bool operator()(A const& a,B const& b) const
		{
			return this->get()(a,b)==0;
		}
	};

	//converts three way comparison into inequality comparison
	template<typename ThreeWayComp=compare<void>>
	struct ne_comp:private empty_store<ThreeWayComp> {
		template<typename A,typename B>
		constexpr bool operator()(A const& a,B const& b) const
		{
			return this->get()(a,b)!=0;
		}
	};

	//inverts comparison
	template<typename Comp=compare<void>>
	struct inv_comp:private empty_store<Comp> {
		template<typename A,typename B>
		constexpr auto operator()(A const& a,B const& b) const -> decltype(this->get()(b,a))
		{
			return this->get()(b,a);
		}
	};

	template<typename T=void>
	using greater=inv_comp<less<T>>;

#if _EXALG_HAS_CPP_17
	//use this to initialize ct_map
	template<typename Key,typename Value>
	struct map_pair {
	private:
		Key _key;
		Value _value;
	public:
		using key_type=Key;
		using mapped_type=Value;
		template<typename A,typename B>
		constexpr map_pair(A&& a,B&& b):_key(std::forward<A>(a)),_value(std::forward<B>(b))
		{}
		constexpr Key const& key() const
		{
			return _key;
		}
		constexpr Value const& value() const
		{
			return _value;
		}
		constexpr Value& value()
		{
			return _value;
		}
	};

	namespace detail {
		template<typename Comp,typename Key,typename Value>
		struct map_compare:protected empty_store<Comp> {
			template<typename Conv>
			constexpr int operator()(Conv const& target,map_pair<Key,Value> const& b) const
			{
				return this->get()(target,b.key());
			}
			constexpr int operator()(map_pair<Key,Value> const& a,map_pair<Key,Value> const& b) const
			{
				return this->get()(a.key(),b.key());
			}
		};
	}

	//Comp defines operator()(Key(&),Key(&)) that is a three-way comparison
	template<typename Key,typename Value,std::size_t entries,typename Comp=compare<Key>>
	class ct_map:protected detail::map_compare<Comp,Key,Value>,protected std::array<map_pair<Key,Value>,entries> {
	public:
		using key_type=Key;
		using mapped_type=Value;
		using value_type=map_pair<Key,Value>;
	protected:
		using Data=std::array<value_type,entries>;
	public:
		using size_type=size_t;
		using difference_type=std::ptrdiff_t;
		using key_compare=detail::map_compare<Comp,Key,Value>;
		using typename Data::reference;
		using typename Data::const_reference;
		using typename Data::iterator;
		using typename Data::const_iterator;
		using typename Data::reverse_iterator;
		using typename Data::const_reverse_iterator;
	private:
		using pair=value_type;
	public:

		using Data::size;
		using Data::max_size;
		using Data::empty;
		using Data::operator[];

		using Data::begin;
		using Data::end;
		using Data::cbegin;
		using Data::cend;
		using Data::rbegin;
		using Data::rend;
		using Data::crbegin;
		using Data::crend;
		using Data::at;
		using Data::back;
		using Data::front;
		using Data::data;

		template<typename... Args>
		constexpr ct_map(Args&& ... rest):Data{{std::forward<Args>(rest)...}}
		{
			static_assert(sizeof...(Args)==entries,"Wrong number of entries");
			qsort(data(),data()+size(),lt_comp<key_compare>());
		}

	private:
		template<std::size_t... Is>
		constexpr ct_map(std::array<value_type,entries> const& in,index_sequence<Is...>):Data{{in[Is]...}}
		{}
	public:
		constexpr ct_map(std::array<value_type,entries> const& in):ct_map(in,make_index_sequence<entries>())
		{}
		template<typename T>
		constexpr iterator find(T const& k)
		{
			return iterator{binary_find(data(),data()+size(),k,static_cast<key_compare const&>(*this))};
		}
		template<typename T>
		constexpr const_iterator find(T const& k) const
		{
			return const_iterator{binary_find(data(),data()+size(),k,static_cast<key_compare const&>(*this))};
		}
	};

	//inputs should be of type map_pair<Key,Value>
	template<typename Comp,typename First,typename... Rest>
	constexpr auto make_ct_map(First&& f,Rest&& ... r)
	{
		using Decayed=typename std::decay<First>::type;
		return ct_map<typename Decayed::key_type,typename Decayed::mapped_type,1+sizeof...(r),Comp>(std::forward<First>(f),std::forward<Rest>(r)...);
	}

	//inputs should be of type map_pair<Key,Value>
	template<typename First,typename... T>
	constexpr auto make_ct_map(First&& k,T&& ... rest)
	{
		using Decayed=typename std::decay<First>::type;
		return make_ct_map<compare<typename Decayed::key_type>>(std::forward<First>(k),std::forward<T>(rest)...);
	}

	template<typename Comp,typename T,std::size_t N>
	constexpr auto make_ct_map(std::array<T,N> const& in)
	{
		return ct_map<typename T::key_type,typename T::mapped_type,N,Comp>(in);
	}

	template<typename T,std::size_t N>
	constexpr auto make_ct_map(std::array<T,N> const& in)
	{
		return make_ct_map<compare<typename T::key_type>>(in);
	}

#endif
	template<typename Type=void,typename... Args>
	constexpr std::array<typename detail::ma_ret<Type,Args...>::type,sizeof...(Args)> make_array(Args&& ... args)
	{
		using RetType=typename detail::ma_ret<Type, Args...>::type;
		return
		{{
			RetType(std::forward<Args>(args))...
		}};
	}

	namespace detail {
		template<typename Type,typename Tuple,typename Ix>
		struct ca_type_h;
		template<typename Type,typename Tuple,std::size_t... Ix>
		struct ca_type_h<Type,Tuple,index_sequence<Ix...>> {
			using type=std::array<typename ma_ret<Type,typename std::tuple_element<Ix,Tuple>::type...>::type,sizeof...(Ix)>;
		};
		template<typename Type,typename Tuple>
		struct ca_type:ca_type_h<Type,
			typename std::remove_reference<Tuple>::type,
			make_index_sequence<std::tuple_size<typename std::remove_reference<Tuple>::type>::value>>{

		};

		template<typename Type,typename Tuple,std::size_t... I>
		constexpr typename ca_type<Type,Tuple>::type conv_array(Tuple&& tup,index_sequence<I...>)
		{
			return {{  generic_get<I>(std::forward<Tuple>(tup))... }};
		}
	}

	template<typename Type=void,typename Tuple>
	constexpr typename detail::ca_type<Type,Tuple>::type conv_array(Tuple&& args)
	{
		constexpr auto TS=std::tuple_size<typename std::remove_reference<Tuple>::type>::value;
		return detail::conv_array<Type>(std::forward<Tuple>(args),make_index_sequence<TS>());
	}

	//the number of elements accessible by generic_get
	template<typename T>
	struct get_max {
	private:
		template<typename U>
		struct gm {
			constexpr static std::size_t const value=0;
		};

		template<typename... U>
		struct gm<std::tuple<U...>> {
			constexpr static std::size_t value=sizeof...(U);
		};

		template<typename U,std::size_t N>
		struct gm<std::array<U,N>> {
			constexpr static std::size_t value=N;
		};

#if _EXALG_HAS_CPP_17
		template<typename... U>
		struct gm<std::variant<U...>> {
			constexpr static std::size_t value=sizeof...(U);
		};
#endif

	public:
		constexpr static std::size_t const value=gm<typename std::remove_cv<typename std::remove_reference<T>::type>::type>::value;
	};

#if _EXALG_HAS_CPP_17
	template<typename T>
	constexpr std::size_t get_max_v=get_max<T>::value;

	namespace detail {

		template<typename Ret,std::size_t I,typename Funcs,typename...Args>
		constexpr Ret apply_single(Funcs&& funcs,Args&& ... args)
		{
			return static_cast<Ret>(generic_get<I>(std::forward<Funcs>(funcs))(std::forward<Args>(args)...));
		}

		template<typename Ret,typename IndexSequence,typename Funcs,typename... Args>
		struct get_jump_table;

		template<typename Ret,std::size_t... Is,typename Funcs,typename... Args>
		struct get_jump_table<Ret,index_sequence<Is...>,Funcs,Args...> {
			using Func=Ret(Funcs&&,Args&& ...);
			static constexpr Func* jtable[]={&apply_single<Ret,Is,Funcs,Args...>...};
		};

		template<typename Ret,std::size_t... Is,typename Funcs,typename... Args>
		constexpr Ret apply_ind_jump_h(std::size_t i,index_sequence<Is...>,Funcs&& funcs,Args&&... args)
		{
			return get_jump_table<Ret,index_sequence<Is...>,Funcs,Args...>::jtable[i](std::forward<Funcs>(funcs),std::forward<Args>(args)...);
		}

		template<typename Ret,std::size_t N,typename Funcs,typename... Args>
		constexpr Ret apply_ind_jump(std::size_t i,Funcs&& funcs,Args&& ... args)
		{
			return apply_ind_jump_h<Ret>(i,make_index_sequence<N>(),std::forward<Funcs>(funcs),std::forward<Args>(args)...);
		}

		template<typename Ret,std::size_t I,std::size_t Max,typename Tuple,typename... Args>
		constexpr Ret apply_ind_linear_h(std::size_t i,Tuple&& funcs,Args&& ... args)
		{
			if constexpr(I<Max)
			{
				if(i==I)
				{
					return apply_single<Ret,I>(std::forward<Tuple>(funcs))(std::forward<Args>(args)...);
				}
				return apply_ind_linear_h<Ret,I+1,Max>(i,std::forward<Tuple>(funcs),std::forward<Args>(args)...);
			}
			else
			{
				throw std::invalid_argument("Index too high");
			}
		}

		template<typename Ret,std::size_t NumFuncs,typename Tuple,typename... Args>
		constexpr Ret apply_ind_linear(std::size_t i,Tuple&& funcs,Args&& ... args)
		{
			return apply_ind_linear_h<Ret,0,NumFuncs>(i,std::forward<Tuple>(funcs),std::forward<Args>(args)...);
		}

		template<typename Ret,std::size_t Lower,std::size_t Upper,typename Funcs,typename... Args>
		constexpr Ret apply_ind_bh(std::size_t i,Funcs&& funcs,Args&& ... args)
		{
			if constexpr(Lower<Upper)
			{
				constexpr size_t I=(Upper-Lower)/2+Lower;
				if(i==I)
				{
					return apply_single<Ret,I>(std::forward<Funcs>(funcs),std::forward<Args>(args)...);
				}
				else if(i<I)
				{
					return apply_ind_bh<Ret,Lower,I>(i,std::forward<Funcs>(funcs),std::forward<Args>(args)...);
				}
				else
				{
					return apply_ind_bh<Ret,I+1,Upper>(i,std::forward<Funcs>(funcs),std::forward<Args>(args)...);
				}
			}
			else
			{
				throw std::invalid_argument("Index too high");
			}
		}

		template<typename Ret,std::size_t NumFuncs,typename Funcs,typename... Args>
		constexpr Ret apply_ind_bsearch(std::size_t i,Funcs&& funcs,Args&& ... args)
		{
			return apply_ind_bh<Ret,0,NumFuncs>(i,std::forward<Funcs>(funcs),std::forward<Args>(args)...);
		}
	}

	//Returns static_cast<Ret>(get<i>(std::forward<Funcs>(funcs))(std::forward<Args>(args)...)); Ret can be void.
	//Assumes i is less than NumFuncs, otherwise behavior is undefined.
	//Other overloads automatically determine Ret and NumFuncs if they are not supplied.
	template<typename Ret,std::size_t NumFuncs,typename Funcs,typename... Args>
	constexpr Ret apply_ind(std::size_t i,Funcs&& funcs,Args&&... args)
	{
		//MSVC currently can't inline the function pointers used by jump so I have a somewhat arbitrary
		//heuristic for choosing which apply to use
		if constexpr(NumFuncs<4)
		{
			return detail::apply_ind_bsearch<Ret,NumFuncs>(i,std::forward<Funcs>(funcs),std::forward<Args>(args)...);
		}
		else
		{
			return detail::apply_ind_jump<Ret,NumFuncs>(i,std::forward<Funcs>(funcs),std::forward<Args>(args)...);
		}
	}

	template<std::size_t NumFuncs,typename Ret,typename Funcs,typename... Args>
	constexpr Ret apply_ind(std::size_t i,Funcs&& funcs,Args&&... args)
	{
		return apply_ind<Ret,NumFuncs>(i,std::forward<Funcs>(funcs),std::forward<Args>(args)...);
	}

	template<std::size_t NumFuncs,typename Funcs,typename... Args>
	constexpr decltype(auto) apply_ind(std::size_t i,Funcs&& funcs,Args&& ... args)
	{
		if constexpr(NumFuncs==0)
		{
			return;
		}
		else
		{
			using Ret=decltype(generic_get<0>(std::forward<Funcs>(funcs))(std::forward<Args>(args)...));
			return apply_ind<Ret,NumFuncs>(i,std::forward<Funcs>(funcs),std::forward<Args>(args)...);
		}
	}

	template<typename Ret,typename Funcs,typename... Args>
	constexpr Ret apply_ind(std::size_t i,Funcs&& funcs,Args&&... args)
	{
		constexpr std::size_t N=get_max<std::remove_cv_t<std::remove_reference_t<Funcs>>>::value;
		return apply_ind<Ret,N>(i,std::forward<Funcs>(funcs),std::forward<Args>(args)...);
	}

	template<typename Funcs,typename... Args>
	constexpr decltype(auto) apply_ind(std::size_t i,Funcs&& funcs,Args&& ... args)
	{
		constexpr std::size_t N=get_max<std::remove_cv_t<std::remove_reference_t<Funcs>>>::value;
		return apply_ind<N>(i,std::forward<Funcs>(funcs),std::forward<Args>(args)...);
	}
#endif
	template<typename FindNext,typename... IndParser>
	class CSVParserBase:protected FindNext,protected std::tuple<IndParser...> {
	private:
		using Parsers=std::tuple<IndParser...>;
	public:
		std::size_t parse(char const* str,std::size_t len)
		{
			throw "Not implemented";
		}
		std::size_t parse(char const*)
		{
			throw "Not implemented";
		}
	};

#if _EXALG_HAS_CPP_14
	namespace detail {
		template<typename Char,typename Value>
		struct charp_pair {
			Char symbol;
			Value const* value;
		};
		//a compile string map of a search tree where each letter is traverse along a branch of possible strings 
		template<typename Char,typename Value,std::size_t N,typename Tree>
		class ct_string_map {
			std::array<Value,N> _values;
			Tree _layers; //Tree of char pairs
			static constexpr auto depth=std::tuple_size<Tree>::value;
		public:
			template<typename... CharT>
			constexpr ct_string_map(CharT const* const*... strings)
			{
				for(std::size_t i=0;i<sizeof...(CharT);++i)
				{
				}
			}
		};
	}

#endif
}
#endif