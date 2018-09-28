#ifndef NEURAL_SCALER_H
#define NEURAL_SCALER_H

#include "neural_net.h"
#include <thread>
#include "../ScoreProcessor/CImg.h"
#include <type_traits>
namespace ScoreProcessor {
	template<typename Base>
	struct smart_scaler_base {
		auto& smart_scale(cil::CImg<unsigned char>& img,float scale,unsigned int num_threads=std::thread::hardware_concurrency()) const
		{
			unsigned int new_x=std::round(img._width*scale);
			unsigned int new_y=std::round(img._height*scale);
			if(new_x==img._width&&new_y==img._height)
			{
				return img;
			}
			img=static_cast<Base*>(this)->get_smart_scale(img,scale,num_threads);
			return img;
		}
	};

	struct neural_scaler:smart_scaler_base<neural_scaler> {
	private:
		unsigned int _nscale;
		unsigned int _out_dim;
		unsigned int _in_dim;
		neural_net::net<> _net;
	
		static unsigned int int_sqrt(size_t a,char const* msg)
		{
			auto sqrt=std::sqrt(a);
			auto r=std::round(sqrt);
			if(r-sqrt!=0)
			{
				throw std::invalid_argument(msg);
			}
			return static_cast<unsigned int>(r);
		};

		template<typename Net>
		static auto assert_dim(unsigned int s,Net&& src)
		{
			auto o=int_sqrt(src.layers().back().neuron_count(),"Invalid output dim, must be square");
			auto i=int_sqrt(src.layers().front().neuron_count(),"Invalid input dim, must be square");
			if(o%s)
			{
				throw std::invalid_argument("Output dim must be a multiple of scale factor");
			}
			if(i*s<o)
			{
				throw std::invalid_argument("Scaled-up input cannot be smaller than output");
			}
			if((i-o/s)%2)
			{
				throw std::invalid_argument("Symmetrical padding around input required");
			}
			return std::tuple<decltype(o),decltype(i),Net&&>(o,i,std::forward<Net>(src));
		}
	public:
		auto& net()
		{
			return _net;
		}
		auto const& net() const
		{
			return _net;
		}
		inline neural_scaler(unsigned int scale,neural_net::net<> const& src):_nscale(scale)
		{
			auto res=assert_dim(scale,src);
			_net=std::get<2>(res);
			_out_dim=std::get<0>(res);
			_in_dim=std::get<1>(res);
		}
		inline neural_scaler(unsigned int scale,neural_net::net<>&& src):_nscale(scale)
		{
			auto res=assert_dim(scale,src);
			_net=std::get<2>(res);
			_out_dim=std::get<0>(res);
			_in_dim=std::get<1>(res);
		}
		cil::CImg<unsigned char> get_smart_scale(cil::CImg<unsigned char> const& img,float scale,unsigned int num_threads=std::thread::hardware_concurrency()) const;

	private:
		struct is_writable_h {
			template<typename U>
			static constexpr auto val(int)->decltype(std::declval<U>().write(std::declval<char const*>(),size_t()),false)
			{
				return true;
			}
			template<typename U>
			static constexpr auto val(...)
			{
				return false;
			}
		};

		template<typename T>
		struct is_writable:std::integral_constant<bool,is_writable_h::val<T>(0)> {

		};

		struct is_readable_h {
			template<typename U>
			static constexpr auto val(int)->decltype(std::declval<U>().read(std::declval<char*>(),size_t()),false)
			{
				return true;
			}
			template<typename U>
			static constexpr auto val(...)
			{
				return false;
			}
		};

		template<typename T>
		struct is_readable:std::integral_constant<bool,is_readable_h::val<T>(0)> {

		};
	public:
		
		template<typename Stream>
		auto save(Stream& src) -> typename std::enable_if<is_writable<Stream>::value>::type
		{
			uint64_t scale=_nscale;
			src.write(reinterpret_cast<char const*>(&scale),sizeof(uint64_t));
			_net.save(src);
		}
		void save(char const* path)
		{
			std::ofstream e(path,std::ios::out|std::ios::binary);
			if(e)
			{
				return save(e);
			}
			throw std::runtime_error("Failed to save");
		}
		
		template<typename Stream>
		auto load(Stream& src) -> typename std::enable_if<is_readable<Stream>::value>::type
		{
			uint64_t scale;
			src.read(reinterpret_cast<char*>(&scale),sizeof(scale));
			neural_net::net<> net(src);
			auto id=int_sqrt(net.layers().front().neuron_count(),"Invalid input dim");
			auto od=int_sqrt(net.layers().front().neuron_count(),"Invalid input dim");
			_nscale=static_cast<unsigned int>(scale);
			_in_dim=id;
			_out_dim=od;
			_net=std::move(net);
		}
		void load(char const* path)
		{
			std::ifstream e(path,std::ios::in|std::ios::binary);
			if(e)
			{
				return load(e);
			}
			throw std::runtime_error("Failed to load");
		}
		inline unsigned int input_dim() const
		{
			return _in_dim;
		}
		inline unsigned int output_dim() const
		{
			return _out_dim;
		}
		inline unsigned int scale_factor() const
		{
			return _nscale;
		}
		inline void feed(float* out,float const* in) const
		{
			auto res=_net.feed_forward_store(in);
			std::memcpy(out,res[_net.layers().size()-1].get(),_net.layers().back().neuron_count()*sizeof(float));
		}
		template<typename Stream>
		neural_scaler(Stream& src)
		{

		}
	private:
		static void place_values(cil::CImg<unsigned char>& img,float const* in,unsigned int x,unsigned int y,unsigned int dim);
	public:



	};
}

#endif