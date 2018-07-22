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
#define PMINLINE inline
namespace ScoreProcessor {

	using InputType=char const*;
	using iter=InputType const*;

	class CommandMaker {
	public:
		struct delivery {
			SaveRules sr;
			unsigned int starting_index;
			bool do_move;
			ProcessList<unsigned char> pl;
			enum do_state {
				do_absolutely_nothing,
				do_nothing,
				do_single,
				do_cut,
				do_splice
			};
			do_state flag;
			enum log_type {
				unassigned_log,
				quiet,
				errors_only,
				count,
				full_message
			};
			log_type lt;
			struct {
				ImageUtils::perc_or_val horiz_padding;
				ImageUtils::perc_or_val optimal_padding;
				ImageUtils::perc_or_val min_padding;
				ImageUtils::perc_or_val optimal_height;
				float excess_weight;
				float padding_weight;
			} splice_args;
			struct {
				ImageUtils::perc_or_val min_height,min_width,min_vert_space;
				float horiz_weight;
			} cut_args;
			std::regex rgx;
			enum regex_state {
				unassigned,normal,inverted
			};
			struct sel_boundary {
				std::string_view begin; //will always be present in the parameters list
				std::string_view end;
			};
			std::vector<sel_boundary> selections;
			regex_state rgxst;
			unsigned int num_threads;
			bool list_files;
			PMINLINE delivery():starting_index(1),flag(do_absolutely_nothing),num_threads(0),rgxst(unassigned),do_move(false),list_files(false),lt(unassigned_log)
			{}
		};
	private:
		std::string_view _help_message;
		std::string_view _name;
		std::string_view _args;
	public:
		//COMPILE-TYPE CONSTANT STRINGS REQUIRED
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
		virtual void make_command(iter begin,iter end,delivery& del)=0;
	};

	struct empty {};

	struct empty2 {};

	struct empty3 {};

	struct no_check {
		template<typename T,typename Name>
		static constexpr void check(T,Name const&)
		{}
	};

	struct no_negatives {
		template<typename T,typename Name>
		static void check(T val,Name const& n)
		{
			if(val<0)
			{
				std::string err_msg("Value for ");
				err_msg.append(n.name()).append(" must be non-negative");
				throw std::invalid_argument(err_msg);
			}
		}
	};

	template<typename Base,typename Check=no_negatives>
	struct FloatParser:public Base,private Check {
		using Base::name;
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
				std::string err_msg("Invalid input for ");
				err_msg.append(Base::name());
				throw std::invalid_argument(err_msg);
			}
			Check::check(f,static_cast<Base const&>(*this));
			return f;
		}
	};

	template<typename Base,typename Check=no_negatives>
	struct DoubleParser:public Base,private Check {
		using Base::name;
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
				std::string err_msg("Invalid input for ");
				err_msg.append(Base::name());
				throw std::invalid_argument(err_msg);
			}
			Check::check(f,static_cast<Base const&>(*this));
			return f;
		}

	};

	template<typename T,typename Base,typename Check=no_check>
	struct IntegerParser:public Base,private Check {
		T parse(char const* in) const
		{
			T t;
			std::string_view str(in);
			auto const last=str.data()+str.size();
			auto const res=std::from_chars(str.data(),last,t);
			if(res.ec==std::errc::invalid_argument)
			{
				std::string err_msg("Invalid argument for");
				err_msg.append(Base::name());
				throw std::invalid_argument(err_msg);
			}
			if(res.ec==std::errc::result_out_of_range)
			{
				std::string err_msg("Argument for ");
				err_msg.append(Base::name());
				err_msg.append(" must be in range [");
				if constexpr(std::is_unsigned<T>())
				{
					constexpr auto min=exlib::to_string_unsigned<std::numeric_limits<T>::min()>();
					err_msg.append(min.data(),min.size()-1);
					constexpr auto max=exlib::to_string_unsigned<std::numeric_limits<T>::max()>();
					err_msg.append(",").append(max.data(),max.size()-1);
				}
				else
				{
					constexpr auto min=exlib::to_string<std::numeric_limits<T>::min()>();
					err_msg.append(min.data(),min.size()-1);
					constexpr auto max=exlib::to_string<std::numeric_limits<T>::max()>();
					err_msg.append(",").append(max.data(),max.size()-1);
				}
				err_msg.append("]");
				throw std::invalid_argument(err_msg);
			}
			for(auto it=res.ptr;it<last;++it)
			{
				auto const c=*it;
				if(c!=' ')
				{
					std::string err_msg("Invalid trailing characters: ");
					err_msg.append(it,last-it);
					throw std::invalid_argument(err_msg);
				}
			}
			Check::check(t,static_cast<Base const&>(*this));
			return t;
		}
	};

	//if LabelId defines a function id(InputType,(optional)size_t prefix_len), this function returns a size_t in range [0,MaxArgs)
	//indicating the index of the tag, or throws an exception indicating the tag does not exist
	//otherwise, the parser will just go through arguments in order
	//Each type of ArgParsers must define a function Type parse(char const*,(optional)size_t prefix_len);
	//optionally a function Type def_val(), if it is used;
	//and a function CompatibleWithString name().
	//If parse accepts a second real_start argument, the first string_view is of the whole input and prefix_len
	//is the length of the prefix not including the colon separator. Otherwise the first string is only a view past
	//the colon.
	//UseTuple must define a function use_tuple(CommandMaker::delivery&,Args...), which uses the tuple result
	//from parsing to change the delivery. The tuple may optionally be referenced directly.
	//if Precheck defines check(CommandMaker::delivery (const)(&),(optional)size_t num_args), this checks the delivery before parsing, 
	//throwing or doing nothing
	template<typename UseTuple,typename Precheck,typename LabelId,typename... ArgParsers>
	class MakerTFull:protected LabelId,protected Precheck,protected UseTuple,protected std::tuple<ArgParsers...>,public CommandMaker {
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

	public:
		constexpr static size_t const MinArgs=defaults<ArgParsers...>();
		constexpr static size_t const MaxArgs=sizeof...(ArgParsers);
	protected:
		using MyParsers=std::tuple<ArgParsers...>;
		using MyArgs=std::tuple<decltype(check_parse<ArgParsers>(0))...>;
		static_assert(std::tuple_size<MyParsers>::value==MaxArgs);
		static_assert(std::tuple_size<MyArgs>::value==MaxArgs);
	private:
		constexpr MyParsers&as_parsers()
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

		template<size_t N>
		void eval_arg_at(MyArgs& mt,InputType str,size_t prefix_len)
		{
			static_assert(N<MaxArgs,"Invalid tuple index");
			std::get<N>(mt)=std::get<N>(as_parsers()).parse(str,prefix_len);
		}

		template<size_t N>
		void eval_arg_at(MyArgs& mt,InputType str)
		{
			static_assert(N<MaxArgs,"Invalid tuple index");
			std::get<N>(mt)=std::get<N>(as_parsers()).parse(str);
		}

		template<size_t N>
		void find_arg_h(MyArgs& mt,InputType str,size_t prefix_len)
		{
			if constexpr(accepts_prefix<std::decay<decltype(std::get<N>(as_parsers()))>::type>::value)
			{
				eval_arg_at<N>(mt,str,prefix_len);
			}
			else
			{
				eval_arg_at<N>(mt,str+prefix_len+1);
			}
		}

		template<size_t N>
		void find_arg(size_t i,MyArgs& mt,InputType str,size_t prefix_len)
		{
			if constexpr(N<MaxArgs)
			{
				if(i==N)
				{
					find_arg_h<N>(mt,str,prefix_len);
				}
				else
				{
					find_arg<N+1>(i,mt,str,prefix_len);
				}
			}
		}

		template<size_t... I>
		constexpr auto name_table(std::index_sequence<I...>)
		{
			std::array<std::string_view,MaxArgs> arr{{std::string_view(std::get<I>(as_parsers()).name())...}};
			return arr;
		}

		template<size_t N>
		constexpr std::string_view find_arg_name(size_t i)
		{
			static auto table=name_table(std::make_index_sequence<MaxArgs>());
			return table[i];
			/*if constexpr(N<MaxArgs)
			{
				if(i==N)
				{
					return std::get<N>(as_parsers()).name();
				}
				else
				{
					find_arg_name<N+1>(i);
				}
			}
			else
			{
				throw std::runtime_error("Name not found, index too high");
			}*/
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

		struct has_labels {
		private:
			template<typename U>
			constexpr static auto val(int) -> decltype(size_t(std::declval<U>().id(InputType(),size_t())),int())
			{
				return 2;
			}
			template<typename U>
			constexpr static auto val(int) -> decltype(size_t(std::declval<U>().id(std::string_view())),int())
			{
				return 1;
			}
			template<typename>
			constexpr static int val(...)
			{
				return 0;
			}
		public:
			constexpr static bool const value=val<LabelId>(0);
		};

		size_t find_label(InputType data,size_t len)
		{
			if constexpr(has_labels::value==1)
			{
				return LabelId::id(data,len);
			}
			else
			{
				return LabelId::id(data);
			}
		}

		template<size_t N>
		void ordered_args(iter begin,size_t n,MyArgs& mt)
		{
			if constexpr(N<MaxArgs)
			{
				if(N<n)
				{
					eval_arg_at<N>(mt,begin[N]);
					ordered_args<N+1>(begin,n,mt);
				}
			}
		}

		void throw_if_repeat(size_t index,bool fulfilled[])
		{
			if(fulfilled[index])
			{
				std::string err_msg("Argument already given for ");
				err_msg.append(find_arg_name<0>(index));
				throw std::invalid_argument(err_msg);
			}
			fulfilled[index]=true;
		}

		void eval_tagged(InputType data,size_t prefix_len,MyArgs& mt,bool fulfilled[])
		{
			auto const index=find_label(data,prefix_len);
			throw_if_repeat(index,fulfilled);
			find_arg<0>(index,mt,data,prefix_len);
		}

		template<size_t N>
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
						find_arg_h<N>(mt,sv,-1);
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

		template<size_t N>
		void check_required(bool fulfilled[])
		{
			if constexpr(N<MinArgs)
			{
				if(!fulfilled[N])
				{
					std::string err_msg("Missing input for ");
					err_msg.append(std::get<N>(as_parsers()).name());
					throw std::invalid_argument(err_msg);
				}
				check_required<N+1>(fulfilled);
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
				if constexpr(has_labels::value)
				{
					bool fulfilled[MaxArgs];
					std::memset(fulfilled,0,MaxArgs);
					unordered_args<0>(begin,n,mt,fulfilled);
					check_required<0>(fulfilled);
				}
				else
				{
					ordered_args<0>(begin,n,mt);
				}
			}
			if constexpr(uses_tuple::value)
			{
				UseTuple::use_tuple(del,mt);
			}
			else
			{
				std::apply(
					[&del,this](auto&&... args)
				{
					UseTuple::use_tuple(del,std::forward<std::decay<decltype(args)>::type>(args)...);
				},mt);
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

	template<typename UseTuple,typename LabelId,typename... ArgParsers>
	using SingMaker=MakerTFull<UseTuple,SingleCheck,LabelId,ArgParsers...>;

	[[noreturn]]
	PMINLINE void bad_pf(InputType sv,size_t len)
	{
		std::string err_msg("Unknown prefix ");
		err_msg.append(sv,len);
		throw std::invalid_argument(err_msg);
	}

	namespace {
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

		template<size_t N>
		constexpr auto make_ltable(ltable<N> const& arr)
		{
			return exlib::sorted(arr,lcomp());
		}

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
			return exlib::binary_find(arr.begin(),arr.end(),target,lcomp());
		}
	}

#define cnnm(n) static PMINLINE constexpr std::string_view name() { return n; }
#define cndf(n) static PMINLINE constexpr auto def_val() { return n; }
#define ncdf(n) static PMINLINE auto def_val() { return n; }

	namespace Output {

		struct PatternParser {
			cndf("%w")
				PMINLINE constexpr char const* parse(char const* s)
			{
				if(s[0]=='-'&&s[1]=='-')
				{
					return s+1;
				}
				return s;
			}
			cnnm("pattern")
		};

		struct MoveParser {
			static PMINLINE constexpr bool parse(char const* s)
			{
				auto const c=s[0];
				if(c=='t'||c=='1'||c=='T')
				{
					return true;
				}
				return false;
			}
			cndf(false)
				cnnm("move")
		};

		class UseTuple {
		public:
			static PMINLINE void use_tuple(CommandMaker::delivery& del,char const* input,bool do_move)
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

		struct LabelId {
			static PMINLINE size_t id(InputType sv,size_t len)
			{
				static constexpr auto table=make_ltable(
					le("mv",1),le("move",1),le("m",1),
					le("o",0),le("out",0),
					le("p",0),le("pat",0));
				auto const index=lfind(table,sv);
				if(index!=table.end())
				{
					return index->index();
				}
				bad_pf(sv,len);
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

		extern MakerTFull<UseTuple,Precheck,LabelId,PatternParser,MoveParser>
			maker;
	}

	namespace NumThreads {
		struct positive {
			template<typename Name>
			PMINLINE static void check(unsigned int n,Name const&)
			{
				if(n==0)
				{
					throw std::invalid_argument("Thread count must be positive");
				}
			}
		};
		struct Name {
			cnnm("number of threads")
		};
		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del,unsigned int nt)
			{
				del.num_threads=nt;
			}
		};
		extern MakerTFull<UseTuple,empty,empty2,IntegerParser<unsigned int,Name,positive>> maker;
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
			MakerTFull<UseTuple,Precheck,empty,Level> maker;
	}

	namespace StrMaker {
		struct MinAngle {
			cnnm("min angle")
				cndf(double(-5))
		};
		struct MaxAngle {
			cnnm("max angle")
				cndf(double(5))
		};
		struct AnglePrec {
			cnnm("angle precision")
				cndf(double(0.1))
		};
		struct PixelPrec {
			cnnm("pixel precision")
				cndf(double(1))
		};
		struct Boundary {
			cnnm("boundary")
				cndf(unsigned char(128))
		};
		struct Gamma {
			cnnm("gamma")
				cndf(float(2))
		};

		struct LabelId {
			PMINLINE static size_t id(InputType sv,size_t len)
			{
				using le=lookup_entry;
				static constexpr auto table=make_ltable(ltable<13>
					(
						{le("a",2),le("ap",2),
						le("b",4),
						le("g",5),le("gam",5),
						le("max",1),
						le("min",0),le("mn",0),le("mna",0),
						le("mx",1),le("mxa",1),
						le("p",3),le("pp",3)}));
				auto idx=lfind(table,sv);
				if(idx!=table.end())
				{
					return idx->index();
				}
				bad_pf(sv,len);
			}
		};

		struct UseTuple {
			PMINLINE static void use_tuple(CommandMaker::delivery& del,double mn,double mx,double a,double p,unsigned char b,float g)
			{
				if(mn>=mx)
				{
					throw std::invalid_argument("Min angle must be less than max angle");
				}
				if(mx-mn>180)
				{
					throw std::invalid_argument("Difference between angles must be less than or equal to 180");
				}
				del.pl.add_process<Straighten>(p,mn,mx,a,b,g);
			}
		};
		extern
			SingMaker<UseTuple,LabelId,
			DoubleParser<MinAngle,no_check>,DoubleParser<MaxAngle,no_check>,
			DoubleParser<AnglePrec>,DoubleParser<PixelPrec>,
			IntegerParser<unsigned char,Boundary>,FloatParser<Gamma>>
			maker;
	}

	namespace CGMaker {

		struct UseTuple {
			static PMINLINE void use_tuple(CommandMaker::delivery& del)
			{
				del.pl.add_process<ChangeToGrayscale>();
			}
		};
		extern SingMaker<UseTuple,empty> maker;
	}

	namespace FRMaker {

		struct Left {
			cnnm("left")
		};
		struct Top {
			cnnm("top")
		};
		struct flagged {
			bool flag=false;
			signed int value;
		};
		struct Right {
			cnnm("horizontal extent")
				PMINLINE static flagged parse(char const* sv,size_t prefix)
			{
				struct Coord {
					cnnm("right")
				};
				struct Width {
					cnnm("width")
				};
				char const* to_parse=sv+prefix+1;
				if(prefix!=-1&&sv[0]=='w')
				{
					return {true,IntegerParser<int,Coord,no_check>().parse(to_parse)};
				}
				return {false,IntegerParser<int,Width,no_negatives>().parse(to_parse)};
			}
		};

		struct Bottom {
			cnnm("vertical extent")
				PMINLINE static flagged parse(char const* sv,size_t prefix)
			{
				struct Coord {
					cnnm("bottom")
				};
				struct Height {
					cnnm("height")
				};
				char const* to_parse=sv+prefix+1;
				if(prefix!=-1&&sv[0]=='h')
				{
					return {true,IntegerParser<int,Coord,no_check>().parse(to_parse)};
				}
				return {false,IntegerParser<int,Height,no_negatives>().parse(to_parse)};
			}
		};

		struct color {
			unsigned int num_layers;
			std::array<unsigned char,4> data;
			PMINLINE constexpr color():num_layers(5),data{{255,255,255,255}}
			{}
		};

		struct Color {
			cnnm("color")
				cndf(color())
				PMINLINE static color parse(char const* data)
			{
				auto error=[](auto res,auto name)
				{
					if(res.ec==std::errc::invalid_argument)
					{
						std::string err_msg("Invalid argument for");
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
				auto last=sv.data()+sv.length();
				auto res=std::from_chars(sv.data(),last,clr.data[0]);
				error(res,"r/gray value");
				auto comma=sv.find(',');
				if(comma==decltype(sv)::npos)
				{
					find_invalids(res.ptr,last);
					clr.num_layers=1;
					return clr;
				}
				find_invalids(res.ptr,sv.data()+comma);

				res=std::from_chars(sv.data()+comma+1,last,clr.data[1]);
				error(res,"g value");
				comma=sv.find(',',comma+1);
				if(comma=decltype(sv)::npos)
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
		};

		struct Origin {
			cnnm("origin")
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
							break;
						}
					default:
						error();
				}
				return FillRectangle::origin_reference(ret);
			}
		};

		struct UseTuple {
			PMINLINE static void use_tuple(CommandMaker::delivery& del,std::tuple<int,int,flagged,flagged,color,FillRectangle::origin_reference> const& args)
			{
				auto const& left=std::get<0>(args);
				auto const& top=std::get<1>(args);
				auto const& right=std::get<2>(args);
				auto const& bottom=std::get<3>(args);
				if(!right.flag)
				{
					if(right.value<=left)
						throw std::invalid_argument("Right coord must be greater than left coord");
				}
				else
				{
					if(right.value<0)
					{
						throw std::invalid_argument("Width cannot be negative");
					}
				}
				if(!bottom.flag)
				{
					if(bottom.value<=top)
						throw std::invalid_argument("Bottom coord must be greater than top coord");
				}
				else
				{
					if(bottom.value<0)
					{
						throw std::invalid_argument("Height cannot be negative");
					}
				}
				ImageUtils::Rectangle<int> rect;
				rect.left=std::get<0>(args);
				rect.top=std::get<1>(args);
				rect.right=right.flag?left+right.value:right.value;
				rect.bottom=bottom.flag?top+bottom.value:bottom.value;
				del.pl.add_process<FillRectangle>(rect,std::get<4>(args).data,std::get<4>(args).num_layers,std::get<5>(args));
			}
		};

		struct LabelId {
			PMINLINE static size_t id(InputType sv,size_t len)
			{
				using le=lookup_entry;
				static constexpr auto table=make_ltable(std::array<le,16>(
					{le("b",3),le("bottom",3),
					le("clr",4),le("color",4),
					le("h",3),le("height",3),
					le("l",0),le("left",0),
					le("o",5),le("or",5),
					le("r",2),le("right",2),
					le("t",1),le("top",1),
					le("w",2),le("width",2)}));
				auto idx=lfind(table,sv);
				if(idx!=table.end())
				{
					return idx->index();
				}
				bad_pf(sv,len);
			}
		};

		extern
			SingMaker<UseTuple,LabelId,IntegerParser<int,Left>,IntegerParser<int,Top>,Right,Bottom,Color,Origin>
			maker;
	}

	namespace GamMaker {
		struct Name {
			cnnm("gamma value")
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
			cnnm("angle")
		};
		struct Radians:public RadName {
			PMINLINE static float parse(char const* sv,size_t len)
			{
				if(len==0)
				{
					return FloatParser<RadName,no_check>().parse(sv);
				}
				auto const c=sv[0];
				sv+=1;
				if(c=='d')
				{
					return FloatParser<RadName,no_check>().parse(sv);
				}
				return FloatParser<RadName,no_check>().parse(sv)*RAD_DEG;
			}
		};
		struct GammaP {
			cnnm("gamma")
				cndf(float(2))
		};
		struct LabelId {
			PMINLINE static size_t id(InputType sv,size_t len)
			{
				static constexpr auto table=make_ltable(
					le("deg",0),le("rad",0),le("r",0),le("d",0),le("g",2),le("gam",2),le("im",1),le("i",1)
				);
				auto idx=lfind(table,sv);
				if(idx!=table.end())
				{
					return 0;
				}
				bad_pf(sv,len);
			}
		};

		struct Mode {
			cndf(Rotate::interp_mode(Rotate::cubic))
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
			cnnm("interpolation mode")
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

		using GammaParser=FloatParser<GammaP,no_negatives>;

		extern
			SingMaker<UseTuple,LabelId,Radians,Mode,GammaParser> maker;
	}

	namespace RsMaker {
		struct Factor {
			cnnm("factor")
		};

		struct Mode {
			PMINLINE static constexpr Rescale::rescale_mode def_val()
			{
				return Rescale::automatic;
			}
			static Rescale::rescale_mode parse(char const* mode_string)
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
			cnnm("interpolation mode")
		};

		struct LabelId {
			PMINLINE static size_t id(InputType in,size_t len)
			{
				static constexpr auto table=make_ltable(le("f",0),le("fact",0),le("i",1),le("im",1),le("g",2),le("gam",2));
				auto idx=lfind(table,in);
				if(idx!=table.end())
				{
					return idx->index();
				}
				bad_pf(in,len);
			}
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
		extern
			SingMaker<UseTuple,LabelId,FloatParser<Factor,no_negatives>,Mode,RotMaker::GammaParser>
			maker;
	}

	namespace FGMaker {
		struct Min {
			cnnm("min brightness")
		};
		struct Max {
			cnnm("max brightness")
				cndf(unsigned char(255))
		};
		struct Replacer {
			cnnm("replacer")
				cndf(unsigned char(255))
		};
		struct LabelId {
			PMINLINE static size_t id(InputType data,size_t len)
			{
				static constexpr auto table=make_ltable(
					le("mn",0),le("min",0),le("mnv",0),
					le("mx",1),le("max",1),le("mxv",1),
					le("r",2),le("rep",2));
				
				auto idx=lfind(table,data);
				if(idx!=table.end())
				{
					return idx->index();
				}
				bad_pf(data,len);
			}
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
			SingMaker<UseTuple,LabelId,
			IntegerParser<unsigned char,Min>,
			IntegerParser<unsigned char,Max>,
			IntegerParser<unsigned char,Replacer>> maker;
	}

	namespace BSel {

		class BoundSelMaker:public CommandMaker {
		public:
			BoundSelMaker():CommandMaker("Files between the begin and end are included in the list of files to process","Boundary Select","first_file1 last_file1 ... first_filen last_filen")
			{}
			void make_command(iter begin,iter end,delivery& del) override
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
			compair("str",&StrMaker::maker),
			compair("rot",&RotMaker::maker),
			compair("fr",&FRMaker::maker));

		constexpr auto mcl=exlib::make_array<compair>();

		constexpr auto ol=exlib::make_array<compair>(
			compair("o",&Output::maker),
			compair("vb",&Verbosity::maker),
			compair("nt",&NumThreads::maker),
			compair("bsel",&BSel::maker));

		constexpr auto aliases=exlib::make_array<compair>(compair("rotate",&RotMaker::maker));

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
