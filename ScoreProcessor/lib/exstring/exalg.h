#ifndef EXALG_H
#define EXALG_H
#include <utility>
#include <array>
#include <functional>
#if __cplusplus>201700L
#include <variant>
#endif
#include <exception>
namespace exlib {

	template<typename A>
	constexpr void swap(A& a,A& b)
	{
		auto temp=std::move(a);
		a=std::move(b);
		b=std::move(temp);
	}

	template<typename T>
	struct less:public std::less<T> {};

	template<>
	struct less<char const*> {
		constexpr bool operator()(char const* a,char const* b) const
		{
			for(size_t i=0;;++i)
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
					swap(*it,*(++pivot));
				}
			}
			swap(*begin,*pivot);
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

	//inserts the element AT elem into the range [begin,elem] according to comp assuming the range is sorted
	template<typename TwoWayIter,typename Comp>
	constexpr void insert_back(TwoWayIter const begin,TwoWayIter elem,Comp comp)
	{
		auto j=elem;
		while(j!=begin)
		{
			--j;
			if(comp(*elem,*j))
			{
				swap(*elem,*j);
				--elem;
			}
		}
	}

	template<typename TwoWayIter>
	constexpr void insert_back(TwoWayIter const begin,TwoWayIter elem)
	{
		using T=typename std::decay<decltype(*begin)>::type;
		insert_back(begin,elem,less<T>());
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
	template<typename T,size_t N,typename Comp>
	constexpr std::array<T,N> sorted(std::array<T,N> const& arr,Comp c)
	{
		auto sorted=arr;
		if constexpr(N<10) isort(sorted.begin(),sorted.end(),c);
		else qsort(sorted.begin(),sorted.end(),c);
		return sorted;
	}

	template<typename T,size_t N>
	constexpr std::array<T,N> sorted(std::array<T,N> const& arr)
	{
		return sorted(arr,less<T>());
	}

	namespace detail {
		template<typename T,size_t N,size_t M,size_t... I,size_t... J>
		constexpr auto concat(std::array<T,N> const& a,std::array<T,M> const& b,std::index_sequence<I...>,std::index_sequence<J...>)
		{
			std::array<T,N+M> ret{{a[I]...,b[J]...}};
			return ret;
		}
	}

	template<typename T,size_t N,size_t M>
	constexpr auto concat(std::array<T,N> const& a,std::array<T,M> const& b)
	{
		return detail::concat(a,b,std::make_index_sequence<N>(),std::make_index_sequence<M>());
	}

	template<typename A,typename B,typename... C>
	constexpr auto concat(A const& a,B const& b,C const&... c)
	{
		return concat(concat(a,b),c...);
	}

	template<typename T>
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
		constexpr int operator()(char const* a,char const* b) const
		{
			for(size_t i=0;;++i)
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
		return binary_find(begin,end,target,compare<typename std::decay_t<decltype(*begin)>>());
	}

	//converts three way comparison into a less than comparison
	template<typename ThreeWayComp>
	struct lt_comp:private ThreeWayComp {
		template<typename A,typename B>
		constexpr bool operator()(A const& a,B const& b) const
		{
			return ThreeWayComp::operator()(a,b)<0;
		}
	};

	//converts three way comparison into a greater than comparison
	template<typename ThreeWayComp>
	struct gt_comp:private ThreeWayComp {
		template<typename A,typename B>
		constexpr bool operator()(A const& a,B const& b) const
		{
			return ThreeWayComp::operator()(a,b)>0;
		}
	};

	//converts three way comparison into a less than or equal to comparison
	template<typename ThreeWayComp>
	struct le_comp:private ThreeWayComp {
		template<typename A,typename B>
		constexpr bool operator()(A const& a,B const& b) const
		{
			return ThreeWayComp::operator()(a,b)<=0;
		}
	};

	//converts three way comparison into a greater than or equal to comparison
	template<typename ThreeWayComp>
	struct ge_comp:private ThreeWayComp {
		template<typename A,typename B>
		constexpr bool operator()(A const& a,B const& b) const
		{
			return ThreeWayComp::operator()(a,b)>=0;
		}
	};

	//converts three way comparison into equality comparison
	template<typename ThreeWayComp>
	struct eq_comp:private ThreeWayComp {
		template<typename A,typename B>
		constexpr bool operator()(A const& a,B const& b) const
		{
			return ThreeWayComp::operator()(a,b)==0;
		}
	};

	//inverts three way comparison
	template<typename ThreeWayComp>
	struct inv_comp:private ThreeWayComp {
		template<typename A,typename B>
		constexpr int operator()(A const& a,B const& b) const
		{
			return ThreeWayComp::operator()(b,a);
		}
	};

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
		struct map_compare:protected Comp {
			template<typename Conv>
			constexpr int operator()(Conv const& target,map_pair<Key,Value> const& b) const
			{
				return Comp::operator()(target,b.key());
			}
			constexpr int operator()(map_pair<Key,Value> const& a,map_pair<Key,Value> const& b) const
			{
				return Comp::operator()(a.key(),b.key());
			}
		};
	}

	//Comp defines operator()(Key(&),Key(&)) that is a three-way comparison
	template<typename Key,typename Value,size_t entries,typename Comp=compare<Key>>
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
		constexpr ct_map(Args&&... rest):Data{{std::forward<Args>(rest)...}}
		{
			static_assert(sizeof...(Args)==entries,"Wrong number of entries");
			qsort(begin(),end(),lt_comp<key_compare>());
		}

	private:
		template<size_t... Is>
		constexpr ct_map(std::array<value_type,entries> const& in,std::index_sequence<Is...>):Data{{in[I]...}}
		{}
	public:
		constexpr ct_map(std::array<value_type,entries> const& in):ct_map(in,std::make_index_sequence<entries>())
		{}
		template<typename T>
		constexpr iterator find(T const& k)
		{
			return binary_find(begin(),end(),k,static_cast<key_compare>(*this));
		}
		template<typename T>
		constexpr const_iterator find(T const& k) const
		{
			return binary_find(begin(),end(),k,static_cast<key_compare>(*this));
		}
	};

	//inputs should be of type map_pair<Key,Value>
	template<typename Comp,typename First,typename... Rest>
	constexpr auto make_ct_map(First&& f,Rest&&... r)
	{
		return ct_map<First::key_type,First::mapped_type,1+sizeof...(r),Comp>(std::forward<First>(f),std::forward<Rest>(r)...);
	}

	//inputs should be of type map_pair<Key,Value>
	template<typename First,typename... T>
	constexpr auto make_ct_map(First&& k,T&&... rest)
	{
		return make_ct_map<compare<First::key_type>>(std::forward<First>(k),std::forward<T>(rest)...);
	}

	template<typename Comp,typename T,size_t N>
	constexpr auto make_ct_map(std::array<T,N> const& in)
	{
		return ct_map<T::key_type,T::mapped_type,N,Comp>(in);
	}

	template<typename T,size_t N>
	constexpr auto make_ct_map(std::array<T,N> const& in)
	{
		return make_ct_map<comp<First::key_type>>(in);
	}

	template<typename Type,typename... Args>
	constexpr auto make_array(Args&&... args)
	{
		return std::array<Type,sizeof...(Args)>{
			{
				std::forward<Args>(args)...
			}};
	}

	template<typename F,typename... Args>
	constexpr auto make_array(F&& arg,Args&&... args)
	{
		return std::array<std::decay<F>::type,sizeof...(args)+1> {
			{
				std::forward<F>(arg),std::forward<Args>(args)...
			}};
	}

	//the number of element accessible by std::get
	template<typename T>
	struct get_max {
	private:
		template<typename U>
		struct gm {
			constexpr static size_t const value=0;
		};

		template<typename... U>
		struct gm<std::tuple<U...>> {
			constexpr static size_t value=sizeof...(U);
		};

		template<typename U,size_t N>
		struct gm<std::array<U,N>> {
			constexpr static size_t value=N;
		};

#if __cplusplus>201700LL
		template<typename... U>
		struct gm<std::variant<U...>> {
			constexpr static size_t value=sizeof...(U);
		};
#endif

	public:
		constexpr static size_t const value=gm<std::remove_cv_t<std::remove_reference_t<T>>>::value;
	};

#if __cplusplus>201700L
	template<typename T>
	constexpr size_t get_max_v=get_max<T>::value;

	namespace detail {

		template<typename Ret,size_t I,typename Funcs,typename...Args>
		constexpr Ret apply_single(Funcs&& funcs,Args&&... args)
		{
			if constexpr(std::is_void_v<Ret>)
			{
				std::get<I>(std::forward<Funcs>(funcs))(std::forward<Args>(args)...);
			}
			else
			{
				return std::get<I>(std::forward<Funcs>(funcs))(std::forward<Args>(args)...);
			}
		}

		template<typename Ret,size_t... Is,typename Funcs,typename... Args>
		constexpr Ret apply_ind_jump_h(size_t i,std::index_sequence<Is...>,Funcs&& funcs,Args&&... args)
		{
			using Func=Ret(Funcs&&,Args&&...);
			static constexpr Func* jtable[]={&apply_single<Ret,Is,Funcs,Args...>...};
			return jtable[i](std::forward<Funcs>(funcs),std::forward<Args>(args)...);
		}

		template<typename Ret,size_t N,typename Funcs,typename... Args>
		constexpr Ret apply_ind_jump(size_t i,Funcs&& funcs,Args&&... args)
		{
			return apply_ind_jump_h<Ret>(i,std::make_index_sequence<N>(),std::forward<Funcs>(funcs),std::forward<Args>(args)...);
		}

		template<typename Ret,size_t I,size_t Max,typename Tuple,typename... Args>
		constexpr Ret apply_ind_linear_h(size_t i,Tuple&& funcs,Args&&... args)
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

		template<typename Ret,size_t NumFuncs,typename Tuple,typename... Args>
		constexpr Ret apply_ind_linear(size_t i,Tuple&& funcs,Args&&... args)
		{
			return apply_ind_linear_h<Ret,0,NumFuncs>(i,std::forward<Tuple>(funcs),std::forward<Args>(args)...);
		}

		template<typename Ret,size_t Lower,size_t Upper,typename Funcs,typename... Args>
		constexpr Ret apply_ind_bh(size_t i,Funcs&& funcs,Args&&... args)
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

		template<typename Ret,size_t NumFuncs,typename Funcs,typename... Args>
		constexpr Ret apply_ind_bsearch(size_t i,Funcs&& funcs,Args&&... args)
		{
			return apply_ind_bh<Ret,0,NumFuncs>(i,std::forward<Funcs>(funcs),std::forward<Args>(args)...);
		}
	}

	//Returns Ret(std::get<i>(std::forward<Funcs>(funcs))(std::forward<Args>(args)...)), Ret can be void
	//assuming i is less than NumFuncs, otherwise behavior is undefined (subject to change)
	//other overloads automatically determine Ret and NumFuncs if they are not supplied
	template<typename Ret,size_t NumFuncs,typename Funcs,typename... Args>
	constexpr auto apply_ind(size_t i,Funcs&& funcs,Args&&... args)
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

	template<size_t NumFuncs,typename Ret,typename Funcs,typename... Args>
	constexpr auto apply_ind(size_t i,Funcs&& funcs,Args&&... args)
	{
		return apply_ind<Ret,NumFuncs>(i,std::forward<Funcs>(funcs),std::forward<Args>(args)...);
	}

	template<size_t NumFuncs,typename Funcs,typename... Args>
	constexpr auto apply_ind(size_t i,Funcs&& funcs,Args&&... args)
	{
		if constexpr(NumFuncs==0)
		{
			return;
		}
		else
		{
			using Ret=decltype(std::get<0>(std::forward<Funcs>(funcs))(std::forward<Args>(args)...));
			return apply_ind<Ret,NumFuncs>(i,std::forward<Funcs>(funcs),std::forward<Args>(args)...);
		}
	}

	template<typename Ret,typename Funcs,typename... Args>
	constexpr auto apply_ind(size_t i,Funcs&& funcs,Args&&... args)
	{
		constexpr size_t N=get_max<std::remove_cv_t<std::remove_reference_t<Funcs>>>::value;
		return apply_ind<Ret,N>(i,std::forward<Funcs>(funcs),std::forward<Args>(args)...);
	}

	template<typename Funcs,typename... Args>
	constexpr auto apply_ind(size_t i,Funcs&& funcs,Args&&... args)
	{
		constexpr size_t N=get_max<std::remove_cv_t<std::remove_reference_t<Funcs>>>::value;
		return apply_ind<N>(i,std::forward<Funcs>(funcs),std::forward<Args>(args)...);
	}
#endif
	template<typename FindNext,typename... IndParser>
	class CSVParserBase:protected FindNext,protected std::tuple<IndParser...> {

	public:
		size_t parse(std::string_view s)
		{

		}
		size_t parse(char const*)
		{}
	};
}
#endif