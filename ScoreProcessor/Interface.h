#ifndef PROCESS_MAKERS_H
#define PROCESS_MAKERS_H
#include <tuple>
#include <vector>
#include <string>
#include <string_view>
#include <assert.h>
#include "parse.h"
#include <charconv>
#include <exception>
#include <unordered_map>
#include "ImageProcess.h"
#include "ImageUtils.h"
#include "lib/exstring/exmath.h"
#include "lib/exstring/exalg.h"
#include <numeric>
#include <functional>
#include <algorithm>
#include <string>
#include "ImageMath.h"
#include "ScoreProcesses.h"
#include "lib/exstring/exstring.h"
#include "Processes.h"
#include "Splice.h"
#define PMINLINE inline
namespace ScoreProcessor {

	using InputType=char const*;
	using iter=InputType const*;

	//Abstract class responsible for taking in iter and modifying delivery based on the input
	class CommandMaker {
	public:
		//information about what ScoreProcessor will do
		struct delivery {
			using pv=exlib::maybe_fixed<unsigned int>;
			SaveRules sr; //output template
			unsigned int starting_index; //index to number files by
			bool do_move; //whether files should be moved or copied
			ProcessList<unsigned char> pl; //list of processes to perform on images
			enum do_state { //what program is doing
				do_absolutely_nothing, //program does nothing
				do_nothing, //program does no image process, but may list files or move/copy files
				do_single, //program does processes that apply to single images
				do_cut, //program cuts the images
				do_splice //program splices images together
			};
			do_state flag;
			enum log_type {
				unassigned_log,
				quiet, //no output
				errors_only,
				count, //count of files completed
				full_message //name of files started and completed output
			};
			log_type lt;
			Splice::standard_heuristics splice_args; //args for splicing
			cil::CImg<unsigned char> splice_divider;
			struct {
				pv min_height,min_width,min_vert_space;
				unsigned char background;
				float horiz_weight;
			} cut_args; //args for cutting
			struct filter { //information about what files to process and which to filter out
				std::regex rgx;
				bool keep_match;
			};
			std::vector<filter> rgxes; //the filters to apply to file input list
			struct sel_boundary { //files between begin and end in the input list are kept
				std::string_view begin; //will always be present in the parameters list
				std::string_view end;
			};
			std::vector<sel_boundary> selections;
			unsigned int num_threads; //num threads to use
			//some processes (like SmartScale) may need multithreading in one image 
			//and thus override the across image thread count
			unsigned int overridden_num_threads;
			bool list_files; //whether files should be listed out to the user
			int quality; //[0,100] jpeg file quality
			PMINLINE delivery():
				starting_index(-1), //invalid values means not given by user
				flag(do_absolutely_nothing),
				num_threads(0),
				overridden_num_threads(0),
				do_move(false),
				list_files(false),
				lt(unassigned_log),
				quality(-1)
			{}
			//assigns the default value of num threads if not assigned
			//num_threads is limited by num_files if the thread_count has not been overridden by a process
			void fix_values(size_t num_files)
			{
				if(num_threads==0)
				{
					num_threads=std::thread::hardware_concurrency();
					if(num_threads==0)
					{
						num_threads=2;
					}
				}
				if(starting_index==-1)
				{
					starting_index=1;
				}
				if(overridden_num_threads)
				{
					overridden_num_threads=num_threads;
					num_threads=1;
				}
				else
				{
					using ui=decltype(num_threads);
					num_threads=std::min(
						num_threads,
						static_cast<ui>((std::min<size_t>(std::numeric_limits<ui>::max(),num_files))));
				}
				if(quality==-1)
				{
					quality=100;
				}
			}
		};
	private:
		std::string_view _help_message;
		std::string_view _name;
		std::string_view _args;
	public:
		//COMPILE-TYPE CONSTANT STRINGS REQUIRED
		//help: help message of CommandMaker
		//name: name of CommandMaker
		//args: arguments that the CommandMaker expects to take
		template<typename SV1,typename SV2,typename SV3>
		CommandMaker(SV1& help,SV2& name,SV3& args):_help_message(help),_name(name),_args(args)
		{}
		PMINLINE std::string_view help_message() const
		{
			return _help_message;
		}
		PMINLINE std::string_view name() const
		{
			return _name;
		}
		PMINLINE std::string_view argument_list() const
		{
			return _args;
		}
		//based on the inputs between [begin,end), modifies del according
		virtual void make_command(iter begin,iter end,delivery& del)=0;
	};

	struct empty {};

	struct empty2 {};

	struct empty3 {};

	struct no_check {
		template<typename T>
		static constexpr void check(T)
		{}
	};

	//has check to assert that given values are non-negative
	struct no_negatives {
		template<typename T>
		static void check(T val)
		{
			if(val<0)
			{
				throw std::invalid_argument("Value must be non-negative");
			}
		}
	};

	//has check to assert that given values are positive
	struct force_positive {
		template<typename T>
		static void check(T n)
		{
			if(n<=0)
			{
				throw std::invalid_argument("Value must be positive");
			}
		}
	};

	//A float parser that takes the properties of Base
	//and checks its input using Check::check
	template<typename Base,typename Check=no_negatives>
	struct FloatParser:public Base,private Check {
		float parse(char const* sv) const
		{
			float f;
			char* end;
			int& err=errno;
			err=0;
			f=std::strtof(sv,&end);
			auto find_bad=[=]()
			{
				if(sv==end)
				{
					return true;
				}
				for(auto it=end;;++it)
				{
					if(*it=='\0')
					{
						return false;
					}
					if(*it!=' ')
					{
						return true;
					}
				}
			};
			if(err||find_bad())
			{
				throw std::invalid_argument("Invalid input");
			}
			Check::check(f);
			return f;
		}
	};

	//A double parser that takes the properties of Base
	//and checks its input using Check::check
	template<typename Base,typename Check=no_negatives>
	struct DoubleParser:public Base,private Check {
		double parse(char const* sv) const
		{
			double f;
			char* end;
			int& err=errno;
			err=0;
			f=std::strtod(sv,&end);
			auto find_bad=[=]()
			{
				if(sv==end) return true;
				for(auto it=end;;++it)
				{
					if(*it=='\0')
					{
						return false;
					}
					if(*it!=' ')
					{
						return true;
					}
				}
			};
			if(err||find_bad())
			{
				throw std::invalid_argument("Invalid input");
			}
			Check::check(f);
			return f;
		}

	};

	//An integer parser that takes the properties of Base
	//and checks its input using Check::check
	template<typename T,typename Base,typename Check=no_check>
	struct IntegerParser:public Base,private Check {
	public:
		T parse(char const* in) const
		{
			T t;
			exlib::conv_res res=exlib::parse(in,t);
			if(res.ce==exlib::conv_error::invalid_characters)
			{
				throw std::invalid_argument("Invalid argument");
			}
			if(res.ce==exlib::conv_error::out_of_range)
			{
				static constexpr auto err_msg=exlib::str_concat(
					"Argument must be in range [",
					exlib::to_string<std::numeric_limits<T>::min()>(),
					",",
					exlib::to_string<std::numeric_limits<T>::max()>(),
					"]"
				);
				throw std::invalid_argument(err_msg.data());
			}
			for(auto it=res.last;;++it)
			{
				auto const c=*it;
				if(c=='\0')
				{
					break;
				}
				if(c!=' ')
				{
					throw std::invalid_argument("Invalid trailing characters");
				}
			}
			Check::check(t);
			return t;
		}
	};

	template<typename Base,typename Check=no_check>
	using IntParser=IntegerParser<int,Base,Check>;

	template<typename Base,typename Check=no_check>
	using UIntParser=IntegerParser<unsigned int,Base,Check>;

	template<typename Base,typename Check=no_check>
	using UCharParser=IntegerParser<unsigned char,Base,Check>;

	//parses a float and ensures it is between 0 and 1
	//and allows % at the end to input percentages
	//n: object to use get name from to throw an appropriate error
	inline float parse01(char const* cp)
	{
		float v;
		auto res=exlib::parse(cp,v);
		if(res.ce==exlib::conv_error::invalid_characters)
		{
			throw std::invalid_argument("Invalid characters");
		}
		if(res.ce==exlib::conv_error::out_of_range)
		{
			throw std::invalid_argument("Argument out of range");
		}
		if(*res.last=='%')
		{
			v/=100;
			++res.last;
		}
		while(true)
		{
			if(*res.last=='\0') break;
			if(*res.last!=' ') throw std::invalid_argument("Invalid argument");
			++res.last;
		}
		if(v<0||v>1)
		{
			throw std::invalid_argument("Argument must be in range [0,1]");
		}
		return v;
	}

	namespace {
		//throws the error involving an unknown prefix
		//sv: pointer to prefix 
		//len: length of prefix
		[[noreturn]]
		PMINLINE void bad_pf(InputType sv,size_t len)
		{
			std::string err_msg("Unknown prefix ");
			err_msg.append(sv,len);
			throw std::invalid_argument(err_msg);
		}

		//class defining a lookup prefix and its associated parser index
		class lookup_entry {
			char const* _key;
			size_t _index;
		public:
			PMINLINE constexpr lookup_entry(char const* key,size_t index):_key(key),_index(index)
			{}
			PMINLINE constexpr char const* key() const
			{
				return _key;
			}
			PMINLINE constexpr size_t index() const
			{
				return _index;
			}
		};

		using le=lookup_entry;

		template<size_t N>
		using ltable=std::array<lookup_entry,N>;

		struct lcomp {
			PMINLINE constexpr int operator()(InputType sv,lookup_entry le) const
			{
				for(size_t i=0;;++i)
				{
					if(sv[i]==':')
					{
						return le.key()[i]==0?0:-1;
					}
					if(le.key()[i]==0)
					{
						return 1;
					}
					if(sv[i]<le.key()[i])
					{
						return -1;
					}
					if(le.key()[i]<sv[i])
					{
						return 1;
					}
				}
			}
			PMINLINE constexpr bool operator()(lookup_entry a,lookup_entry b) const
			{
				return exlib::less<char const*>()(a.key(),b.key());
			}
		};

		//makes a sorted table of lookup entries to be searched through
		template<size_t N>
		constexpr auto make_ltable(ltable<N> const& arr)
		{
			return exlib::sorted(arr,lcomp());
		}

		//makes a sorted table of lookup entries to be searched through
		template<typename... Args>
		constexpr auto make_ltable(Args... args)
		{
			return make_ltable(ltable<sizeof...(args)>{
				{
					args...
				}});
		}

		template<size_t N>
		constexpr auto lfind(ltable<N> const& arr,InputType target)
		{
			return exlib::binary_find(arr.data(),arr.data()+arr.size(),target,lcomp());
		}

		//given a lookup table, does a binary search for the given prefix; throws an error if it is not found
		//arr: lookup_table
		//pf: prefix
		//pflen: prefix length
		template<size_t N>
		size_t lookup_prefix(ltable<N> const& arr,InputType pf,size_t pflen)
		{
			auto idx=lfind(arr,pf);
			if(idx!=arr.data()+arr.size())
			{
				return idx->index();
			}
			bad_pf(pf,pflen);
		}
	}

	//otherwise, the parser will just go through arguments in order
	//Each type of ArgParsers must define 
	//  - a function Type parse(char const*,(optional)size_t prefix_len)
	//  - a static constexpr member name that is convertible to std::string_view, preferably char const*
	//  - optionally a function (constexpr) ConvertibleToType def_val() that returns the default value
	//  - optionally a static constexpr member char (const)* labels[] that identifies labels for this type
	//If parse accepts a second real_start argument, the first arg is of the whole input and prefix_len
	//is the length of the prefix not including the colon separator. If there is no prefix, prefix_len is -1. 
	//Otherwise the first string is only a view past the colon.
	//UseTuple must define a function use_tuple(CommandMaker::delivery&,Args...), which uses the tuple result
	//from parsing to change the delivery. The tuple may optionally be referenced directly.
	//The tuple contains the types return by parse.
	//if Precheck defines check(CommandMaker::delivery (const)(&),(optional)size_t num_args), this checks the delivery before parsing,
	//possibly throwing, modifying delivery, or doing nothing
	//How this works:
	// First, Precheck checks the input delivery and/or number of args.
	// Then, the number of args is checked to be between MinArgs and MaxArgs.
	//   MinArgs is found to be the first N ArgParsers which do not define def_val
	//   MaxArgs is equal to sizeof...(ArgParsers)
	//  If not every ArgParser defines labels, then the arguments are simply evaluated in order.
	//  Otherwise, the arguments are evaluated in order until an argument with a prefix is found
	//  After this prefixed arguments only are considered (if a prefix is not found now, an exception is thrown).
	//  If a argument is given repeatedly an exception is thrown, or if an argument with no default value is unfulfilled,
	//  an exception is thrown.
	//  The delivery and the parsed arguments are then supplied to UseTuple::use_tuple(...)
	template<typename UseTuple,typename Precheck,typename... ArgParsers>
	class MakerTFull:protected Precheck,protected UseTuple,protected std::tuple<ArgParsers...>,public CommandMaker {
	public:
		using CommandMaker::CommandMaker;
	private:
		template<typename T>
		struct has_def_val {
		private:
			template<typename U>
			constexpr static auto val(int) -> decltype(std::declval<U>().def_val(),bool())
			{
				return true;
			}
			template<typename>
			constexpr static bool val(...)
			{
				return false;
			}
		public:
			constexpr static bool const value=val<T>(0);
		};

		template<typename...>
		struct count_helper;

		template<typename A,typename... Rest>
		struct count_helper<A,Rest...> {
			typedef A type;
			typedef count_helper<Rest...> rest;
		};

		template<>
		struct count_helper<> {};

		template<typename A>
		constexpr static size_t _count()
		{
			if constexpr(!has_def_val<A::type>::value)
			{
				return 1+_count<A::rest>();
			}
			return 0;
		}

		template<>
		constexpr static size_t _count<count_helper<>>()
		{
			return 0;
		}

		template<typename... A>
		constexpr static size_t defaults()
		{
			return _count<count_helper<A...>>();
		}

		template<typename U>
		constexpr static auto check_parse(int) -> decltype(std::declval<U>().parse(InputType()));
		template<typename U>
		constexpr static auto check_parse(int) -> decltype(std::declval<U>().parse(InputType(),size_t()));
		template<typename>
		constexpr static bool check_parse(...)
		{
			static_assert(false,"Failed to find parse function");
		}

		template<typename T>
		struct accepts_prefix {
		private:
			template<typename U>
			constexpr static auto val(int) -> decltype(std::declval<U>().parse(InputType(),size_t()),bool())
			{
				return true;
			}
			template<typename>
			constexpr static bool val(...)
			{
				return false;
			}
		public:
			constexpr static bool const value=val<T>(0);
		};

		struct uses_tuple {
		private:
			template<typename U>
			constexpr static auto val(int) ->
				decltype(std::declval<U>().use_tuple(std::declval<CommandMaker::delivery>(),std::declval<MyArgs>()),bool())
			{
				return true;
			}
			template<typename>
			constexpr static bool val(...)
			{
				return false;
			}
		public:
			constexpr static bool const value=val<UseTuple>(0);
		};

		struct has_precheck {
		private:
			template<typename U>
			constexpr static auto val(int) ->
				decltype(std::declval<U>().check(std::declval<CommandMaker::delivery>()),int())
			{
				return 1;
			}
			template<typename U>
			constexpr static auto val(int) ->
				decltype(std::declval<U>().check(std::declval<CommandMaker::delivery>(),size_t()),int())
			{
				return 2;
			}
			template<typename>
			constexpr static bool val(...)
			{
				return 0;
			}
		public:
			constexpr static int const value=val<Precheck>(0);
		};

		template<typename T>
		struct has_labels {
		private:
			template<typename U>
			constexpr static auto concattable() -> decltype(exlib::concat(U::labels,std::array<char const*,1>{}),bool())
			{
				return true;
			}
			template<typename U,typename... Extra>
			constexpr static auto concattable(Extra...)
			{
				return false;
			}
		public:
			static constexpr bool value=concattable<T>();
		};
	public:
		constexpr static size_t MinArgs=defaults<ArgParsers...>();
		constexpr static size_t MaxArgs=sizeof...(ArgParsers);
		constexpr static bool enable_labels=sizeof...(ArgParsers)>0&&std::conjunction<has_labels<ArgParsers>...>::value;
	protected:
		using MyParsers=std::tuple<ArgParsers...>;
		using MyArgs=std::tuple<decltype(check_parse<ArgParsers>(0))...>;
		static_assert(std::tuple_size<MyParsers>::value==MaxArgs);
		static_assert(std::tuple_size<MyArgs>::value==MaxArgs);
	private:
		constexpr MyParsers& as_parsers()
		{
			return static_cast<MyParsers&>(*this);
		}

		static void check_size(size_t n)
		{
			if(n>MaxArgs)
			{
				throw std::invalid_argument("Too many arguments");
			}
			if(n<MinArgs)
			{
				throw std::invalid_argument("Too few arguments");
			}
		}

		template<size_t I>
		struct parse_n {
			using Parser=std::remove_reference_t<decltype(std::get<I>(std::declval<MyParsers>()))>;
			[[noreturn]]
			static void rethrow_with_name(std::exception const& ex)
			{
				std::string err_msg(Parser::name);
				err_msg.append(": ");
				err_msg.append(ex.what());
				throw std::invalid_argument(err_msg);
			}
			static void eval_arg(MyArgs& ma,MyParsers& mp,InputType str)
			{
				try
				{
					std::get<I>(ma)=std::get<I>(mp).parse(str);
				}
				catch(std::exception const& ex)
				{
					rethrow_with_name(ex);
				}
			}
			static void eval_arg(MyArgs& ma,MyParsers& mp,InputType str,size_t prefix_len)
			{
				try
				{
					if constexpr(accepts_prefix<Parser>::value)
					{
						std::get<I>(ma)=std::get<I>(mp).parse(str,prefix_len);
					}
					else
					{
						std::get<I>(ma)=std::get<I>(mp).parse(str+prefix_len+1);
					}
				}
				catch(std::exception const& ex)
				{
					rethrow_with_name(ex);
				}
			}
			void operator()(MyArgs& ma,MyParsers& mp,InputType str,size_t prefix_len) const
			{
				eval_arg(ma,mp,str,prefix_len);
			}
		};

		template<size_t... Is>
		static constexpr auto make_parse_table(std::index_sequence<Is...>)
		{
			return std::make_tuple(parse_n<Is>()...);
		}

		void find_arg(size_t i,MyArgs& mt,InputType str,size_t prefix_len)
		{
			constexpr static auto parse_table=make_parse_table(std::make_index_sequence<MaxArgs>());
			exlib::apply_ind(i,parse_table,mt,as_parsers(),str,prefix_len);
		}

		constexpr static auto name_table=std::array<std::string_view,MaxArgs>{{ArgParsers::name...}};

		static constexpr std::string_view find_arg_name(size_t i)
		{
			return name_table[i];
		}

	public:

		//returns the length of the prefix not including the colon, or -1 if there is no prefix
		constexpr static size_t find_prefix(InputType str)
		{
			for(size_t i=0;;++i)
			{
				if(str[i]=='\0')
				{
					return -1;
				}
				if(str[i]==':')
				{
					return i;
				}
			}
		}

	private:

		template<typename Parser>
		static constexpr size_t num_labels(){
			return exlib::array_size<decltype(Parser::labels)>::value;
		}

		template<typename Parser,size_t I,size_t... Is>
		static constexpr auto make_lookup_entries(std::index_sequence<Is...>)
		{
			return std::array<lookup_entry,sizeof...(Is)>{{le(Parser::labels[Is],I)...}};
		}
		
		template<typename... Parsers,size_t... Indices>
		static constexpr auto make_lookup_table(std::index_sequence<Indices...>)
		{
			return exlib::concat(make_lookup_entries<Parsers,Indices>(std::make_index_sequence<num_labels<Parsers>()>{})...);
		}

		size_t find_label(InputType data,size_t len)
		{
			static constexpr auto lookup_table=
				make_ltable(make_lookup_table<ArgParsers...>(std::make_index_sequence<sizeof...(ArgParsers)>()));
			return lookup_prefix(lookup_table,data,len);
		}

		template<size_t N>
		void ordered_args(iter begin,size_t n,MyArgs& mt)
		{
			if constexpr(N<MaxArgs)
			{
				if(N<n)
				{
					parse_n<N>::eval_arg(mt,as_parsers(),begin[N]);
					ordered_args<N+1>(begin,n,mt);
				}
			}
		}

		void throw_if_repeat(size_t index,bool fulfilled[])
		{
			if(fulfilled[index])
			{
				std::string err_msg("Argument already given for ");
				err_msg.append(find_arg_name(index));
				throw std::invalid_argument(err_msg);
			}
			fulfilled[index]=true;
		}

		void eval_tagged(InputType data,size_t prefix_len,MyArgs& mt,bool fulfilled[])
		{
			auto const index=find_label(data,prefix_len);
			throw_if_repeat(index,fulfilled);
			find_arg(index,mt,data,prefix_len);
		}

		template<size_t N=0>
		void unordered_args(iter begin,size_t n,MyArgs& mt,bool fulfilled[])
		{
			if constexpr(N<MaxArgs)
			{
				if(N<n)
				{
					InputType const sv=begin[N];
					auto const prefix=find_prefix(sv);
					if(prefix==-1)
					{
						parse_n<N>::eval_arg(mt,as_parsers(),sv,prefix);
						fulfilled[N]=true;
						unordered_args<N+1>(begin,n,mt,fulfilled);
					}
					else
					{
						eval_tagged(sv,prefix,mt,fulfilled);
						constexpr size_t const M=N+1;
						unordered_args_rest(begin+M,n-M,mt,fulfilled);
					}
				}
			}
		}

		void unordered_args_rest(iter begin,size_t n,MyArgs& mt,bool fulfilled[])
		{
			for(size_t i=0;i<n;++i)
			{
				auto const pf=find_prefix(begin[i]);
				if(pf==-1)
				{
					throw std::invalid_argument("Unlabeled arguments used after labeled arguments");
				}
				eval_tagged(begin[i],pf,mt,fulfilled);
			}
		}

		void check_required(bool fulfilled[])
		{
			for(size_t i=0;i<MinArgs;++i)
			{
				if(!fulfilled[i])
				{
					std::string err_msg("Missing input for ");
					err_msg.append(find_arg_name(i));
					throw std::invalid_argument(err_msg);
				}
			}
		}

		template<size_t I>
		constexpr auto get_def_val(int) -> decltype(std::get<I>(std::declval<MyParsers>()).def_val())
		{
			return std::get<I>(as_parsers()).def_val();
		}
		template<size_t I>
		constexpr auto get_def_val(...)
		{
			return decltype(check_parse<std::remove_reference<decltype(std::get<I>(std::declval<MyParsers>()))>::type>(0))();
		}
		template<size_t... Is>
		constexpr MyArgs init_args_h(std::index_sequence<Is...>)
		{
			return MyArgs{get_def_val<Is>(0)...};
		}
		constexpr MyArgs init_args()
		{
			return init_args_h(std::make_index_sequence<MaxArgs>());
		}

	public:
		void make_command(iter begin,iter end,delivery& del) override
		{
			if constexpr(has_precheck::value==1)
			{
				Precheck::check(del);
			}
			size_t n=end-begin;
			if constexpr(has_precheck::value==2)
			{
				Precheck::check(del,n);
			}
			check_size(n);
			auto mt=init_args();
			if constexpr(MaxArgs>0)
			{
				if constexpr(enable_labels)
				{
					bool fulfilled[MaxArgs]{false};
					unordered_args(begin,n,mt,fulfilled);
					check_required(fulfilled);
				}
				else
				{
					ordered_args<0>(begin,n,mt);
				}
			}
			if constexpr(uses_tuple::value)
			{
				UseTuple::use_tuple(del,std::move(mt));
			}
			else
			{
				std::apply(
					[&del,this](auto&&... args)
				{
					UseTuple::use_tuple(del,std::forward<decltype(args)>(args)...);
				},std::move(mt));
			}
		}
	};

	struct SingleCheck {
		PMINLINE static void check(CommandMaker::delivery& del)
		{
			if(del.flag>del.do_single)
			{
				throw std::invalid_argument("Single operations cannot be performed at the same time as multi operations");
			}
			del.flag=del.do_single;
		}
	};

	template<CommandMaker::delivery::do_state state>
	struct MultiCommand {
		static_assert(state>CommandMaker::delivery::do_state::do_single,"Invalid multistate");
		static PMINLINE void check(CommandMaker::delivery& del)
		{
			if(del.flag==del.do_single)
			{
				throw std::invalid_argument("Single operations cannot be performed at the same time as multi operations");
			}
			del.flag=state;
		}
	};

	template<typename UseTuple,typename... ArgParsers>
	using SingMaker=MakerTFull<UseTuple,SingleCheck,ArgParsers...>;

#define cnnm(n) static constexpr char const* name=n
#define clbl(...) static constexpr char const* labels[]={__VA_ARGS__}
#define cndf(n) static PMINLINE constexpr auto def_val() { return (n); }
#define ncdf(n) static PMINLINE auto def_val() { return (n); }

	namespace Output {

		struct PatternParser {
			static PMINLINE constexpr InputType parse(InputType s)
			{
				if(s[0]=='-'&&s[1]=='-')
				{
					return s+1;
				}
				return s;
			}
			clbl("p","pat","o","out");
			cnnm("pattern");
			cndf("%w")
		};

		struct MoveParser {
			static PMINLINE constexpr bool parse(InputType s)
			{
				auto const c=s[0];
				if(c=='t'||c=='1'||c=='T')
				{
					return true;
				}
				return false;
			}
			clbl("mv","move","m");
			cnnm("move");
			cndf(false)

		};

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,InputType input,bool do_move)
			{
				if(*input==0)
				{
					throw std::invalid_argument("Output format cannot be empty");
				}
				if(del.flag<del.do_nothing)
				{
					del.flag=del.do_nothing;
				}
				del.sr.assign(input);
				del.do_move=do_move;
			}
		};

		struct Precheck {
			static PMINLINE void check(CommandMaker::delivery& del,size_t num_args)
			{
				if(!del.sr.empty())
				{
					throw std::invalid_argument("Output format already given");
				}
				if(num_args==0)
				{
					throw std::invalid_argument("No arguments given");
				}
			}
		};

		extern MakerTFull<UseTuple,Precheck,PatternParser,MoveParser> maker;
	}

	namespace NumThreads {
		struct Precheck {
			PMINLINE static void check(CommandMaker::delivery& del)
			{
				if(del.num_threads!=0)
				{
					throw std::invalid_argument("Thread count already given");
				}
			}
		};

		struct Name {
			cnnm("number of threads");
		};
		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,unsigned int nt)
			{
				del.num_threads=nt;
			}
		};
		extern MakerTFull<UseTuple,Precheck,IntegerParser<unsigned int,Name,force_positive>> maker;
	}

	namespace Verbosity {
		struct Precheck {
			static PMINLINE void check(CommandMaker::delivery const& del)
			{
				if(del.lt!=del.unassigned_log)
				{
					throw std::invalid_argument("Log already assigned");
				}
			}
		};
		struct Level {
			cnnm("level");
			static PMINLINE auto parse(char const* sv)
			{
				auto error=[=]()
				{
					std::string err_msg("Invalid level ");
					err_msg.append(sv);
					throw std::invalid_argument(err_msg);
				};
				switch(sv[0])
				{
					case '0':
					case 's':
					case 'S':
						return CommandMaker::delivery::quiet;
					case '1':
					case 'e':
					case 'E':
						return CommandMaker::delivery::errors_only;
					case '2':
					case 'c':
					case 'C':
						return CommandMaker::delivery::count;
					case '3':
					case 'l':
					case 'L':
						return CommandMaker::delivery::full_message;
					default:
						error();
				}
			}
		};
		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,CommandMaker::delivery::log_type level)
			{
				del.lt=level;
			}
		};
		extern
			MakerTFull<UseTuple,Precheck,Level> maker;
	}

	namespace StrMaker {
		struct MinAngle {
			cnnm("min angle");
			clbl("min","mn","mna");
			cndf(double(-5))
		};
		struct MaxAngle {
			cnnm("max angle");
			clbl("max","mx","mxa");
			cndf(double(5))
		};
		struct AnglePrec {
			cnnm("angle precision");
			clbl("a","ap");
			cndf(double(0.1))
		};
		struct PixelPrec {
			cnnm("pixel precision");
			clbl("p","pp");
			cndf(double(1))
		};
		struct Boundary {
			cnnm("boundary");
			clbl("b");
			cndf(unsigned char(128))
		};
		struct Gamma {
			cnnm("gamma");
			clbl("g","gam");
			cndf(float(2))
		};

		struct UseHoriz {
			cnnm("use horiz");
			clbl("h","horiz","v","vert","vertical","horizontal");
			cndf(true)
			static bool parse(char const* in,size_t prefix_len)
			{
				auto const actual=in+prefix_len+1;
				if(prefix_len==-1||in[0]=='h')
				{
					return actual[0]=='t';
				}
				else
				{
					return actual[0]!='t';
				}
			}
		};

		struct UseTuple {
			PMINLINE static void use_tuple(CommandMaker::delivery& del,double mn,double mx,double a,double p,unsigned char b,float g,bool use_horiz)
			{
				if(mn>=mx)
				{
					throw std::invalid_argument("Min angle must be less than max angle");
				}
				if(mx-mn>180)
				{
					throw std::invalid_argument("Difference between angles must be less than or equal to 180");
				}
				del.pl.add_process<Straighten>(p,mn,mx,a,b,g,use_horiz);
			}
		};

		using GammaParser=FloatParser<Gamma,no_negatives>;

		extern
			SingMaker<UseTuple,
			DoubleParser<MinAngle,no_check>,DoubleParser<MaxAngle,no_check>,
			DoubleParser<AnglePrec>,DoubleParser<PixelPrec>,
			IntegerParser<unsigned char,Boundary>,GammaParser,UseHoriz>
			maker;
	}

	namespace CGMaker {

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del)
			{
				del.pl.add_process<ChangeToGrayscale>();
			}
		};
		extern SingMaker<UseTuple> maker;
	}

	namespace FRMaker {

		struct Left {
			clbl("left","l");
			cnnm("left");
		};
		struct Top {
			clbl("t","top");
			cnnm("top");
		};
		struct flagged {
			bool flag=false;
			signed int value;
		};
		struct Right {
			cnnm("horizontal extent");
			clbl("r","right","w","width");
			struct Coord {
				cnnm("right");
			};
			struct Width {
				cnnm("width");
			};
			PMINLINE static flagged parse(char const* sv,size_t prefix)
			{
				char const* to_parse=sv+prefix+1;
				if(prefix==-1||sv[0]=='r')
				{
					return {false,IntegerParser<int,Coord,no_check>().parse(to_parse)};
				}
				return {true,IntegerParser<int,Width,no_negatives>().parse(to_parse)};
			}
		};

		struct Bottom {
			cnnm("vertical extent");
			clbl("b","bot","bottom","h","height");
			struct Coord {
				cnnm("bottom");
			};
			struct Height {
				cnnm("height");
			};
			PMINLINE static flagged parse(char const* sv,size_t prefix)
			{
				char const* to_parse=sv+prefix+1;
				if(prefix==-1||sv[0]=='b')
				{
					return {false,IntegerParser<int,Coord,no_check>().parse(to_parse)};
				}
				return {true,IntegerParser<int,Height,no_negatives>().parse(to_parse)};
			}
		};

		struct color {
			unsigned int num_layers;
			std::array<unsigned char,4> data;
			PMINLINE constexpr color():num_layers(5),data{{255,255,255,255}}
			{}
		};

		struct Color {
			cnnm("color");
			clbl("c","clr","color");
			PMINLINE static color parse(char const* data)
			{
				auto error=[](auto res,auto name)
				{
					if(res.ec==std::errc::invalid_argument)
					{
						std::string err_msg("Invalid argument for ");
						err_msg.append(name);
						throw std::invalid_argument(err_msg);
					}
					if(res.ec==std::errc::result_out_of_range)
					{
						throw std::invalid_argument("Values must be in range [0,255]");
					}
				};
				auto find_invalids=[](char const* data,char const* end)
				{
					for(auto c=data;c<end;++c)
					{
						if(*c!=' ')
						{
							std::string err_msg("Invalid characters found: ");
							err_msg.append(c,end-c);
							throw std::invalid_argument(err_msg);
						}
					}
				};

				color clr;
				std::string_view sv(data);
				if(sv[0]=='#')
				{
					clr.num_layers=3;
					if(sv.length()!=7) throw std::invalid_argument("Invalid color argument");
					auto conv=[](char c)
					{
						if(c>='a'&&c<='z')
						{
							return unsigned char(c-'a'+10);
						}
						if(c>='A'&&c<='Z')
						{
							return unsigned char(c-'A'+10);
						}
						if(c>='0'&&c<='9')
						{
							return unsigned char(c-'0');
						}
						throw std::invalid_argument("Invalid color argument");
					};
					auto calc=[=](unsigned char& val,size_t idx)
					{
						val=conv(sv[idx])*16U+conv(sv[idx+1]);
					};
					calc(clr.data[0],1);
					calc(clr.data[1],3);
					calc(clr.data[2],5);
					return clr;
				}
				auto last=sv.data()+sv.length();
				auto res=std::from_chars(sv.data(),last,clr.data[0]);
				error(res,"r/gray value");
				auto comma=sv.find(',');
				if(comma==decltype(sv)::npos)
				{
					find_invalids(res.ptr,last);
					clr.data[1]=clr.data[2]=clr.data[0];
					clr.num_layers=1;
					return clr;
				}
				find_invalids(res.ptr,sv.data()+comma);

				res=std::from_chars(sv.data()+comma+1,last,clr.data[1]);
				error(res,"g value");
				comma=sv.find(',',comma+1);
				if(comma==decltype(sv)::npos)
				{
					throw std::invalid_argument("Invalid 2 spectrum color");
				}
				find_invalids(res.ptr,sv.data()+comma);

				res=std::from_chars(sv.data()+comma+1,last,clr.data[2]);
				error(res,"b value");
				comma=sv.find(',',comma+1);
				if(comma==decltype(sv)::npos)
				{
					find_invalids(res.ptr,last);
					clr.num_layers=3;
					return clr;
				}
				find_invalids(res.ptr,sv.data()+comma);

				res=std::from_chars(sv.data()+comma+1,last,clr.data[3]);
				error(res,"a value");
				find_invalids(res.ptr,last);
				clr.num_layers=4;
				return clr;
			}
			cndf(color())
		};

		struct Origin {
			cnnm("origin");
			clbl("o","or");
			PMINLINE static FillRectangle::origin_reference parse(char const* sv)
			{
				auto error=[=]()
				{
					std::string err_msg("Unknown origin code ");
					err_msg.append(sv);
					throw std::invalid_argument(err_msg);
				};
				int ret;
				switch(sv[0])
				{
					case 't':
						ret=0;
						break;
					case 'm':
						ret=3;
						break;
					case 'b':
						ret=6;
						break;
					default:
						error();
				}
				switch(sv[1])
				{
					case 'l':
						break;
					case 'm':
						ret+=1;
						break;
					case 'r':
						ret+=2;
						break;
					case '\0':
						if(ret==3)
						{
							ret+=1;
							break;
						}
					default:
						error();
				}
				return FillRectangle::origin_reference(ret);
			}
		};

		inline void check_flagged(int base,flagged comp,char const* err_msg1,char const* err_msg2)
		{
			if(!comp.flag)
			{
				if(comp.value<=base)
				{
					throw std::invalid_argument(err_msg1);
				}
			}
			else
			{
				if(comp.value<0)
				{
					throw std::invalid_argument(err_msg2);
				}
			}
		}

		inline ImageUtils::Rectangle<int> coords_to_rect(int left,int top,flagged right,flagged bottom)
		{
			check_flagged(left,right,"Right coord must be greater than left coord","Width cannot be negative");
			check_flagged(top,bottom,"Bottom coord must be greater than top coord","Height cannot be negative");
			ImageUtils::Rectangle<int> rect;
			rect.left=left;
			rect.top=top;
			rect.right=right.flag?left+right.value:right.value;
			rect.bottom=bottom.flag?top+bottom.value:bottom.value;
			return rect;
		}

		struct UseTuple {
			PMINLINE static void use_tuple(CommandMaker::delivery& del,std::tuple<int,int,flagged,flagged,color,FillRectangle::origin_reference> const& args)
			{
				auto const& left=std::get<0>(args);
				auto const& top=std::get<1>(args);
				auto const& right=std::get<2>(args);
				auto const& bottom=std::get<3>(args);
				auto const rect=coords_to_rect(left,top,right,bottom);
				del.pl.add_process<FillRectangle>(rect,std::get<4>(args).data,std::get<4>(args).num_layers,std::get<5>(args));
			}
		};

		extern
			SingMaker<UseTuple,IntegerParser<int,Left>,IntegerParser<int,Top>,Right,Bottom,Color,Origin>
			maker;
	}

	namespace GamMaker {
		struct Name {
			cnnm("gamma value");
		};

		struct Maker:public CommandMaker {
		private:
			float value;
		public:
			PMINLINE Maker():value(0.5),CommandMaker("Applies a gamma correction","Gamma","value=2=1/previous")
			{}
			PMINLINE void make_command(iter begin,iter end,CommandMaker::delivery& del) override
			{
				size_t n=end-begin;
				if(n>1)
				{
					throw std::invalid_argument("Too many arguments");
				}
				if(n=1)
				{
					value=FloatParser<Name>().parse(begin[0]);
				}
				else
				{
					value=1/value;
				}
				del.pl.add_process<Gamma>(value);
			}
		};

		extern Maker maker;
	}

	namespace RotMaker {
		struct RadName {
			cnnm("angle");
		};
		struct Degrees:public RadName {
			clbl("deg","rad","r","d");
			PMINLINE static float parse(char const* sv,size_t len)
			{
				if(len==-1)
				{
					return FloatParser<RadName,no_check>().parse(sv);
				}
				auto const c=sv[0];
				sv+=len+1;
				if(c=='d')
				{
					return FloatParser<RadName,no_check>().parse(sv);
				}
				return FloatParser<RadName,no_check>().parse(sv)*RAD_DEG;
			}
		};

		struct Mode {
			PMINLINE static Rotate::interp_mode parse(char const* sv)
			{
				switch(sv[0])
				{
					case 'n':
						return Rotate::nearest_neighbor;
					case 'l':
						return Rotate::linear;
					case 'c':
						return Rotate::cubic;
					default:
						std::string err("Unknown mode ");
						err.append(sv);
						throw std::invalid_argument(err);
				}
			}
			cnnm("interpolation mode");
			clbl("i","m","im");
			cndf(Rotate::interp_mode(Rotate::cubic))
		};

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,float angle,Rotate::interp_mode m,float gamma)
			{
				angle=fmod(angle,360);
				if(angle<0)
				{
					angle+=360;
				}
				if(angle!=0)
				{
					if(gamma!=1)
					{
						del.pl.add_process<Gamma>(gamma);
						del.pl.add_process<Rotate>(angle,m);
						del.pl.add_process<Gamma>(1/gamma);
					}
					else
					{
						del.pl.add_process<Rotate>(angle,m);
					}
				}
			}
		};

		using GammaParser=StrMaker::GammaParser;

		extern
			SingMaker<UseTuple,Degrees,Mode,GammaParser> maker;
	}

	namespace RsMaker {
		struct Factor {
			clbl("f","fact");
			cnnm("factor");
		};

		struct Mode {
			clbl("i","im");
			PMINLINE static constexpr Rescale::rescale_mode def_val()
			{
				return Rescale::automatic;
			}
			PMINLINE static Rescale::rescale_mode parse(char const* mode_string)
			{
				switch(mode_string[0]) //thank you null-termination
				{
					case 'a':
						return Rescale::automatic;
						break;
					case 'n':
						return Rescale::nearest_neighbor;
						break;
					case 'm':
						return Rescale::moving_average;
						break;
					case 'l':
						switch(mode_string[1])
						{
							case 'i':
								return Rescale::linear;
								break;
							case 'a':
								return Rescale::lanczos;
								break;
							case '\0':
								throw std::invalid_argument("Ambiguous mode starting with l");
								break;
							default:
								throw std::invalid_argument("Mode does not exist");
						}
						break;
					case 'g':
						return Rescale::grid;
						break;
					case 'c':
						return Rescale::cubic;
						break;
					default:
						throw std::invalid_argument("Mode does not exist");
				};
			}
			cnnm("interpolation mode");
		};

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,float f,Rescale::rescale_mode rm,float g)
			{
				if(f!=1)
				{
					if(g!=1&&rm!=Rescale::nearest_neighbor)
					{
						del.pl.add_process<Gamma>(g);
						del.pl.add_process<Rescale>(f,rm);
						del.pl.add_process<Gamma>(1/g);
					}
					else
					{
						del.pl.add_process<Rescale>(f,rm);
					}
				}
			}
		};
		extern SingMaker<UseTuple,FloatParser<Factor,no_negatives>,Mode,RotMaker::GammaParser> maker;
	}

	namespace MlaaMaker {
		struct Contrast {
			cnnm("contrast threshold");
			clbl("ct","c");
			cndf(unsigned char(128))
		};
		struct UseTuple {
			static void use_tuple(CommandMaker::delivery& del,unsigned char threshold,double gamma)
			{
				del.pl.add_process<MLAA>(gamma,threshold);
			}
		};
		extern SingMaker<UseTuple,IntegerParser<unsigned char,Contrast>,RotMaker::GammaParser> maker;
	};

	namespace FGMaker {
		struct Min {
			clbl("mn","min","mnv");
			cnnm("min brightness");
		};
		struct Max {
			clbl("mx","max","mxv");
			cnnm("max brightness");
			cndf(unsigned char(255))
		};
		struct Replacer {
			clbl("r","rep");
			cnnm("replacer");
			cndf(unsigned char(255))
		};

		struct UseTuple {
			PMINLINE static void use_tuple(CommandMaker::delivery& del,unsigned char min,unsigned char max,unsigned char rep)
			{
				if(min>max)
				{
					throw std::invalid_argument("Min value cannot be greater than max value");
				}
				if(min==max&&max==rep)
				{
					return;
				}
				del.pl.add_process<FilterGray>(min,max,rep);
			}
		};
		extern
			SingMaker<UseTuple,
			IntegerParser<unsigned char,Min>,
			IntegerParser<unsigned char,Max>,
			IntegerParser<unsigned char,Replacer>> maker;
	}

	namespace BSel {

		class BoundSelMaker:public CommandMaker {
		public:
			PMINLINE BoundSelMaker():CommandMaker("Files between the begin and end are included in the list of files to process","Boundary Select","first_file1 last_file1 ... first_filen last_filen")
			{}
			PMINLINE void make_command(iter begin,iter end,delivery& del) override
			{
				size_t n=end-begin;
				if(n==0)
				{
					throw std::invalid_argument("No arguments given");
				}
				if(n&1)
				{
					throw std::invalid_argument("Odd number of arguments");
				}
				for(size_t i=0;i<n;i+=2)
				{
					delivery::sel_boundary b={begin[i],begin[i+1]};
					auto remove_start_dash=[](std::string_view& sv)
					{
						if(sv[0]=='-'&&sv[1]=='-')
						{
							sv.remove_prefix(1);
						}
					};
					remove_start_dash(b.begin);
					remove_start_dash(b.end);
					del.selections.push_back(b);
				}
			}
		};

		extern BoundSelMaker maker;

	}

	namespace List {
		struct Precheck {
			PMINLINE void check(CommandMaker::delivery const& del)
			{
				if(del.list_files)
				{
					throw std::invalid_argument("List command already given");
				}
			}
		};
		struct UseTuple {
			PMINLINE void use_tuple(CommandMaker::delivery& del)
			{
				del.list_files=true;
			}
		};

		extern MakerTFull<UseTuple,Precheck> maker;
	}

	namespace SIMaker {
		struct Precheck {
			PMINLINE static void check(CommandMaker::delivery const& del)
			{
				if(del.starting_index!=-1)
				{
					throw std::invalid_argument("Starting index already given");
				}
			}
		};

		struct Number {
			cnnm("starting index");
		};

		struct UseTuple {
			PMINLINE static void use_tuple(CommandMaker::delivery& del,unsigned int si)
			{
				del.starting_index=si;
			}
		};

		extern MakerTFull<UseTuple,Precheck,IntegerParser<unsigned int,Number>> maker;
	}

	namespace RgxFilter {
		struct Regex {
			static PMINLINE char const* parse(InputType in)
			{
				if(in[0]=='-'&&in[1]=='-')
				{
					return in+1;
				}
				return in;
			}
			cnnm("regex pattern");
		};
		struct KeepMatch {
			static PMINLINE bool parse(InputType in)
			{
				char c=*in;
				if(c=='t'||c=='1'||c=='T')
				{
					return true;
				}
				return false;
			}
			cnnm("keep match");
			cndf(true)

		};
		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,char const* pattern,bool keep)
			{
				try
				{
					CommandMaker::delivery::filter flt{std::regex(pattern),keep};
					del.rgxes.emplace_back(std::move(flt));
				}
				catch(std::exception const& err)
				{
					std::string err_msg("Invalid regex pattern: ");
					err_msg.append(err.what());
					throw std::invalid_argument(err_msg);
				}
			}
		};

		extern MakerTFull<UseTuple,empty,Regex,KeepMatch> maker;
	}

	namespace CCGMaker {

		PMINLINE InputType find_comma(InputType in)
		{
			while(true)
			{
				if(*in=='\0'||*in==',') return in;
				++in;
			}
		}
		PMINLINE InputType disregard_spaces(InputType in)
		{
			while(true)
			{
				if(*in!=' ') return in;
			}
		}

		PMINLINE std::pair<InputType,InputType> find_first(InputType in,char const* error)
		{
			auto start=disregard_spaces(in);
			auto end=find_comma(start);
			if(*end!=',') throw std::invalid_argument(error);
			return std::make_pair(start,end);
		}

		template<typename T>
		void parse_to(InputType start,char end_char,T& out,char const* invalid_msg,char const* range_msg)
		{
			auto res=exlib::parse(start,out);
			if(res.ce==exlib::conv_error::invalid_characters||*disregard_spaces(res.last)!=end_char)
			{
				throw std::invalid_argument(invalid_msg);
			}
			if(res.ce==exlib::conv_error::out_of_range)
			{
				throw std::invalid_argument(range_msg);
			}
		}

		struct RCR {
			static PMINLINE std::array<unsigned char,2> parse(InputType in)
			{
				std::array<unsigned char,2> ret;
				auto ff=find_first(in,"Missing required color range max value");
				constexpr char const* range_error="Inputs for required color range must be in range [0,255]";
				if(ff.first==ff.second)
				{
					ret[0]=0;
				}
				else
				{
					parse_to(ff.first,',',ret[0],"Invalid input for required color range min value",range_error);
				}
				parse_to(ff.second+1,'\0',ret[1],"Invalid input for required color range max value",range_error);
				if(ret[0]>ret[1]) throw std::invalid_argument("Required color min value cannot be greater than max value");
				return ret;
			}
			cnnm("required color range");
			clbl("rcr");
			cndf((std::array<unsigned char,2>{0,255}))
		};

		struct BSR {
			static PMINLINE std::array<unsigned int,2> parse(InputType in)
			{
				std::array<unsigned int,2> ret;
				auto ff=find_first(in,"Missing bad size range max value");
				constexpr char const* range_error="Cluster size input out of range";
				if(ff.first==ff.second)
				{
					ret[0]=0;
				}
				else
				{
					parse_to(ff.first,',',ret[0],"Invalid input for cluster size min value",range_error);
				}
				parse_to(ff.second+1,'\0',ret[1],"Invalid input for cluster size max value",range_error);
				if(ret[0]>ret[1]) throw std::invalid_argument("Cluster size min value cannot be greater than max value");
				return ret;
			}
			cnnm("cluster size range");
			clbl("bsr");
			cndf((std::array<unsigned int,2>{
				{
					0,0
				}}))
		};

		struct SelRange {
			static PMINLINE std::array<unsigned char,2> parse(InputType in)
			{
				constexpr char const* inv_min="Invalid selection range min input";
				constexpr char const* inv_max="Invalid selection range max input";
				constexpr char const* range_error="Inputs for selection range must be in range [0,255]";
				std::array<unsigned char,2> ret;
				auto start=disregard_spaces(in);
				auto end=find_comma(start);
				if(*end!=',')
				{
					ret[1]=254;
					parse_to(start,'\0',ret[0],inv_min,range_error);
					return ret;
				}
				if(start==end)
				{
					ret[0]=0;
					parse_to(start,'\0',ret[1],inv_max,range_error);
					return ret;
				}
				parse_to(start,',',ret[0],inv_min,range_error);
				parse_to(end+1,'\0',ret[1],inv_max,range_error);
				if(ret[0]>ret[1]) throw std::invalid_argument("Selection min value cannot be greater than max value");
				return ret;
			}
			cnnm("selection range");
			clbl("sr");
			cndf((std::array<unsigned char,2>{
				{
					0,254
				}}))
		};

		struct Replacer {
			clbl("rc");
			cnnm("replacer");
			cndf(unsigned char(255))
		};

		struct EightWay {
			cnnm("eight way");
			clbl("ew");
			static PMINLINE constexpr bool parse(InputType i)
			{
				return Output::MoveParser::parse(i);
			}
			cndf(false)
		};

		struct Precheck {
			static PMINLINE void check(CommandMaker::delivery& del,size_t n)
			{
				SingleCheck::check(del);
				if(n==0)
				{
					throw std::invalid_argument("No arguments given");
				}
			}
		};

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,std::array<unsigned char,2> rsr,std::array<unsigned int,2> bsr,std::array<unsigned char,2> sr,unsigned char rc,bool eight_way)
			{
				if(rsr[0]==0&&rsr[1]==255&&bsr[0]==0&&bsr[1]==0)
				{
					return;
				}
				del.pl.add_process<ClusterClearGrayAlt>(rsr[0],rsr[1],bsr[0],bsr[1],sr[0],sr[1],rc,eight_way);
			}
		};

		extern MakerTFull<UseTuple,Precheck,RCR,BSR,SelRange,IntegerParser<unsigned char,Replacer>,EightWay> maker;
	}

	namespace BlurMaker {
		struct StDev {
			cnnm("radius");
		};

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,float stdev,float gamma)
			{
				if(gamma!=1)
				{
					del.pl.add_process<Gamma>(gamma);
					del.pl.add_process<Blur>(stdev);
					del.pl.add_process<Gamma>(1/gamma);
				}
				else
				{
					del.pl.add_process<Blur>(stdev);
				}
			}
		};

		extern SingMaker<UseTuple,FloatParser<StDev,force_positive>,RotMaker::GammaParser> maker;
	}

	namespace EXLMaker {

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del)
			{
				del.pl.add_process<ExtractLayer0NoRealloc>();
			}
		};

		extern SingMaker<UseTuple> maker;
	}

	namespace CTMaker {
		enum flags {
			red=~0,
			green=~1,
			blue=~2,
			searching=~3
		};
		struct RedName {
			cnnm("red");
			clbl("r","red");
		};
		struct GreenName {
			cnnm("green");
			clbl("g","green");
		};
		struct BlueName {
			cnnm("blue");
			clbl("b","blue");
		};

		struct Red:public RedName {
			static PMINLINE int parse(char const* in)
			{
				switch(in[0])
				{
					case 'g':
						return green;
					case 'b':
						return blue;
					default:
						return IntegerParser<unsigned char,RedName>().parse(in);
				}
			}
			cndf(255)
		};
		struct Green:public GreenName {
			static PMINLINE int parse(char const* in)
			{
				switch(in[0])
				{
					case 'r':
						return red;
					case 'b':
						return blue;
					default:
						return IntegerParser<unsigned char,GreenName>().parse(in);
				}
			}
			cndf(int(red))
		};
		struct Blue:public BlueName {
			static PMINLINE int parse(char const* in)
			{
				switch(in[0])
				{
					case 'r':
						return red;
					case 'g':
						return green;
					default:
						return IntegerParser<unsigned char,BlueName>().parse(in);
				}
			}
			cndf(int(red))
		};
		struct UseTuple {
			static PMINLINE int trace_value(std::array<int,3>& vals,size_t dest)
			{
				if(vals[dest]>=0)
				{
					return vals[dest];
				}
				int next=~vals[dest];
				vals[dest]=searching;
				if(vals[next]==searching) throw std::invalid_argument("Circular dependency");
				vals[dest]=trace_value(vals,next);
				return vals[dest];
			}
		public:
			static PMINLINE void use_tuple(CommandMaker::delivery& del,int r,int g,int b)
			{
				std::array<int,3> vals{
					{
						r,g,b
					}};
				for(int i=0;i<3;++i)
				{
					trace_value(vals,i);
				}
				del.pl.add_process<FillTransparency>(vals[0],vals[1],vals[2]);
			}
		};

		extern SingMaker<UseTuple,Red,Green,Blue> maker;
	}

	namespace RBMaker {
		struct Tol {
			cnnm("tolerance");
			cndf(0.5f)
		};

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,float tol)
			{
				del.pl.add_process<RemoveBorderGray>(tol);
			}
		};
		extern SingMaker<UseTuple,FloatParser<Tol,force_positive>> maker;
	}

	namespace HPMaker {
		using pv=exlib::maybe_fixed<unsigned int>;

		struct LName {
			cnnm("left padding");
			clbl("lph","lpw","left","l");
		};
		struct Left:public LName {
			static PMINLINE pv parse(InputType it,size_t prefix_len)
			{
				auto actual=it+prefix_len+1;
				if(prefix_len==-1||prefix_len==1||prefix_len==4)
				{
					if(*actual=='k')
					{
						return pv(unsigned int(-1));
					}
					else if(*actual=='r')
					{
						return pv(1,2);
					}
					else
					{

						auto amount=IntegerParser<unsigned int,LName>().parse(actual);
						return pv(amount);
					}
				}
				auto amount=parse01(actual);
				if(it[2]=='h')
				{
					return pv(amount,PadBase::height);
				}
				else
				{
					return pv(amount,PadBase::width);
				}
			}
		};
		struct RName {
			cnnm("right padding");
			clbl("rph","rpw","right","r");
		};
		struct Right:public RName {
			static PMINLINE pv parse(InputType it,size_t prefix_len)
			{
				auto actual=it+prefix_len+1;
				if(prefix_len==-1||prefix_len==1||prefix_len==5)
				{
					if(*actual=='k')
					{
						return pv(unsigned int(-1));
					}
					else if(*actual=='l')
					{
						return pv(1,2);
					}
					else
					{
						auto amount=IntegerParser<unsigned int,RName>().parse(actual);
						return pv(amount);
					}
				}
				auto amount=parse01(actual);
				if(it[2]=='h')
				{
					return pv(amount,PadBase::height);
				}
				else
				{
					return pv(amount,PadBase::width);
				}
			}
			cndf(pv(1,2))
		};
		struct TolName {
			clbl("tol","tpw","tph");
			cnnm("tolerance");
		};
		struct Tol:public TolName {
			static PMINLINE pv parse(InputType it,size_t prefix_len)
			{
				auto actual=it+prefix_len+1;
				if(prefix_len==-1||prefix_len==1||it[2]=='l')
				{
					auto amount=IntegerParser<unsigned int,TolName,no_negatives>().parse(actual);
					return pv(amount);
				}
				auto amount=parse01(actual);
				if(it[2]=='h')
				{
					return pv(amount,PadBase::height);
				}
				else
				{
					return pv(amount,PadBase::width);
				}
			}
			cndf(pv(0.005,PadBase::height))
		};
		struct BGround {
			cnnm("background threshold");
			clbl("bg");
			cndf(unsigned char(128))
		};
		using BGParser=IntegerParser<unsigned char,BGround>;
		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,pv left,pv right,pv tol,unsigned char bg)
			{
				if(left.index()==2)
				{
					if(right.index()==2)
					{
						throw std::invalid_argument("Circular dependency");
					}
					left=right;
				}
				if(right.index()==2)
				{
					right=left;
				}
				del.pl.add_process<PadHoriz>(left,right,tol,bg);
			}
		};
		extern SingMaker<UseTuple,Left,Right,Tol,BGParser> maker;
	}

	namespace VPMaker {
		using pv=exlib::maybe_fixed<unsigned int>;
		struct TName {
			cnnm("top padding");
			clbl("t","top","tpw","tph");
		};
		struct Top:public TName {
			static PMINLINE pv parse(InputType it,size_t prefix_len)
			{
				auto actual=it+prefix_len+1;
				if(prefix_len==-1||prefix_len==1||it[2]=='p')
				{
					if(*actual=='k')
					{
						return pv(unsigned int(-1));
					}
					else if(*actual=='r')
					{
						return pv(1,2);
					}
					else
					{

						auto amount=IntegerParser<unsigned int,TName>().parse(actual);
						return pv(amount);
					}
				}
				auto amount=parse01(actual);
				if(it[2]=='h')
				{
					return pv(amount,PadBase::height);
				}
				else
				{
					return pv(amount,PadBase::width);
				}
			}
		};
		struct BName {
			cnnm("bottom padding");
			clbl("b","bottom","bot","bph","bpw");
		};
		struct Bottom:public BName {
			static PMINLINE pv parse(InputType it,size_t prefix_len)
			{
				auto actual=it+prefix_len+1;
				if(prefix_len==-1||prefix_len==1||prefix_len==6)
				{
					if(*actual=='k')
					{
						return pv(unsigned int(-1));
					}
					else if(*actual=='l')
					{
						return pv(1,2);
					}
					else
					{
						auto amount=IntegerParser<unsigned int,BName>().parse(actual);
						return pv(amount);
					}
				}
				auto amount=parse01(actual);
				if(it[2]=='h')
				{
					return pv(amount,PadBase::height);
				}
				else
				{
					return pv(amount,PadBase::width);
				}
			}
			cndf(pv(1,2))
		};
		struct TolName {
			clbl("tol","tpw","tph");
			cnnm("tolerance");
		};
		struct Tol:public TolName {
			static PMINLINE  pv parse(InputType it,size_t prefix_len)
			{
				auto actual=it+prefix_len+1;
				if(prefix_len==-1)
				{
					auto sl=strlen(it);
					if(it[sl-1]=='%')
					{
						auto amount=parse01(actual);
						return pv(amount,PadBase::width);
					}
					else
					{
						auto amount=IntegerParser<unsigned int,TolName>().parse(actual);
						return pv(amount);
					}
				}
				if(prefix_len==1||it[2]=='l')
				{
					auto amount=IntegerParser<unsigned int,TolName>().parse(actual);
					return pv(amount);
				}
				auto amount=parse01(actual);
				if(it[2]=='h')
				{
					return pv(amount,PadBase::height);
				}
				else
				{
					return pv(amount,PadBase::width);
				}
			}
			cndf(pv(0.005,PadBase::height))
		};
		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,pv top,pv bottom,pv tol,unsigned char bg)
			{
				if(top.index()==2)
				{
					if(bottom.index()==2)
					{
						throw std::invalid_argument("Circular dependency");
					}
					top=bottom;
				}
				if(bottom.index()==2)
				{
					bottom=top;
				}
				del.pl.add_process<PadVert>(top,bottom,tol,bg);
			}
		};
		extern SingMaker<UseTuple,Top,Bottom,Tol,HPMaker::BGParser> maker;
	}

	namespace RCGMaker {
		struct Min {
			clbl("mn","min");
			cnnm("min");
		};
		struct Mid {
			clbl("md","mid");
			cnnm("mid");
		};
		struct Max {
			clbl("mx","max");
			cnnm("max");
			cndf(unsigned char(255))
		};

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,unsigned char min,unsigned char mid,unsigned char max)
			{
				if(min>mid)
				{
					throw std::invalid_argument("Min cannot be greater than mid value");
				}
				if(mid>max)
				{
					throw std::invalid_argument("Mid cannot be reater than max value");
				}
				del.pl.add_process<RescaleGray>(min,mid,max);
			}
		};

		extern SingMaker<UseTuple,UCharParser<Min>,UCharParser<Mid>,UCharParser<Max>> maker;
	}

	namespace HSMaker {
		struct Side {
			cnnm("side");
			clbl("side");
			constexpr PMINLINE static bool parse(InputType it)
			{
				if(it[0]=='l'||it[0]=='L')
				{
					return false;
				}
				if(it[0]=='r'||it[0]=='R')
				{
					return true;
				}
				throw std::invalid_argument("Unknown side");
			}
		};
		struct Direction {
			cnnm("direction");
			clbl("dir");
			constexpr PMINLINE static bool parse(InputType it)
			{
				if(it[0]=='b'||it[0]=='B')
				{
					return true;
				}
				if(it[0]=='t'||it[0]=='T')
				{
					return false;
				}
				throw std::invalid_argument("Unknown direction");
			}
		};

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,bool side,bool dir,unsigned char bg)
			{
				del.pl.add_process<HorizontalShift>(side,dir,bg);
			}
		};

		extern SingMaker<UseTuple,Side,Direction,HPMaker::BGParser> maker;
	}

	namespace VSMaker {
		struct Side {
			cnnm("side");
			clbl("side");
			constexpr PMINLINE static bool parse(InputType it)
			{
				if(it[0]=='t'||it[0]=='T')
				{
					return false;
				}
				if(it[0]=='B'||it[0]=='b')
				{
					return true;
				}
				throw std::invalid_argument("Unknown side");
			}
		};
		struct Direction {
			cnnm("direction");
			clbl("dir");
			constexpr PMINLINE static bool parse(InputType it)
			{
				if(it[0]=='r'||it[0]=='R')
				{
					return true;
				}
				if(it[0]=='l'||it[0]=='L')
				{
					return false;
				}
				throw std::invalid_argument("Unknown direction");
			}
		};

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,bool side,bool dir,unsigned char bg)
			{
				del.pl.add_process<VerticalShift>(side,dir,bg);
			}
		};

		extern SingMaker<UseTuple,Side,Direction,HPMaker::BGParser> maker;
	}

	namespace SpliceMaker {
		using pv=decltype(Splice::standard_heuristics().horiz_padding);
		struct HP {
			cnnm("horizontal padding");
			clbl("hp","hppw","hpph");
			cndf(pv(0.03,0))
		};
		struct OP {
			cnnm("optimal padding");
			clbl("op","oppw","opph");
			cndf(pv(0.05,0))
		};
		struct MP {
			cnnm("min padding");
			clbl("mp","mppw","mpph");
			cndf(pv(0.012,0))
		};
		struct OH {
			cnnm("optimal height");
			clbl("oh","ohph","ohpw");
			cndf(pv(0.55,0))
		};
		struct EXC {
			cnnm("excess weight");
			clbl("ew","exw");
			cndf(10.0f)
		};
		struct PW {
			cnnm("padding weight");
			clbl("pw");
			cndf(1.0f)
		};
		struct BG {
			cnnm("background");
			clbl("bg");
			cndf(unsigned char(128))
		};
		template<typename Base>
		struct pv_parser:public Base {
			PMINLINE pv parse(InputType it,size_t prefix_len) const
			{
				auto actual=it+prefix_len+1;
				if(prefix_len==-1||prefix_len==2)
				{
					auto sl=strlen(actual);
					if(sl==0) throw std::invalid_argument(std::string("Empty argument for ").append(Base::name));
					if(actual[sl-1]=='%') return pv(parse01(actual),0);
					return pv(UIntParser<Base>().parse(actual));
				}
				auto amount=parse01(actual);
				if(it[3]=='w')
				{
					return pv(amount,0);
				}
				return pv(amount,1);
			}
		};

		struct Divider {
			using ccp=char const*;
			cnnm("divider");
			clbl("div");
			cndf(ccp{nullptr});
			static char const* parse(InputType in)
			{
				return Output::PatternParser::parse(in);
			}
		};
		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,pv hp,pv op,pv mp,pv oh,float exc,float pw,unsigned char bg,char const* divider)
			{
				del.splice_args.horiz_padding=hp;
				del.splice_args.optimal_padding=op;
				del.splice_args.min_padding=mp;
				del.splice_args.optimal_height=oh;
				del.splice_args.excess_weight=exc;
				del.splice_args.padding_weight=pw;
				del.splice_args.background_color=bg;
				if(divider)
				{
					del.splice_divider.load(divider);
				}
			}
		};
		extern
			MakerTFull<
			UseTuple,
			MultiCommand<CommandMaker::delivery::do_state::do_splice>,
			pv_parser<HP>,pv_parser<OP>,pv_parser<MP>,pv_parser<OH>,
			FloatParser<EXC>,FloatParser<PW>,FloatParser<BG>,Divider> maker;
	}

	namespace CutMaker {

		using pv=SpliceMaker::pv;

		template<typename T>
		using pv_parser=SpliceMaker::pv_parser<T>;

		struct MW {
			cnnm("min width");
			clbl("mw","mwpw","mwph");
			cndf(pv(0.66,0))
		};

		struct MH {
			cnnm("min height");
			clbl("mh","mhpw","mhph");
			cndf(pv(0.08,0))
		};

		struct HW {
			cnnm("horiz weight");
			clbl("hw");
			cndf(20.0f)
		};

		struct MV {
			cnnm("min vertical space");
			clbl("mv","mvpw","mvph");
			cndf(pv(0))
		};

		using uchar=unsigned char;
		struct BG {
			cnnm("background");
			clbl("bg");
			cndf(uchar(128))
		};

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,pv mw,pv mh,pv mv,float hw,unsigned char bg)
			{
				del.cut_args.min_height=mh;
				del.cut_args.min_width=mw;
				del.cut_args.min_vert_space=mv;
				del.cut_args.background=bg;
				del.cut_args.horiz_weight=hw;
			}
		};

		extern
			MakerTFull<
			UseTuple,
			MultiCommand<CommandMaker::delivery::do_state::do_cut>,
			pv_parser<MW>,pv_parser<MH>,pv_parser<MV>,FloatParser<HW>,UCharParser<BG>> maker;

	}

	namespace SmartScale {

		struct Ratio {
			clbl("fact","f");
			cnnm("factor");
		};

		struct Input {
			clbl("net");
			PMINLINE static char const* parse(InputType it)
			{
				return it;
			}
			cnnm("network_file");
			cndf(nullptr)
		};

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,float ratio,char const* network)
			{
				if(ratio<=0)
				{
					throw std::invalid_argument("Ratio must be positive");
				}
				if(ratio>1)
				{
					del.overridden_num_threads=1;
					if(network==nullptr)
					{
						std::string path;
						path.reserve(MAX_PATH);
						GetModuleFileNameA(NULL,path.data(),MAX_PATH);
						auto const files=exlib::files_in_dir(path);
						for(auto const& f:files)
						{
							auto ext=exlib::find_extension(f.data(),f.data()+f.size());
							if(strcmp(ext,"ssn")==0)
							{
								del.pl.add_process<NeuralScale>(ratio,f.data(),&del.overridden_num_threads);
								break;
							}
						}
						throw std::invalid_argument("Failed to find network data");
					}
					else
					{
						del.pl.add_process<NeuralScale>(ratio,network,&del.overridden_num_threads);
					}
				}
				if(ratio<1)
				{
					del.pl.add_process<Rescale>(ratio,Rescale::moving_average);
				}
			}
		};

		extern SingMaker<UseTuple,FloatParser<Ratio>,Input> maker;
	}

	namespace Cropper {

		template<typename Base>
		struct TLParser:Base {
			cndf(int(0))
		};

		using Left=TLParser<FRMaker::Left>;

		using Top=TLParser<FRMaker::Top>;

		using FRMaker::flagged;
		
		template<typename Base>
		struct RBParser:Base {
			static std::optional<flagged> parse(char const* c,size_t prefix)
			{
				if(*c=='k')
				{
					return {};
				}
				return Base::parse(c,prefix);
			}
			cndf(std::optional<flagged>())
		};

		using Bottom=RBParser<FRMaker::Bottom>;

		using Right=RBParser<FRMaker::Right>;

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,int l,int t,std::optional<flagged> r,std::optional<flagged> b)
			{
				auto fix_value=[](int base,auto& val,auto err_msg1,auto err_msg2)
				{
					if(val.has_value())
					{
						auto& value=val.value();
						FRMaker::check_flagged(base,value,err_msg1,err_msg2);
						if(value.flag)
						{
							value.value+=base;
						}
					}
				};
				fix_value(l,r,"Right coord must be greater than left coord","Width cannot be negative");
				fix_value(t,b,"Bottom coord must be greater than top coord","Height cannot be negative");
				auto flagged_to_optional_int=[](auto flagged) -> std::optional<int>
				{
					if(flagged.has_value())
					{
						return {flagged.value().value};
					}
					return {};
				};
				del.pl.add_process<Crop>(l,t,flagged_to_optional_int(r),flagged_to_optional_int(b));
			}
		};

		extern SingMaker<UseTuple,IntParser<Left>,IntParser<Top>,Right,Bottom> maker;
	}

	namespace Quality {
		struct Value {
			cnnm("quality");
		};
		struct Precheck {
			static PMINLINE void check(CommandMaker::delivery& del)
			{
				if(del.quality!=-1)
				{
					throw std::invalid_argument("Quality already set");
				}
			}
		};
		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,int quality)
			{
				if(quality<0||quality>100)
				{
					throw std::invalid_argument("Quality must be an integer [0,100]");
				}
				del.quality=quality;
			}
		};
		extern MakerTFull<UseTuple,Precheck,IntParser<Value>> maker;
	}

	namespace RescaleAbsoluteMaker {
		using uint=unsigned int;
		inline constexpr uint interpolate=-1;
		struct Width {
			clbl("w","width");
			cnnm("width");
			cndf(interpolate)
		};
		struct Height {
			clbl("h","height");
			cnnm("height");
			cndf(interpolate)
		};
		struct Ratio {
			clbl("r","rat","ratio");
			cnnm("ratio");
			static float parse(InputType it)
			{
				if(it[0]=='p')
				{
					return -1.0;
				}
				return FloatParser<Ratio>{}.parse(it);
			}
			cndf(-2.0f)
		};
		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,int width,int height,float ratio,Rescale::rescale_mode mode,float gamma)
			{
				if(width!=-1&&height!=-1&&ratio>=-1.0f)
				{
					throw std::invalid_argument("You may only give two of three arguments");
				}
				if(gamma!=1)
				{
					del.pl.add_process<Gamma>(gamma);
					del.pl.add_process<RescaleAbsolute>(width,height,ratio,mode);
					del.pl.add_process<Gamma>(1/gamma);
				}
				else
				{
					del.pl.add_process<RescaleAbsolute>(width,height,ratio,mode);
				}
			}
		};
		extern SingMaker<UseTuple,UIntParser<Width>,UIntParser<Height>,Ratio,RsMaker::Mode,RotMaker::GammaParser> maker;
	}

	namespace CCSMaker {
		using FRMaker::Origin;
		inline constexpr int interpolate=-1;
		struct Width {
			clbl("w","width");
			cnnm("width");
			static int parse(InputType it)
			{
				if(it[0]=='p')
				{
					return interpolate;
				}
				return IntParser<empty,no_negatives>{}.parse(it);
			}
			cndf(interpolate)
		};
		struct Height {
			clbl("h","height");
			cnnm("height");
			static int parse(InputType it)
			{
				return Width::parse(it);
			}
			cndf(interpolate)
		};
		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,int width,int height,FillRectangle::origin_reference origin)
			{
				del.pl.add_process<ChangeCanvasSize>(width,height,origin);
			}
		};
		extern SingMaker<UseTuple,Width,Height,Origin> maker;
	}

	namespace TemplateClearMaker {

		struct Name {
			cnnm("template name");
			clbl("name","nm","tnm");
			static char const* parse(InputType in)
			{
				return in;
			}
		};
		struct Threshold {
			cnnm("threshold");
			clbl("thresh","th","thr");
			static float parse(InputType in)
			{
				auto res=FloatParser<empty,no_check>().parse(in);
				if(res<0||res>1) throw std::invalid_argument("Range [0,1] required");
				return res;
			}
			cndf(0.95f)
		};
		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,char const* name,float threshold)
			{
				del.pl.add_process<TemplateMatchErase>(name,threshold);
			}
		};
		extern SingMaker<UseTuple,Name,Threshold> maker;
	}
	namespace SlidingTemplateClearMaker {
		struct Downscale {
			cnnm("downscale_factor");
			clbl("dsf","fact","f");
			static unsigned int parse(InputType in)
			{
				auto res=IntegerParser<unsigned int,empty>().parse(in);
				if(res==0)
				{
					throw std::invalid_argument("Value must be greater than 0");
				}
				return res;
			}
		};
		struct Replacer {
			cnnm("replacer");
			clbl("mutualfloodfill","mff","fill","fll","replace","rpl"/*,"darken","drk","lighten","lgh"*/);
			using Func=std::function<void(cil::CImg<unsigned char>&,cil::CImg<unsigned char> const&,ImageUtils::PointUINT)>;
			static Func parse(InputType in,std::size_t prefix_len)
			{
				auto const real_start=in+prefix_len+1;
				if(prefix_len==-1||in[0]=='f')
				{
					auto const color=FRMaker::Color::parse(real_start);
					return [color](cil::CImg<unsigned char>& img,cil::CImg<unsigned char> const& tmplt,ImageUtils::PointUINT point)
					{
						FillRectangle{
							{int(point.x),int(point.x+tmplt.width()),int(point.y),int(point.y+tmplt.height())},
						color.data,
						color.num_layers,
						FillRectangle::origin_reference::top_left}.process(img);
					};
				}
				if(in[0]=='r')
				{
					return [](cil::CImg<unsigned char>& img,cil::CImg<unsigned char> const& tmplt,ImageUtils::PointUINT point)
					{
						copy_paste(img,tmplt,{0,tmplt._width,0,tmplt._height},{int(point.x),int(point.y)});
					};
				}
				if(in[0]=='m')
				{
					auto ul=IntegerParser<unsigned char,Replacer>().parse(real_start);
					if(ul==255)
					{
						throw std::invalid_argument("This flood fill would be a no-op");
					}
					return [ul](cil::CImg<unsigned char>& img,cil::CImg<unsigned char> const& tmplt,ImageUtils::PointUINT point)
					{
						using Point=decltype(point);
						std::vector<detail::scan_range> buffer;
						if(img._spectrum==1)
						{
							for(unsigned int y=0;y<tmplt._height;++y)
							{
								for(unsigned int x=0;x<tmplt._width;++x)
								{
									if(tmplt(x,y)<=ul)
									{
										detail::flood_operation(img,point+Point{x,y},[=,&img](Point p)
											{
												if(img(p.x,p.y)<=ul)
												{
													img(p.x,p.y)=255;
													return true;
												}
												return false;
											},[&](auto)
											{},buffer);
									}
								}
							}
						}
						else
						{
							std::vector<ImageUtils::horizontal_line<unsigned int>> rects;
							for(unsigned int y=0;y<tmplt._height;++y)
							{
								for(unsigned int x=0;x<tmplt._width;++x)
								{
									if(tmplt(x,y)<=ul)
									{
										detail::flood_operation(img,point+Point{x,y},[=,&img](Point p)
											{
												if(img(p.x,p.y)<=ul)
												{
													img(p.x,p.y)=255;
													return true;
												}
												return false;
											},[&](ImageUtils::horizontal_line<unsigned int> line)
											{
												rects.push_back(line);
											},buffer);
									}
								}
							}
							for(unsigned int s=1;s<img._spectrum;++s)
							{
								for(auto const rect:rects)
								{
									std::fill(&img(rect.left,rect.y,0,s),&img(rect.right,rect.y,0,s),255);
								}
							}
						}
					};
				}
			}
			static Func def_val()
			{
				return [](cil::CImg<unsigned char>& img,cil::CImg<unsigned char> const& tmplt,ImageUtils::PointUINT point)
				{
					FillRectangle{
						{int(point.x),int(point.x+tmplt.width()),int(point.y),int(point.y+tmplt.height())},
					{255,255,255,255},
					1,
					FillRectangle::origin_reference::top_left}.process(img);
				};
			}
		};


		struct Left:FRMaker::Left {
			cndf(-999999)
		};
		struct Top:FRMaker::Top {
			cndf(-999999)
		};
		using FRMaker::flagged;
		struct Right:FRMaker::Right {
			static flagged def_val()
			{
				return {false,999999};
			}
		};
		struct Bottom:FRMaker::Bottom {
			static flagged def_val()
			{
				return {false,99999};
			}
		};
		using FRMaker::Origin;

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,char const* name,unsigned int downscale,float threshold,Replacer::Func replacer,int left,int top,flagged hor,flagged vert,FillRectangle::origin_reference origin)
			{
				std::vector<cil::CImg<unsigned char>> tmplts;
				auto start=Output::PatternParser::parse(name);
				auto find_next=[](char const* in)
				{
					while(true)
					{
						auto c=*in;
						if(c=='*')
						{
							return in;
						}
						if(c=='\0')
						{
							return in;
						}
						++in;
					}
				};
				std::string filename;
				while(true)
				{
					auto end=find_next(start);
					filename.assign(start,end-start);
					tmplts.emplace_back(filename.c_str());
					if(*end=='\0')
					{
						break;
					}
					start=Output::PatternParser::parse(end+1);
				}
				if(tmplts.empty())
				{
					throw std::invalid_argument("Need at least one template");
				}
				del.pl.add_process<SlidingTemplateMatchEraseExact>(std::move(tmplts),downscale,threshold,replacer,FRMaker::coords_to_rect(left,top,hor,vert),origin);
			}
		};

		extern SingMaker<UseTuple,TemplateClearMaker::Name,Downscale,TemplateClearMaker::Threshold,Replacer,IntParser<Left>,IntParser<Top>,Right,Bottom,Origin> maker;
	}

	namespace RemoveEmptyLinesMaker {
		struct MinSpace {
			cnnm("min space");
			clbl("ms");
		};
		struct MaxPresence {
			cnnm("max presence");
			clbl("mp");
			cndf(5U);
		};
		struct UseTuple {
			static void use_tuple(CommandMaker::delivery& del,unsigned int ms,unsigned int mp,unsigned char bt)
			{
				del.pl.add_process<RemoveEmptyLines>(ms,mp,bt);
			}
		};
		extern SingMaker<UseTuple,UIntParser<MinSpace>,UIntParser<MaxPresence>,HPMaker::BGParser> maker;
	}

	namespace VerticalCompressMaker {
		struct MinSpace {
			cnnm("min (vertical) space");
			clbl("ms","mvs");
		};
		struct MinHSpace {
			cnnm("min horizontal space");
			clbl("mhs");
			static unsigned int parse(char const* data)
			{
				if(strcmp(data,"ms")==0||strcmp(data,"mvs")==0)
				{
					return -1;
				}
				return UIntParser<MinHSpace>().parse(data);
			}
			cndf(-1)
		};
		struct MaxVerticalProtection {
			cnnm("max vertical protection");
			clbl("mvp");
		};
		struct MinHorizProtection {
			cnnm("min horizontal protection");
			clbl("mhp");
		};
		struct UseTuple {
			static void use_tuple(CommandMaker::delivery& del,unsigned int mvs,unsigned int mh,unsigned int mv,unsigned char bg,unsigned int mhs)
			{
				del.pl.add_process<VertCompress>(mvs,mhs,mh,mv,bg);
			}
		};
		extern SingMaker<UseTuple,UIntParser<MinSpace>,UIntParser<MaxVerticalProtection>,UIntParser<MinHorizProtection>,HPMaker::BGParser,MinHSpace> maker;
	}

	struct compair {
	private:
		char const* _key;
		CommandMaker* _maker;
	public:
		PMINLINE constexpr compair():_key(0),_maker(0)
		{
			//static_assert(false,"I'm a stupid dumdum that didn't intialize my key pair");
		}
		PMINLINE constexpr compair(char const* key,CommandMaker* m):_key(key),_maker(m)
		{}
		PMINLINE constexpr char const* key() const
		{
			return _key;
		}
		PMINLINE constexpr CommandMaker* maker() const
		{
			return _maker;
		}
	};

	namespace {
		constexpr auto scl=exlib::make_array<compair>(
			compair("cg",&CGMaker::maker),
			compair("fg",&FGMaker::maker),
			compair("hp",&HPMaker::maker),
			compair("vp",&VPMaker::maker),
			compair("str",&StrMaker::maker),
			compair("rot",&RotMaker::maker),
			compair("fr",&FRMaker::maker),
			compair("rcg",&RCGMaker::maker),
			compair("ccg",&CCGMaker::maker),
			compair("bl",&BlurMaker::maker),
			compair("exl",&EXLMaker::maker),
			compair("ct",&CTMaker::maker),
			compair("rb",&RBMaker::maker),
			compair("rs",&RsMaker::maker),
			compair("crp",&Cropper::maker),
			compair("rsa",&RescaleAbsoluteMaker::maker),
			compair("ccs",&CCSMaker::maker),
			compair("mlaa",&MlaaMaker::maker),
			compair("tme",&TemplateClearMaker::maker),
			compair("stme",&SlidingTemplateClearMaker::maker),
			compair("rel",&RemoveEmptyLinesMaker::maker),
			compair("vc",&VerticalCompressMaker::maker));

		constexpr auto mcl=exlib::make_array<compair>(
			compair("spl",&SpliceMaker::maker),
			compair("cut",&CutMaker::maker));

		constexpr auto ol=exlib::make_array<compair>(
			compair("o",&Output::maker),
			compair("vb",&Verbosity::maker),
			compair("nt",&NumThreads::maker),
			compair("bsel",&BSel::maker),
			compair("si",&SIMaker::maker),
			compair("flt",&RgxFilter::maker),
			compair("list",&List::maker),
			compair("q",&Quality::maker));

		constexpr auto aliases=exlib::make_array<compair>(
			compair("rotate",&RotMaker::maker),
			compair("ccga",&CCGMaker::maker),
			compair("splice",&SpliceMaker::maker),
			compair("ss",&SmartScale::maker),
			compair("hs",&HSMaker::maker),
			compair("vs",&VSMaker::maker));

		struct comp {
			constexpr bool operator()(compair a,compair b) const
			{
				return exlib::strcmp(a.key(),b.key())<0;
			}
		};
		constexpr auto com_map=exlib::sorted(exlib::concat(scl,mcl,ol,aliases),comp());
	}

	PMINLINE constexpr auto const& single_command_list() noexcept
	{
		return scl;
	}

	PMINLINE constexpr auto const& multi_command_list() noexcept
	{
		return mcl;
	}

	PMINLINE constexpr auto const& option_list() noexcept
	{
		return ol;
	}

	PMINLINE CommandMaker* find_command(char const* lbl) noexcept
	{
		struct find {
			int operator()(char const* target,compair cp)
			{
				return exlib::strcmp(target,cp.key());
			}
		};
		//when com_map gets bigger, might change this to a hash map, doesn't really matter though; playing with constexpr and the limits of MSVC was fun
		auto const r=exlib::binary_find(com_map.begin(),com_map.end(),lbl,find());
		if(r==com_map.end())
		{
			return nullptr;
		}
		return r->maker();
	}
#undef cnnm
#undef cndf
#undef ncdf
}
#endif // !PROCESS_MAKERS_H
