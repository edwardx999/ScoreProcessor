/*
Copyright 2019 Edward Xie

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once
#ifndef EXITERATOR_H
#define EXITERATOR_H
#include <iterator>
#include "exretype.h"
#include "exmacro.h"
#include <assert.h>
#include <utility>
#include <tuple>
#ifdef _MSVC_LANG
#define _EXITERATOR_HAS_CPP20 (_MSVC_LANG>=202000l)
#define _EXITERATOR_HAS_CPP17 (_MSVC_LANG>=201700l)
#else
#define _EXITERATOR_HAS_CPP20 (__cplusplus>=202000l)
#define _EXITERATOR_HAS_CPP17 (__cplusplus>=201700l)
#endif
#if _EXITERATOR_HAS_CPP17
#define _EXITERATOR_NODISCARD [[nodiscard]]
#else
#define _EXITERATOR_NODISCARD
#endif
namespace exlib {
	/*
		A curiously recurring template pattern base for iterators.
		T is value_type of iterator.
	*/
	template<typename T,typename Derived>
	struct iterator_base {
		using value_type=T;
		using difference_type=std::ptrdiff_t;
		using pointer=T*;
		using reference=T&;
		using iterator_category=std::random_access_iterator_tag;
	private:
		T* _base;
		constexpr Derived& chain() noexcept
		{
			return *static_cast<Derived*>(this);
		}
	public:
		constexpr T* base() const noexcept
		{
			return _base;
		}
		constexpr iterator_base() noexcept
		{}
		constexpr iterator_base(T* base) noexcept:_base(base)
		{}
		constexpr iterator_base(Derived const& other) noexcept:_base(other.base())
		{}
		constexpr Derived& operator=(Derived const& other) noexcept
		{
			_base=other.base();
			return chain();
		}
		constexpr T& operator*() const noexcept
		{
			return *_base;
		}
		constexpr T* operator->() const noexcept
		{
			return _base;
		}
		constexpr Derived operator++(int) noexcept
		{
			return Derived(_base++);
		}
		constexpr Derived& operator++() noexcept
		{
			++_base;
			return chain();
		}
		constexpr Derived operator--(int) noexcept
		{
			return Derived(_base--);
		}
		constexpr Derived& operator--() noexcept
		{
			--_base;
			return chain();
		}
		constexpr std::ptrdiff_t operator-(Derived other) const noexcept
		{
			return _base-other.base();
		}
		constexpr Derived operator+(std::ptrdiff_t s) const noexcept
		{
			return Derived(_base+s);
		}
		constexpr Derived operator-(std::ptrdiff_t s) const noexcept
		{
			return Derived(_base-s);
		}
		constexpr Derived& operator+=(std::ptrdiff_t s) noexcept
		{
			_base+=s;
			return chain();
		}
		constexpr Derived& operator-=(std::ptrdiff_t s) noexcept
		{
			_base-=s;
			return chain();
		}
		constexpr T& operator[](size_t s) const noexcept
		{
			return _base[s];
		}
#define iterator_base_comp(op) constexpr bool operator op(Derived other) const noexcept { return _base op other.base() ;}
		EXLIB_FOR_ALL_COMP_OPS(iterator_base_comp)
#undef comp
	};

	/*
		A curiously recurring template pattern base for reverse_iterators.
		T is value_type of iterator.
	*/
	template<typename T,typename Derived>
	struct riterator_base {
		using value_type=T;
		using difference_type=std::ptrdiff_t;
		using pointer=T*;
		using reference=T&;
		using iterator_category=std::random_access_iterator_tag;
	private:
		T* _base;
		constexpr Derived& chain()
		{
			return *static_cast<Derived*>(this);
		}
	public:
		constexpr T* base() const noexcept
		{
			return _base;
		}
		constexpr riterator_base() noexcept
		{}
		constexpr riterator_base(T* base) noexcept:_base(base)
		{}
		constexpr riterator_base(Derived const& other) noexcept:_base(other.base())
		{}
		constexpr Derived& operator=(Derived const& other) noexcept
		{
			_base=other.base();
			return chain();
		}
		constexpr T& operator*() const noexcept
		{
			return *(_base-1);
		}
		constexpr T* operator->() const noexcept
		{
			return _base-1;
		}
		constexpr Derived operator--(int) noexcept
		{
			return Derived(_base++);
		}
		constexpr Derived& operator--() noexcept
		{
			++_base;
			return chain();
		}
		constexpr Derived operator++(int) noexcept
		{
			return Derived(_base--);
		}
		constexpr Derived& operator++() noexcept
		{
			--_base;
			return chain();
		}
		constexpr std::ptrdiff_t operator-(Derived other) const noexcept
		{
			return other.base()-_base;
		}
		constexpr Derived operator+(std::ptrdiff_t s) const noexcept
		{
			return Derived(_base-s);
		}
		constexpr Derived operator-(std::ptrdiff_t s) const noexcept
		{
			return Derived(_base+s);
		}
		constexpr Derived& operator+=(std::ptrdiff_t s) noexcept
		{
			_base-=s;
			return chain();
		}
		constexpr Derived& operator-=(std::ptrdiff_t s) noexcept
		{
			_base+=s;
			return chain();
		}
		constexpr T& operator[](size_t s) const noexcept
		{
			return *(_base-s-1);
		}
#define riterator_base_comp(op) constexpr bool operator op(Derived other){ return other.base() op _base ;}
		EXLIB_FOR_ALL_COMP_OPS(riterator_base_comp)
#undef comp
	};

	template<typename T>
	struct const_iterator:iterator_base<T const,const_iterator<T>> {
		using iterator_base<T const,const_iterator<T>>::iterator_base;
	};
	template<typename T>
	struct iterator:iterator_base<T,iterator<T>> {
		using iterator_base<T,iterator<T>>::iterator_base;
		iterator(const_iterator<T> ci):iterator_base(ci.base())
		{}
	};

	template<typename T>
	struct const_reverse_iterator:iterator_base<T const,const_reverse_iterator<T>> {
		using riterator_base<T const,const_reverse_iterator<T>>::riterator_base;
	};
	template<typename T>
	struct reverse_iterator:riterator_base<T,reverse_iterator<T>> {
		using riterator_base<T,reverse_iterator<T>>::riterator_base;
		reverse_iterator(const_reverse_iterator<T> cri):riterator_base(cri.base())
		{}
	};

	namespace cstring_iterator_impl {

		/*
			Iterator for a c-string so that an end iterator can be passed to stl algorithms without calculating strlen.
		*/
		template<typename CharType,CharType terminator=CharType{}>
		class cstring_iterator {
			CharType* _loc;
		public:
			using reference=CharType&;
			using value_type=CharType;
			using pointer=CharType*;
			using iterator_category=std::forward_iterator_tag;
			using difference_type=std::ptrdiff_t;
			constexpr cstring_iterator(CharType* str=nullptr):_loc(str)
			{}
			template<typename Other>
			constexpr cstring_iterator(cstring_iterator<Other,terminator> str):_loc(str.operator->())
			{}

			_EXITERATOR_NODISCARD constexpr bool operator==(cstring_iterator o) const noexcept
			{
				return _loc==o._loc;
			}
			_EXITERATOR_NODISCARD constexpr bool operator!=(cstring_iterator o) const noexcept
			{
				return _loc!=o._loc;
			}
			constexpr cstring_iterator& operator++() noexcept
			{
				++_loc;
				if(*_loc==terminator)
				{
					_loc=nullptr;
				}
				return *this;
			}
			constexpr cstring_iterator operator++(int) noexcept
			{
				auto copy(*this);
				++(*this);
				return copy;
			}
			_EXITERATOR_NODISCARD constexpr reference operator*() const noexcept
			{
				return *_loc;
			}
			_EXITERATOR_NODISCARD constexpr pointer operator->() const noexcept
			{
				return _loc;
			}
			_EXITERATOR_NODISCARD explicit constexpr operator CharType* () const noexcept
			{
				return _loc;
			}
			_EXITERATOR_NODISCARD static constexpr cstring_iterator end() noexcept
			{
				return {nullptr};
			}
			_EXITERATOR_NODISCARD constexpr cstring_iterator begin() const noexcept
			{
				return *this;
			}
		};

		template<typename Iter>
		_EXITERATOR_NODISCARD constexpr Iter begin(Iter it) noexcept
		{
			return it;
		}
		template<typename Iter>
		_EXITERATOR_NODISCARD constexpr Iter end(Iter) noexcept
		{
			return {nullptr};
		}

#if _EXITERATOR_HAS_CPP17&&defined(__INTELLISENSE__)
		template<typename CharType>
		cstring_iterator(CharType*)->cstring_iterator<CharType>;
#endif
	}
	using cstring_iterator_impl::cstring_iterator;

	template<typename CharType>
	constexpr cstring_iterator<CharType> make_cstring_iterator(CharType* str)
	{
		return {str};
	}

	namespace iterator_detail {
		template<typename T>
		class has_decrement_operator {
			template<typename U>
			constexpr static auto get(U u)-> decltype(--u,u--,true);
			template<typename U,typename... Extra>
			constexpr static char get(U,Extra...);
		public:
			static constexpr bool value=std::is_same<bool,decltype(get(std::declval<T>()))>::value;
		};

		template<typename T,typename Derived,bool has_dec=has_decrement_operator<T>::value>
		struct inherit_decrement {
		private:
			Derived& get_chained() noexcept
			{
				return static_cast<Derived&>(*this);
			}
		public:
			constexpr Derived& operator--() noexcept(noexcept(--get_chained()._base))
			{
				Derived& me=get_chained();
				--me._base;
				return me;
			}
			constexpr Derived operator--(int) noexcept(noexcept(get_chained()._base--))
			{
				Derived& me=static_cast<Derived&>(*this);
				Derived copy(me);
				--me._base;
				return copy;
			}
		};

		template<typename T,typename Derived>
		struct inherit_decrement<T,Derived,false> {};

		template<typename T,typename PtrDiffT>
		class has_subscript_operator {
			template<typename U>
			constexpr static auto get(U u)-> decltype(u[PtrDiffT{}],true);
			template<typename U,typename... Extra>
			constexpr static char get(U,Extra...);
		public:
			static constexpr bool value=std::is_same<bool,decltype(get(std::declval<T>()))>::value;
		};

		template<typename T,typename PtrDiffT,typename Derived,typename ValueType,bool has_dec=has_subscript_operator<T,PtrDiffT>::value>
		struct inherit_subscript {
		private:
			constexpr Derived const& get_chained() const noexcept
			{
				return static_cast<Derived const&>(*this);
			}
		public:
			constexpr ValueType operator[](PtrDiffT s) const noexcept(noexcept(get_chained().functor()(get_chained()._base[s])))
			{
				return get_chained().functor()(get_chained()._base[s]);
			}
		};

		template<typename T,typename PtrDiffT,typename Derived,typename ValueType>
		struct inherit_subscript<T,PtrDiffT,Derived,ValueType,false> {};


		template<typename T,typename PtrDiffT>
		class has_plus_equal {
			template<typename U>
			constexpr static auto get(U u,PtrDiffT p)-> decltype(u+=p,true);
			template<typename U,typename... Extra>
			constexpr static char get(U,PtrDiffT p,Extra...);
		public:
			static constexpr bool value=std::is_same<bool,decltype(get(std::declval<T>(),std::declval<PtrDiffT>()))>::value;
		};

		template<typename T,typename PtrDiffT,typename Derived,bool has_dec=has_plus_equal<T,PtrDiffT>::value>
		struct inherit_plus_equal {
		private:
			constexpr Derived& get_chained() noexcept
			{
				return static_cast<Derived&>(*this);
			}
		public:
			constexpr Derived& operator+=(PtrDiffT p) noexcept(noexcept(get_chained()._base+=p))
			{
				Derived& me=static_cast<Derived&>(*this);
				me._base+=p;
				return me;
			}
		};

		template<typename T,typename PtrDiffT,typename Derived>
		struct inherit_plus_equal<T,PtrDiffT,Derived,false> {};

		template<typename T,typename PtrDiffT>
		class has_minus_equal {
			template<typename U>
			constexpr static auto get(U u,PtrDiffT p)-> decltype(u-=p,true);
			template<typename U,typename... Extra>
			constexpr static char get(U,PtrDiffT p,Extra...);
		public:
			static constexpr bool value=std::is_same<bool,decltype(get(std::declval<T>(),std::declval<PtrDiffT>()))>::value;
		};

		template<typename T,typename PtrDiffT,typename Derived,bool has_dec=has_minus_equal<T,PtrDiffT>::value>
		struct inherit_minus_equal {
		private:
			constexpr Derived& get_chained() noexcept
			{
				return static_cast<Derived&>(*this);
			}
		public:
			constexpr Derived& operator-=(PtrDiffT p) noexcept(noexcept(get_chained()._base-=p))
			{
				Derived& me=static_cast<Derived&>(*this);
				me._base-=p;
				return me;
			}
		};

		template<typename T,typename PtrDiffT,typename Derived>
		struct inherit_minus_equal<T,PtrDiffT,Derived,false> {};

		template<typename T>
		class ptr_wrapper {
			T _val;
		public:
			template<typename... Args>
			constexpr ptr_wrapper(Args&&... args) noexcept(noexcept(T(std::forward<Args>(args)...))):_val(std::forward<Args>(args)...)
			{}
			constexpr T& operator*() const noexcept
			{
				return _val;
			}
			constexpr T* operator->() const noexcept
			{
				return &_val;
			}
		};

		template<typename T>
		using iter_diff_t=typename std::iterator_traits<T>::difference_type;

		template<typename Iter,typename Functor>
		using func_value_t=decltype(std::declval<Functor const&>()(std::declval<typename std::iterator_traits<Iter>::value_type>()));
	}

	template<typename Base,typename Functor>
	class transform_iterator:
		empty_store<Functor>,
		public iterator_detail::inherit_decrement<Base,transform_iterator<Base,Functor>>,
		public iterator_detail::inherit_plus_equal<Base,iterator_detail::iter_diff_t<Base>,transform_iterator<Base,Functor>>,
		public iterator_detail::inherit_minus_equal<Base,iterator_detail::iter_diff_t<Base>,transform_iterator<Base,Functor>>,
		public iterator_detail::inherit_subscript<Base,iterator_detail::iter_diff_t<Base>,transform_iterator<Base,Functor>,iterator_detail::func_value_t<Base,Functor>> {
		Base _base;
		using FunctorStore=empty_store<Functor>;
		friend struct iterator_detail::inherit_decrement<Base,transform_iterator<Base,Functor>>;
		friend struct iterator_detail::inherit_plus_equal<Base,iterator_detail::iter_diff_t<Base>,transform_iterator<Base,Functor>>;
		friend struct iterator_detail::inherit_minus_equal<Base,iterator_detail::iter_diff_t<Base>,transform_iterator<Base,Functor>>;
		friend struct iterator_detail::inherit_subscript<Base,iterator_detail::iter_diff_t<Base>,transform_iterator<Base,Functor>,iterator_detail::func_value_t<Base,Functor>>;
	public:
		using iterator_category=typename std::iterator_traits<Base>::iterator_category;
		using difference_type=iterator_detail::iter_diff_t<Base>;
		using value_type=iterator_detail::func_value_t<Base,Functor>;
		using reference=value_type;
		using pointer=iterator_detail::ptr_wrapper<value_type>;
		constexpr transform_iterator(Base base,Functor f) noexcept(std::is_nothrow_copy_constructible<Base>::value&& std::is_nothrow_move_constructible<Functor>::value):_base(base),FunctorStore(std::move(f))
		{}
		constexpr transform_iterator(Base base) noexcept(std::is_nothrow_copy_constructible<Base>::value&& std::is_nothrow_default_constructible<Functor>::value):_base(base)
		{}
		constexpr transform_iterator() noexcept(std::is_nothrow_default_constructible<Base>::value&& std::is_nothrow_default_constructible<Functor>::value)
		{}
		constexpr Base base() const noexcept(std::is_nothrow_copy_constructible<Base>::value)
		{
			return _base;
		}
		constexpr Functor functor() const noexcept(std::is_nothrow_copy_constructible<Functor>::value)
		{
			return empty_store<Functor>::get();
		}
		constexpr transform_iterator& operator++() noexcept(noexcept(++_base))
		{
			++_base;
			return *this;
		}
		constexpr transform_iterator operator++(int) noexcept(noexcept(++_base)&&std::is_nothrow_copy_constructible<transform_iterator>::value)
		{
			auto copy(*this);
			++_base;
			return copy;
		}
		constexpr reference operator*() const noexcept(noexcept(std::declval<Functor const&>()(*_base)))
		{
			return FunctorStore::get()(*_base);
		}
		constexpr pointer operator->() const noexcept(noexcept(std::declval<Functor const&>()(*_base)))
		{
			return {operator*()};
		}
	};

#define make_comp_op_for_gi(op) template<typename Base,typename Functor> constexpr auto operator op(transform_iterator<Base,Functor> const& a,transform_iterator<Base,Functor> const& b) noexcept(noexcept(a.base() op b.base())) -> decltype(a.base() op b.base()) {return a.base() op b.base();}
	EXLIB_FOR_ALL_COMP_OPS(make_comp_op_for_gi)
#undef make_comp_op_for_gi

	template<typename Base,typename Functor>
	constexpr auto operator+(transform_iterator<Base,Functor> const& it,typename transform_iterator<Base,Functor>::difference_type d) noexcept(noexcept(it.base()+d)&&std::is_nothrow_copy_constructible<Functor>::value) -> decltype(it.base()+d,transform_iterator<Base,Functor>{it.base(),it.functor()})
	{
		return {it.base()+d,it.functor()};
	}

	template<typename PtrDiffT,typename Base,typename Functor>
	constexpr auto operator+(PtrDiffT d,transform_iterator<Base,Functor> const& it) noexcept(noexcept(it+d)) -> decltype(it+d)
	{
		return it+d;
	}

	template<typename Base,typename Functor>
	constexpr auto operator-(transform_iterator<Base,Functor> const& it,typename transform_iterator<Base,Functor>::difference_type d) noexcept(noexcept(it.base()-d)&&std::is_nothrow_copy_constructible<Functor>::value) -> decltype(it.base()-d,transform_iterator<Base,Functor>{it.base(),it.functor()})
	{
		return {it.base()-d,it.functor()};
	}

	template<typename Base,typename Functor>
	constexpr auto operator-(transform_iterator<Base,Functor> const& a,transform_iterator<Base,Functor> const& b) noexcept(noexcept(a.base()-b.base())) -> decltype(a.base()-b.base(),typename transform_iterator<Base,Functor>::difference_type{})
	{
		return a.base()-b.base();
	}

	template<typename Base,typename Functor>
	constexpr transform_iterator<Base,typename std::decay<Functor>::type> make_transform_iterator(Base base,Functor&& f)
	{
		return transform_iterator<Base,typename std::decay<Functor>::type>(base,std::forward<Functor>(f));
	}

	template<typename IntegralType,IntegralType Increment=1>
	class count_iterator {
		IntegralType _index;
	public:
		constexpr count_iterator() noexcept
		{}
		constexpr count_iterator(IntegralType index) noexcept:_index(index)
		{}
		using value_type=IntegralType;
		using reference=value_type const&;
		using pointer=value_type const*;
		using difference_type=std::ptrdiff_t;
		using iterator_category=std::random_access_iterator_tag;
		constexpr value_type operator*() const noexcept
		{
			return _index;
		}
		constexpr pointer operator->() const noexcept
		{
			return &_index;
		}
		constexpr count_iterator operator++(int) noexcept
		{
			auto copy{*this};
			_index+=Increment;
			return copy;
		}
		constexpr count_iterator& operator++() noexcept
		{
			_index+=Increment;
			return *this;
		}
		constexpr count_iterator operator--(int) noexcept
		{
			auto copy{*this};
			_index-=Increment;
			return copy;
		}
		constexpr count_iterator& operator--() noexcept
		{
			_index-=Increment;
			return *this;
		}
		constexpr count_iterator& operator+=(difference_type dif) noexcept
		{
			_index+=dif*Increment;
			return *this;
		}
		constexpr count_iterator& operator-=(difference_type dif) noexcept
		{
			_index-=dif*Increment;
			return *this;
		}
		constexpr value_type operator[](difference_type s) const noexcept
		{
			return _index+s*Increment;
		}
	};
	template<typename IntegralType,IntegralType Increment>
	constexpr count_iterator<IntegralType,Increment> operator+(count_iterator<IntegralType,Increment> it,std::ptrdiff_t diff) noexcept
	{
		return {*it+diff*Increment};
	}
	template<typename IntegralType,IntegralType Increment>
	constexpr count_iterator<IntegralType,Increment> operator+(std::ptrdiff_t diff,count_iterator<IntegralType,Increment> it) noexcept
	{
		return {*it+diff*Increment};
	}
	template<typename IntegralType,IntegralType Increment>
	constexpr count_iterator<IntegralType,Increment> operator-(count_iterator<IntegralType,Increment> it,std::ptrdiff_t diff) noexcept
	{
		return {*it-diff*Increment};
	}
	template<typename IntegralType,IntegralType Increment>
	constexpr std::ptrdiff_t operator-(count_iterator<IntegralType,Increment> it,count_iterator<IntegralType,Increment> it2)
	{
		assert((*it-*it2)%Increment==0);
		return (*it-*it2)/Increment;
	}

#define make_comp_op_for_gi(op) template<typename Integral,Integral Increment> constexpr bool operator op(count_iterator<Integral,Increment> a,count_iterator<Integral,Increment> b) noexcept {return *a op *b;}
	EXLIB_FOR_ALL_COMP_OPS(make_comp_op_for_gi)
#undef make_comp_op_for_gi

	using index_iterator=count_iterator<std::size_t>;

	template<typename IntegralType>
	count_iterator<IntegralType> make_count_iterator(IntegralType c)
	{
		return {c};
	}

	namespace filter_iterator_detail {
		struct get_arrow_operator {
			template<typename Iter>
			constexpr static auto get(Iter iter)-> decltype(iter.operator->())
			{
				return iter.operator->();
			}
			template<typename Type,typename... Extra>
			constexpr static Type* get(Type* ptr,Extra...)
			{
				return ptr;
			}
		};
	}
	template<typename Base,typename Functor>
	class filter_iterator:empty_store<Functor> {
		Base _base;
		Base _end;
		using Traits=std::iterator_traits<Base>;
		constexpr void advance_until_satisfied()
		{
			for(;_base!=_end;++_base)
			{
				if(empty_store<Functor>::get()(*_base))
				{
					return;
				}
			}
		}
	public:
		using value_type=typename Traits::value_type;
		using difference_type=typename Traits::difference_type;
		using pointer=typename Traits::pointer;
		using reference=typename Traits::reference;
		using iterator_category=std::forward_iterator_tag;

		constexpr filter_iterator(Base iter,Base end,Functor f=Functor()) noexcept(std::is_nothrow_move_constructible<Functor>::value):_base(iter),_end(end),empty_store<Functor>(std::move(f))
		{
			advance_until_satisfied();
		}

		constexpr filter_iterator(Base end,Functor f) noexcept(std::is_nothrow_move_constructible<Functor>::value):_base(end),_end(end),empty_store<Functor>(std::move(f))
		{}

		constexpr filter_iterator(Base end) noexcept(std::is_nothrow_default_constructible<Functor>::value):_base(end),_end(end)
		{}

		constexpr Functor predicate() const noexcept(std::is_nothrow_copy_constructible<Functor>::value)
		{
			return empty_store<Functor>::get();
		}

		constexpr reference operator*() const noexcept
		{
			return *_base;
		}
		constexpr pointer operator->() const noexcept
		{
			return filter_iterator_detail::get_arrow_operator::get(_base);
		}
		constexpr filter_iterator& operator++() noexcept (noexcept(empty_store<Functor>::get()(*_base)))
		{
			++_base;
			advance_until_satisfied();
			return *this;
		}
		constexpr filter_iterator operator++(int) noexcept (noexcept(empty_store<Functor>::get()(*_base)))
		{
			auto copy(*this);
			++_base;
			advance_until_satisfied();
			return copy;
		}
		constexpr bool operator==(filter_iterator const& o) const noexcept
		{
			assert(_end==o._end);
			return _base==o._base;
		}
		constexpr bool operator!=(filter_iterator const& o) const noexcept
		{
			assert(_end==o._end);
			return _base!=o._base;
		}
		constexpr Base base() const noexcept(std::is_nothrow_copy_constructible<Base>::value)
		{
			return _base;
		}
	};

	template<typename Iter,typename Functor>
	filter_iterator<Iter,Functor> make_filter_iterator(Iter iter,Iter end,Functor f)
	{
		return {iter,end,std::move(f)};
	}

	//	namespace indexed_iterator_detail {
	//		template<typename Value>
	//		struct value_with_index {
	//			Value value;
	//			std::size_t index;
	//			template<typename Val>
	//			constexpr value_with_index(Val&& val,std::size_t ind):value(std::forward<Val>(val)),index(ind) {}
	//			
	//			template<typename T>
	//			constexpr value_with_index(value_with_index<T> const& val):value(val.value),index(val.index) {}
	//
	//			template<typename T>
	//			constexpr value_with_index(value_with_index<T>&& val) : value(std::move(val.value)), index(val.index) {}
	//
	//			template<typename T>
	//			constexpr value_with_index& operator=(value_with_index<T> const& val) noexcept(noexcept(value=val.value))
	//			{
	//				value=val.value;
	//				index=val.index;
	//				return *this;
	//			}
	//			template<typename T>
	//			constexpr value_with_index& operator=(value_with_index<T>&& val) noexcept(noexcept(value=std::move(val.value)))
	//			{
	//				value=std::move(val.value);
	//				index=val.index;
	//				return *this;
	//			}
	//		};
	//
	//		template<typename Val1,typename Val2>
	//		void swap(value_with_index<Val1>& a,value_with_index<Val2>& b) noexcept
	//		{
	//			using std::swap;
	//			swap(a.value,b.value);
	//			swap(a.index,b.index);
	//		}
	//
	//		template<typename Val1,typename Val2>
	//		void swap(value_with_index<Val1>&& a,value_with_index<Val2>&& b) noexcept
	//		{
	//			using std::swap;
	//			swap(a.value,b.value);
	//			swap(a.index,b.index);
	//		}
	//
	//		template<typename Val1,typename Val2>
	//		void swap(value_with_index<Val1>&& a,value_with_index<Val2>& b) noexcept
	//		{
	//			using std::swap;
	//			swap(a.value,b.value);
	//			swap(a.index,b.index);
	//		}
	//
	//		template<typename Val1,typename Val2>
	//		void swap(value_with_index<Val1>& a,value_with_index<Val2>&& b) noexcept
	//		{
	//			using std::swap;
	//			swap(a.value,b.value);
	//			swap(a.index,b.index);
	//		}
	//
	//		template<typename Base,typename Derived,
	//			bool random_access=
	//				std::is_convertible<
	//					typename std::iterator_traits<Base>::iterator_category*,
	//					std::random_access_iterator_tag*>::value>
	//		class indexed_iterator_base {
	//			Base _base;
	//			std::size_t _index;
	//		public:
	//			using difference_type=typename Base::difference_type;
	//			using value_type=value_with_index<typename Base::value_type>;
	//			using reference=value_with_index<typename Base::reference>;
	//			using pointer=iterator_detail::ptr_wrapper<reference>;
	//			using iterator_category=typename Base::iterator_category;
	//			constexpr indexed_iterator_base() noexcept:_base{},_index{}{}
	//			constexpr indexed_iterator_base(Base base, std::size_t offset=0) noexcept:_base(base), _index{offset}{}
	//			constexpr reference operator*() const noexcept
	//			{
	//				return {_base[_index],_index};
	//			}
	//			constexpr reference operator[](std::size_t index) const noexcept
	//			{
	//				return {_base[index+index],_index+index};
	//			}
	//			constexpr pointer operator->() const noexcept
	//			{
	//				return {**this};
	//			}
	//			constexpr Derived& operator++() noexcept
	//			{
	//				++_index;
	//				return static_cast<Derived&>(*this);
	//			}
	//			constexpr Derived operator++(int) noexcept
	//			{
	//				Derived copy{*this};
	//				++_index;
	//				return copy;
	//			}
	//			constexpr Derived& operator--() noexcept
	//			{
	//				--_index;
	//				return static_cast<Derived&>(*this);
	//			}
	//			constexpr Derived operator--(int) noexcept
	//			{
	//				Derived copy{*this};
	//				--_index;
	//				return copy;
	//			}
	//			constexpr Derived& operator+=(difference_type d) noexcept
	//			{
	//				_index+=d;
	//				return static_cast<Derived&>(*this);
	//			}
	//			constexpr Derived& operator-=(difference_type d) noexcept
	//			{
	//				_index-=d;
	//				return static_cast<Derived&>(*this);
	//			}
	//			constexpr Base base() const noexcept
	//			{
	//				return _base;
	//			}
	//			constexpr std::size_t index() const noexcept
	//			{
	//				return _index;
	//			}
	//		};
	//		
	//		template<typename Base,typename Derived>
	//		class indexed_iterator_base<Base,Derived,false> {
	//			Base _base;
	//			std::size_t _index;
	//		public:
	//			using difference_type=typename Base::difference_type;
	//			using value_type=value_with_index<typename Base::value_type>;
	//			using reference=value_with_index<typename Base::reference>;
	//			using pointer=iterator_detail::ptr_wrapper<reference>;
	//			using iterator_category=typename Base::iterator_category;
	//			indexed_iterator_base() noexcept:_base{},_index{}{}
	//			indexed_iterator_base(Base base, std::size_t offset=0) noexcept:_base(base), _index{offset}{}
	//			reference operator*() const noexcept
	//			{
	//				return {*_base,_index};
	//			}
	//			pointer operator->() const noexcept
	//			{
	//				return {**this};
	//			}
	//			Derived& operator++() noexcept
	//			{
	//				++_index;
	//				++_base;
	//				return static_cast<Derived&>(*this);
	//			}
	//			Derived operator++(int) noexcept
	//			{
	//				Derived copy{*this};
	//				++_index;
	//				++_base;
	//				return copy;
	//			}
	//			Derived& operator--() noexcept
	//			{
	//				--_index;
	//				--_base;
	//				return static_cast<Derived&>(*this);
	//			}
	//			Derived operator--(int) noexcept
	//			{
	//				Derived copy{*this};
	//				--_index;
	//				--_base;
	//				return copy;
	//			}
	//			Base base() const noexcept
	//			{
	//				return _base;
	//			}
	//			std::size_t index() const noexcept
	//			{
	//				return _index;
	//			}
	//		};
	//
	//	}
	//
	//	template<typename Base>
	//	class indexed_iterator:public indexed_iterator_detail::indexed_iterator_base<Base,indexed_iterator<Base>> {
	//		using MyBase=indexed_iterator_detail::indexed_iterator_base<Base,indexed_iterator<Base>>;
	//	public:
	//		using MyBase::MyBase;
	//	};
	//
	//	template<typename Base>
	//	constexpr auto operator+(indexed_iterator<Base> a,typename indexed_iterator<Base>::difference_type d) noexcept -> typename std::decay<decltype(a+=d,a)>::type
	//	{
	//		a+=d;
	//		return a;
	//	}
	//	template<typename Base>
	//	constexpr auto operator-(indexed_iterator<Base> a,typename indexed_iterator<Base>::difference_type d) noexcept -> typename std::decay<decltype(a-=d,a)>::type
	//	{
	//		a-=d;
	//		return a;
	//	}
	//	template<typename Base>
	//	constexpr auto operator-(indexed_iterator<Base> const& a,indexed_iterator<Base> const& b) noexcept -> decltype(a.base()-b.base())
	//	{
	//		return a.base()-b.base();
	//	}
	//
	//#define make_comp_op_for_index_iterator(op)\
	//	template<typename Base>\
	//	constexpr bool operator op(indexed_iterator<Base> const& a,indexed_iterator<Base> const& b)\
	//	{\
	//		return a.index() op b.index();\
	//	}
	//	make_comp_op_for_index_iterator(==)
	//		make_comp_op_for_index_iterator(!=)
	//		make_comp_op_for_index_iterator(<)
	//		make_comp_op_for_index_iterator(>)
	//		make_comp_op_for_index_iterator(<=)
	//		make_comp_op_for_index_iterator(>=)
	//#undef make_comp_op_for_index_iterator
	//
	//	template<typename Base>
	//	constexpr indexed_iterator<Base> make_indexed_iterator(Base b,std::size_t offset=0) noexcept
	//	{
	//		return {b,offset};
	//	}
	//
	//	template<typename Container>
	//	constexpr auto make_begin_indexed_iterator(Container&& cont) noexcept -> indexed_iterator<decltype(std::forward<Container>(cont).begin())>
	//	{
	//		return {std::forward<Container>(cont).begin()};
	//	}
	//
	//	template<typename Container>
	//	constexpr auto make_end_indexed_iterator(Container&& cont) noexcept -> indexed_iterator<decltype(std::forward<Container>(cont).size(),std::forward<Container>(cont).end())>
	//	{
	//		return {std::forward<Container>(cont).end(),std::forward<Container>(cont).size()};
	//	}

	template<typename... References>
	class multi_rvalue_reference;
	template<typename... References>
	class multi_reference;

	template<typename... References>
	constexpr multi_rvalue_reference<References...> move(multi_reference<References...> const& ref) noexcept;
	template<typename... References>
	constexpr multi_rvalue_reference<References...> move(multi_reference<References...> const&& ref) noexcept;
	template<typename... References>
	constexpr multi_rvalue_reference<References...> move(multi_reference<References...>&& ref) noexcept;
	template<typename... References>
	constexpr multi_rvalue_reference<References...> move(multi_reference<References...>& ref) noexcept;
}
namespace std {
	template<typename... References>
	constexpr exlib::multi_rvalue_reference<References...> move(exlib::multi_reference<References...>&& ref) noexcept
	{
		return exlib::move(ref);
	}
	template<typename... References>
	constexpr exlib::multi_rvalue_reference<References...> move(exlib::multi_reference<References...>& ref) noexcept
	{
		return exlib::move(ref);
	}
	template<typename... References>
	constexpr exlib::multi_rvalue_reference<References...> move(exlib::multi_reference<References...> const&& ref) noexcept
	{
		return exlib::move(ref);
	}
	template<typename... References>
	constexpr exlib::multi_rvalue_reference<References...> move(exlib::multi_reference<References...> const& ref) noexcept
	{
		return exlib::move(ref);
	}
}
namespace exlib {

	template<typename... References>
	class multi_rvalue_reference {
		std::tuple<decltype(std::move(std::declval<References>()))...> mutable refs;
	public:
		decltype(refs) const& base() const noexcept
		{
			return refs;
		}
		constexpr multi_rvalue_reference(multi_rvalue_reference const& args)=default;
		constexpr multi_rvalue_reference(multi_rvalue_reference&& args)=default;

		template<typename... Args>
		constexpr multi_rvalue_reference(Args&&... refs):refs(std::forward<Args>(refs)...)
		{}

		template<typename... Types>
		constexpr operator std::tuple<Types...>() const noexcept(std::is_nothrow_move_constructible<std::tuple<Types...>>::value)
		{
			return move_convert_help<std::tuple<Types...>>(make_index_sequence<sizeof...(References)>{});
		}
	private:
		template<typename Type,std::size_t... Indices>
		constexpr Type move_convert_help(std::index_sequence<Indices...>) const
		{
			return Type(std::move(std::get<Indices>(refs))...);
		}
	};

	template<typename... References>
	class multi_reference {
		std::tuple<References...> refs;
	public:
		decltype(refs) const& base() const noexcept
		{
			return refs;
		}

		constexpr multi_reference(multi_reference const& args) noexcept=default;
		constexpr multi_reference(multi_reference&& args) noexcept=default;

		template<typename... Refs>
		constexpr multi_reference(multi_rvalue_reference<Refs...> const& rrefs) noexcept:refs(rrefs.base())
		{}

		template<typename... Args>
		constexpr multi_reference(Args&&... refs) noexcept:refs(refs...)
		{}
		
		constexpr multi_reference& operator=(multi_reference const& args)
		{
			assign_help(make_index_sequence<sizeof...(References)>{},args.base());
			return *this;
		}

		template<typename... Types>
		constexpr multi_reference& operator=(multi_reference<Types...> const& args)
		{
			assign_help(make_index_sequence<sizeof...(References)>{},args.base());
			return *this;
		}

		template<typename... Types>
		constexpr multi_reference& operator=(multi_rvalue_reference<Types...> const& args)
		{
			assign_help(make_index_sequence<sizeof...(References)>{},std::move(args.base()));
			return *this;
		}

		template<typename... Types>
		constexpr multi_reference& operator=(std::tuple<Types...>&& args)
		{
			assign_help(make_index_sequence<sizeof...(References)>{},std::move(args));
			return *this;
		}

		template<typename... Types>
		constexpr multi_reference& operator=(std::tuple<Types...> const& args)
		{
			assign_help(make_index_sequence<sizeof...(References)>{},args);
			return *this;
		}

		template<typename... Types>
		constexpr operator std::tuple<Types...>() const noexcept(std::is_nothrow_copy_constructible<std::tuple<Types...>>::value)
		{
			return std::tuple<Types...>(refs);
		}
	private:
		template<std::size_t I,std::size_t I2,std::size_t... Indices,typename Tuple>
		constexpr void assign_help(index_sequence<I,I2,Indices...>,Tuple&& args)
		{
			assign_help(index_sequence<I>{},std::forward<Tuple>(args));
			assign_help(index_sequence<I2,Indices...>{},std::forward<Tuple>(args));
		}
		template<std::size_t Index,typename... Types>
		constexpr void assign_help(index_sequence<Index>,std::tuple<Types...> const& args)
		{
			std::get<Index>(refs)=std::get<Index>(args);
		}
		template<std::size_t Index,typename... Types>
		constexpr void assign_help(index_sequence<Index>,std::tuple<Types...>&& args)
		{
			std::get<Index>(refs)=std::move(std::get<Index>(args));
		}
	};

#define make_multi_reference_compare_base(rtype,op)\
	template<typename... Types,typename... Types2>\
	constexpr bool operator op(rtype<Types...> const& a,rtype<Types2...> const& b)\
	{\
		return a.base() op b.base();\
	}\
	template<typename... Types,typename... Types2>\
	constexpr bool operator op(std::tuple<Types...> const& a,rtype<Types2...> const& b)\
	{\
		return a op b.base();\
	}\
	template<typename... Types,typename... Types2>\
	constexpr bool operator op(rtype<Types...> const& a,std::tuple<Types2...> const& b)\
	{\
		return a.base() op b;\
	}
#define make_multi_reference_compare(op) make_multi_reference_compare_base(multi_reference,op)
#define make_multi_rvalue_reference_compare(op) make_multi_reference_compare_base(multi_rvalue_reference,op)

	EXLIB_FOR_ALL_COMP_OPS(make_multi_reference_compare)
	EXLIB_FOR_ALL_COMP_OPS(make_multi_rvalue_reference_compare)

#undef make_multi_reference_compare_base
#undef make_multi_rvalue_reference_compare
#undef make_multi_reference_compare

	namespace multi_reference_detail {
		template<typename T>
		constexpr T const& as_const(T const& a) noexcept
		{
			return a;
		}
		template<typename Ret,typename Tuple,std::size_t... I>
		constexpr Ret move_impl(Tuple&& tuple,exlib::index_sequence<I...>)
		{
			return Ret{std::move(std::get<I>(tuple))...};
		}
	}

	template<typename... References>
	constexpr multi_rvalue_reference<References...> move(multi_reference<References...> const& ref) noexcept
	{
		return multi_reference_detail::move_impl<multi_rvalue_reference<References...>>(ref.base(),make_index_sequence<sizeof...(References)>{});
	}
	template<typename... References>
	constexpr multi_rvalue_reference<References...> move(multi_reference<References...> const&& ref) noexcept
	{
		return exlib::move(multi_reference_detail::as_const(ref));
	}
	template<typename... References>
	constexpr multi_rvalue_reference<References...> move(multi_reference<References...>&& ref) noexcept
	{
		return exlib::move(multi_reference_detail::as_const(ref));
	}
	template<typename... References>
	constexpr multi_rvalue_reference<References...> move(multi_reference<References...>& ref) noexcept
	{
		return exlib::move(multi_reference_detail::as_const(ref));
	}

#define multi_reference_get(qualifier)\
	template<std::size_t I,typename... Types>\
	constexpr auto get(multi_reference<Types...> qualifier ref) -> decltype(std::get<I>(ref.base()))\
	{\
		return std::get<I>(ref.base());\
	}\
	template<std::size_t I,typename... Types>\
	constexpr auto get(multi_rvalue_reference<Types...> qualifier ref) -> decltype(std::move(std::get<I>(ref.base())))\
	{\
		return std::move(std::get<I>(ref.base()));\
	}

	multi_reference_get(&)
	multi_reference_get(&&)
	multi_reference_get(const&)
	multi_reference_get(const&&)

#undef multi_reference_get

	namespace combined_iterator_detail {
		template<typename RandIter,typename T>
		constexpr std::size_t find_index(RandIter iter,std::size_t len,T const& target)
		{
			for(std::size_t i=0;i<len;++i)
			{
				if(iter[i]==target) return i;
			}
			return len;
		}
		template<typename Iter,typename Iter2>
		constexpr auto has_diff(Iter const& it,Iter2 const& it2) -> decltype(it-it2,5);

		template<typename Iter,typename Iter2,typename... Extra>
		constexpr double has_diff(Iter const& it,Iter2 const& it2,Extra...);

		template<typename Iter,typename Iter2=Iter>
		constexpr auto has_diff()
		{
			return std::is_same<decltype(has_diff(std::declval<Iter>(),std::declval<Iter2>())),int>::value;
		}

		template<typename... Types,typename... Types2,std::size_t I>
		void swap_detail(multi_reference<Types...>& refs,std::tuple<Types2...>& vals,index_sequence<I>)
		{
			using std::get;
			using std::swap;
			swap(get<I>(refs),get<I>(vals));
		}
		template<typename... Types,typename... Types2,std::size_t I>
		void swap_detail(std::tuple<Types...>& vals,multi_reference<Types2...>& refs,index_sequence<I>)
		{
			using std::get;
			using std::swap;
			swap(get<I>(refs),get<I>(vals));
		}
		template<typename... Types,std::size_t I>
		void swap_detail(multi_reference<Types...>& vals,multi_reference<Types...>& refs,index_sequence<I>)
		{
			using std::get;
			using std::swap;
			swap(get<I>(refs),get<I>(vals));
		}
		template<typename Tuple,typename Tuple2,std::size_t I,std::size_t I2,::size_t... Is>
		void swap_detail(Tuple& refs,Tuple2& vals,index_sequence<I,I2,Is...>)
		{
			swap_detail(refs,vals,index_sequence<I>{});
			swap_detail(refs,vals,index_sequence<I2,Is...>{});
		}
		template<typename Iter,typename Diff>
		constexpr auto has_plus_equal(Iter const& it,Diff d) -> decltype(it+=d,5);

		template<typename Iter,typename Diff,typename... Extra>
		constexpr double has_plus_equal(Iter const& it,Diff,Extra...);

		template<typename Iter,typename Diff>
		constexpr bool has_plus_equal()
		{
			return std::is_same<int,decltype(has_plus_equal(std::declval<Iter>(),std::declval<Diff>()))>::value;
		}
	}

	template<typename... Types,typename... Types2>
	void swap(multi_reference<Types...>& refs,std::tuple<Types2...>& vals)
	{
		static_assert(sizeof...(Types)==sizeof...(Types2));
		combined_iterator_detail::swap_detail(refs,vals,make_index_sequence<sizeof...(Types)>{});
	}
	template<typename... Types,typename... Types2>
	void swap(std::tuple<Types...>& vals,multi_reference<Types2...>& refs)
	{
		static_assert(sizeof...(Types)==sizeof...(Types2));
		combined_iterator_detail::swap_detail(refs,vals,make_index_sequence<sizeof...(Types)>{});
	}
	template<typename... Types>
	void swap(multi_reference<Types...>& vals,multi_reference<Types...>& refs)
	{
		combined_iterator_detail::swap_detail(refs,vals,make_index_sequence<sizeof...(Types)>{});
	}

	template<typename... Types,typename... Types2>
	void swap(multi_reference<Types...>&& refs,std::tuple<Types2...>& vals)
	{
		static_assert(sizeof...(Types)==sizeof...(Types2));
		combined_iterator_detail::swap_detail(refs,vals,make_index_sequence<sizeof...(Types)>{});
	}
	template<typename... Types,typename... Types2>
	void swap(std::tuple<Types...>& vals,multi_reference<Types2...>&& refs)
	{
		static_assert(sizeof...(Types)==sizeof...(Types2));
		combined_iterator_detail::swap_detail(refs,vals,make_index_sequence<sizeof...(Types)>{});
	}
	template<typename... Types>
	void swap(multi_reference<Types...>&& vals,multi_reference<Types...>& refs)
	{
		combined_iterator_detail::swap_detail(refs,vals,make_index_sequence<sizeof...(Types)>{});
	}
	template<typename... Types>
	void swap(multi_reference<Types...>& vals,multi_reference<Types...>&& refs)
	{
		combined_iterator_detail::swap_detail(refs,vals,make_index_sequence<sizeof...(Types)>{});
	}
	template<typename... Types>
	void swap(multi_reference<Types...>&& vals,multi_reference<Types...>&& refs)
	{
		combined_iterator_detail::swap_detail(refs,vals,make_index_sequence<sizeof...(Types)>{});
	}

	template<typename... Iters>
	struct combined_iterator {
		std::tuple<Iters...> _iters;
		using full_seq=make_index_sequence<sizeof...(Iters)>;
	public:
		using value_type=std::tuple<typename std::iterator_traits<Iters>::value_type...>;
		using difference_type=typename std::common_type<typename std::iterator_traits<Iters>::difference_type...>::type;
		using reference=multi_reference<typename std::iterator_traits<Iters>::reference...>;
		using rvalue_reference=multi_rvalue_reference<typename std::iterator_traits<Iters>::reference...>;
		using pointer=iterator_detail::ptr_wrapper<reference>;
		using iterator_category=typename std::common_type<typename std::iterator_traits<Iters>::iterator_category...>::type;
		constexpr combined_iterator(Iters const&... iters) noexcept:_iters(iters...)
		{}
		constexpr combined_iterator(combined_iterator const&)=default;
		constexpr combined_iterator& operator=(combined_iterator const&)=default;
	private:
		template<std::size_t... I>
		constexpr reference ref_help(index_sequence<I...>) const
		{
			return reference(*std::get<I>(_iters)...);
		}
		template<std::size_t... I>
		constexpr auto subscript_help(index_sequence<I...>,std::size_t index) const -> decltype(reference(std::get<I>(_iters)[index]...))
		{
			return reference(std::get<I>(_iters)[index]...);
		}
		template<std::size_t I>
		constexpr void inc_help(index_sequence<I>)
		{
			++std::get<I>(_iters);
		}
		template<std::size_t I,std::size_t I2,::size_t... Is,typename... Extra>
		constexpr void inc_help(index_sequence<I,I2,Is...>,Extra...)
		{
			++std::get<I>(_iters),inc_help(index_sequence<I2,Is...>{});
		}
		template<std::size_t I>
		constexpr auto dec_help(index_sequence<I>) -> decltype(--std::get<I>(_iters),void())
		{
			--std::get<I>(_iters);
		}
		template<std::size_t I,std::size_t I2,::size_t... Is,typename... Extra>
		constexpr auto dec_help(index_sequence<I,I2,Is...>,Extra...) -> decltype(--std::get<I>(_iters),dec_help(index_sequence<I2,Is...>{}),void())
		{
			dec_help(index_sequence<I>{}),dec_help(index_sequence<I2,Is...>{});
		}
		template<std::size_t I>
		constexpr auto add_help(index_sequence<I>,difference_type n) -> decltype(std::get<I>(_iters)+=n,void())
		{
			std::get<I>(_iters)+=n;
		}
		template<std::size_t I,std::size_t I2,::size_t... Is,typename... Extra>
		constexpr auto add_help(index_sequence<I,I2,Is...>,difference_type n,Extra...) -> decltype((std::get<I>(_iters)+=n),add_help(index_sequence<I2,Is...>{},n),void())
		{
			add_help(index_sequence<I>{},n),add_help(index_sequence<I2,Is...>{},n);
		}
	public:
		constexpr std::tuple<Iters...> const& base() const
		{
			return _iters;
		}
		constexpr reference operator*() const noexcept(value_conjunction<noexcept(*std::declval<Iters>())...>::value)
		{
			return ref_help(full_seq{});
		}
		constexpr pointer operator->() const noexcept(noexcept(*std::declval<combined_iterator>()))
		{
			return {operator*()};
		}
		template<typename SizeT>
		constexpr auto operator[](SizeT n) const noexcept(value_conjunction<noexcept(std::declval<Iters>()[n])...>::value) -> decltype(subscript_help(full_seq{},n))
		{
			return subscript_help(full_seq{},n);
		}
		constexpr auto operator++() noexcept(value_conjunction<noexcept(++std::declval<Iters&>())...>::value) -> decltype(inc_help(full_seq{}),std::declval<combined_iterator&>())
		{
			inc_help(full_seq{});
			return *this;
		}
		constexpr auto operator++(int) noexcept(noexcept(operator++())&&std::is_nothrow_copy_constructible<combined_iterator>::value) -> typename std::remove_reference<decltype(++* this)>::type
		{
			auto copy(*this);
			operator++();
			return copy;
		}
		constexpr auto operator--() noexcept(value_conjunction<noexcept(--std::declval<Iters&>())...>::value) -> decltype(dec_help(full_seq{}),std::declval<combined_iterator&>())
		{
			dec_help(full_seq{});
			return *this;
		}
		constexpr auto operator--(int) noexcept(noexcept(operator--())&&std::is_nothrow_copy_constructible<combined_iterator>::value) -> typename std::remove_reference<decltype(++* this)>::type
		{
			auto copy(*this);
			operator--();
			return copy;
		}
		template<typename DifferenceType>
		constexpr auto operator+=(DifferenceType n) noexcept(value_conjunction<noexcept(std::declval<Iters>()+=n)...>::value) -> decltype(add_help(full_seq{},n),*this)
		{
			add_help(full_seq{},n);
			return *this;
		}
		template<typename DifferenceType>
		constexpr auto operator-=(DifferenceType n) noexcept(value_conjunction<noexcept(std::declval<Iters>()+=n)...>::value) -> decltype(*this+=n)
		{
			*this+=-n;
			return *this;
		}
#define make_comp_op_for_combined_iter(op,name)\
	private:\
		template<typename It,typename It2>\
		constexpr auto name(std::tuple<It,It2> iters) const -> decltype(std::get<0>(iters) op std::get<1>(iters))\
		{}\
		template<typename It,typename It2,typename It3,typename It4,typename... Its>\
		constexpr auto name(std::tuple<It,It2> it,std::tuple<It3,It4> it2,Its... iters) const -> decltype(name(it),name(it2,iters...))\
		{}\
	public:\
		template<typename... OIters>\
		constexpr auto operator op(combined_iterator<OIters...> const& other) const noexcept -> decltype(name(std::declval<std::tuple<Iters,OIters>>()...))\
		{\
			return std::get<0>(_iters) op std::get<0>(other._iters);\
		}
		make_comp_op_for_combined_iter(==,has_equal)
		make_comp_op_for_combined_iter(<,has_less)
		make_comp_op_for_combined_iter(>,has_greater)
		make_comp_op_for_combined_iter(!=,has_not_equal)
		make_comp_op_for_combined_iter(<=,has_less_equal)
		make_comp_op_for_combined_iter(>=,has_greater_equal)
#undef make_comp_op_for_combined_iter
	private:
	public:
	};

	namespace combined_iterator_detail {
		template<typename... Iters,std::size_t... I>
		constexpr auto plus_help(combined_iterator<Iters...> const& iter,index_sequence<I...>,typename combined_iterator<Iters...>::difference_type n) -> decltype(combined_iterator<Iters...>(std::get<I>(iter.base())+n...))
		{
			return combined_iterator<Iters...>(std::get<I>(iter.base())+n...);
		}
		template<typename... Iters>
		struct minus_help_str {
			template<std::size_t I>
			constexpr static auto has_diff() -> decltype(std::get<I>(std::declval<std::tuple<Iters...>>())-std::get<I>(std::declval<std::tuple<Iters...>>()),true)
			{
				return true;
			}
			template<std::size_t I,typename... Extra>
			constexpr static bool has_diff(Extra...)
			{
				return false;
			}
			template<std::size_t... Is>
			static constexpr std::array<bool,sizeof...(Is)> make_has_diffs(index_sequence<Is...>)
			{
				return {has_diff<Is>()...};
			}
			static constexpr auto has_diffs=make_has_diffs(make_index_sequence<sizeof...(Iters)>{});
			static constexpr std::size_t diff_index=combined_iterator_detail::find_index(has_diffs.begin(),sizeof...(Iters),true);
			static constexpr typename combined_iterator<Iters...>::difference_type minus_help(combined_iterator<Iters...> const& a,combined_iterator<Iters...> const& b)
			{
				return std::get<diff_index>(a.base())-std::get<diff_index>(b.base());
			}
		};
	}

	template<typename... OIters>
	constexpr auto operator-(combined_iterator<OIters...> const& a,combined_iterator<OIters...> const& b) noexcept -> typename std::enable_if<value_disjunction<combined_iterator_detail::has_diff<OIters>()...>::value,typename combined_iterator<OIters...>::difference_type>::type
	{
		return combined_iterator_detail::minus_help_str<OIters...>::minus_help(a,b);
	}
	template<typename... OIters>
	constexpr auto operator+(
		combined_iterator<OIters...> const& iter,
		typename combined_iterator<OIters...>::difference_type n)
		-> decltype(combined_iterator_detail::plus_help(iter,make_index_sequence<sizeof...(OIters)>{},n))
	{
		return combined_iterator_detail::plus_help(iter,make_index_sequence<sizeof...(OIters)>{},n);
	}
	template<typename DifferenceType,typename... OIters>
	constexpr auto operator+(DifferenceType n,combined_iterator<OIters...> const& iter) -> decltype(iter+n)
	{
		return iter+n;
	}
	template<typename... OIters>
	constexpr auto operator-(combined_iterator<OIters...> const& iter,typename combined_iterator<OIters...>::difference_type n) -> decltype(iter+n)
	{
		return iter+-n;
	}
	template<typename... Iters>
	constexpr combined_iterator<Iters...> make_combined_iterator(Iters const&... iters)
	{
		return {iters...};
	}
}
#endif