#ifndef NEURAL_NET_H
#define NEURAL_NET_H
#ifndef NEURALNET_H
#define NEURALNET_H
#include <memory>
#include <array>
#include <utility>
#include <vector>
#include <assert.h>
#include <random>
#include <iostream>
#include <algorithm>
#include <ratio>
#include <fstream>
#include <string>
namespace neural_net {

	using DataType=float;
	template<typename TOut,typename TIn1,typename TIn2,typename Op>
	void element_wise_op(TOut dst,TIn1 in1,TIn2 in2,size_t elems,Op op)
	{
		for(size_t i=0;i<elems;++i)
		{
			dst[i]=op(in1[i],in2[i]);
		}
	}

	inline void el_mult(DataType* dst,DataType const* in,DataType const* in2,size_t n)
	{
		element_wise_op(dst,in,in2,n,[](DataType a,DataType b)
		{
			return a*b;
		});
	}

	inline void el_add(DataType* dst,DataType const* in,DataType const* in2,size_t n)
	{
		element_wise_op(dst,in,in2,n,[](DataType a,DataType b)
		{
			return a+b;
		});
	}

	inline void matrix_fill(DataType* const dst,DataType const value,size_t dim1,size_t dim2=1)
	{
		size_t const limit=dim1*dim2;
		for(size_t i=0;i<limit;++i)
		{
			dst[i]=value;
		}
	}

	inline DataType dot(DataType const* a,DataType const* b,size_t n)
	{
		DataType sum=0;
		for(size_t i=0;i<n;++i)
		{
			sum+=a[i]*b[i];
		}
		return sum;
	}

	//dst += mat * col
	inline void matrix_t_col(DataType* const dst,DataType const* const mat,DataType const* const col,size_t const rows,size_t const cols)
	{
		for(size_t r=0;r<rows;++r)
		{
			auto const offset=r*cols;
			auto const mat_row=mat+offset;
			for(size_t c=0;c<cols;++c)
			{
				dst[r]+=mat_row[c]*col[c];
			}
		}
	}

	//dst += (mat)T * col
	inline void matrix_trans_t_col(DataType* const dst,DataType const* const mat,DataType const* const col,size_t const rows,size_t const cols)
	{
		for(size_t r=0;r<rows;++r)
		{
			auto const offset=r*cols;
			auto const mat_row=mat+offset;
			for(size_t c=0;c<cols;++c)
			{
				dst[c]+=mat_row[c]*col[r];
			}
		}
	}

	inline DataType sigmoid(DataType x)
	{
		return 1.0/(1.0+exp(-x));
	}

	struct sigmoid_deriv_t {
		inline DataType operator()(DataType y) const
		{
			float x=sigmoid(y);
			return x*(1-x);
		}
	};

	struct sigmoid_t {
		using derivative=sigmoid_deriv_t;
		inline DataType operator()(DataType x) const
		{
			return sigmoid(x);
		}
	};

	inline DataType softplus(DataType x)
	{
		return log(1+exp(x));
	}

	struct softplus_deriv_t {
		inline DataType operator()(DataType x) const
		{
			return sigmoid(x);
		}
	};

	struct softplus_t {
		using derivative=softplus_deriv_t;
		inline DataType operator()(DataType x) const
		{
			return softplus(x);
		}
	};

	struct relu_deriv_t {
		inline DataType operator()(DataType x) const
		{
			if(x>0)
			{
				return 1;
			}
			return 0;
		}
	};
	struct relu_t {
		using derivative=relu_deriv_t;
		inline DataType operator()(DataType x) const
		{
			if(x>0)
			{
				return x;
			}
			return 0;
		}
	};

	//Leak should be an std::ratio or other class that defines num and den
	template<typename Leak>
	struct leaky_relu_deriv_t {
		inline DataType operator()(DataType x) const
		{
			if(x>0)
			{
				return 1;
			}
			return static_cast<DataType>(Leak::num)/Leak::den;
		}
	};
	//Leak should be an std::ratio or other class that defines num and den
	template<typename Leak=std::ratio<1,32>>
	struct leaky_relu_t {
		using derivative=leaky_relu_deriv_t<Leak>;
		inline DataType operator()(DataType x) const
		{
			if(x>0)
			{
				return x;
			}
			return (static_cast<DataType>(Leak::num)/Leak::den)*x;
		}
	};

	template<typename Leak,typename Clip>
	struct clipped_leaky_relu_deriv_t {
		static constexpr DataType clip=(static_cast<DataType>(Clip::num)/Clip::den);
		static constexpr DataType leak=(static_cast<DataType>(Leak::num)/Leak::den);
		inline DataType operator()(DataType x) const
		{
			if(x>clip||x<0)
			{
				return leak;
			}
			return 1;
		}
	};


	template<typename Leak=std::ratio<1,32>,typename Clip=std::ratio<1,1>>
	struct clipped_leaky_relu_t {
		using derivative=clipped_leaky_relu_deriv_t<Leak,Clip>;
		static constexpr DataType clip=derivative::clip;
		static constexpr DataType leak=derivative::leak;
		inline DataType operator()(DataType x) const
		{
			if(x>clip)
			{
				constexpr DataType off=(1-leak)*clip;
				return off+x*leak;
			}
			if(x<0)
			{
				return leak*x;
			}
			return x;
		}
	};

	struct layer {
	private:
		std::unique_ptr<DataType[]> _weights;
		std::unique_ptr<DataType[]> _biases;
		size_t _count;
	public:
		inline layer(layer&&)=default;
		inline layer& operator=(layer&&)=default;
		inline layer(size_t nodes):_weights(),_biases(),_count(nodes)
		{}
		inline layer(size_t nodes,size_t connections):_weights(new DataType[nodes*connections]),_biases(new DataType[nodes]),_count(nodes)
		{}
		inline size_t neuron_count() const
		{
			return _count;
		}
		inline DataType const* weights() const
		{
			return _weights.get();
		}
		inline DataType* weights()
		{
			return _weights.get();
		}
		inline DataType* biases()
		{
			return _biases.get();
		}
		inline DataType const* biases() const
		{
			return _biases.get();
		}
	};

	inline std::vector<layer> construct_layers(std::vector<layer> const& layers)
	{
		std::vector<layer> ret;
		ret.reserve(layers.size());
		ret.emplace_back(layers.front().neuron_count());
		for(size_t i=1;i<layers.size();++i)
		{
			ret.emplace_back(layers[i].neuron_count(),layers[i-1].neuron_count());
		}
		return ret;
	}

	struct results:public std::unique_ptr<std::unique_ptr<DataType[]>[]> {
		using layer_results=std::unique_ptr<DataType[]>;
		using base=std::unique_ptr<layer_results[]>;
		inline results(std::vector<layer> const& layers):
			base(new layer_results[layers.size()])
		{
			for(size_t i=0;i<layers.size();++i)
			{
				(*this)[i]=layer_results(new DataType[2*layers[i].neuron_count()]);
			}
		}
	};

	struct delta_t:public std::unique_ptr<std::unique_ptr<DataType[]>[]> {
		using layer_results=std::unique_ptr<DataType[]>;
		using base=std::unique_ptr<layer_results[]>;
		inline delta_t(std::vector<layer> const& layers):
			base(new layer_results[layers.size()])
		{
			for(size_t i=1;i<layers.size();++i)
			{
				(*this)[i]=layer_results(new DataType[layers[i].neuron_count()]);
			}
		}
		struct init0_t {};
		inline delta_t(std::vector<layer> const& layers,init0_t):
			base(new layer_results[layers.size()])
		{
			for(size_t i=1;i<layers.size();++i)
			{
				(*this)[i]=layer_results(new DataType[layers[i].neuron_count()]());
			}
		}
	};

	template<typename ActivationFunc=clipped_leaky_relu_t<>,typename Deriv=typename ActivationFunc::derivative>
	struct net:private ActivationFunc,private Deriv {
	private:
		std::vector<layer> _layers;
	public:
		inline std::vector<layer>& layers()
		{
			return _layers;
		}
		inline std::vector<layer> const& layers() const
		{
			return _layers;
		}
		net()=default;
		net(net const& other)
		{
			auto const& l=other.layers();
			_layers=construct_layers(l);
			for(size_t i=1;i<l.size();++i)
			{
				std::memcpy(_layers[i].biases(),l[i].biases(),l[i].neuron_count()*sizeof(float));
				std::memcpy(_layers[i].weights(),l[i].weights(),l[i].neuron_count()*l[i-1].neuron_count()*sizeof(float));
			}
		}
		net(net&&)=default;
		net& operator=(net const& other)
		{
			return (*this=std::move(net(other)));
		}
		net& operator=(net&&)=default;
		template<typename A,typename B>
		friend std::ostream& operator<<(std::ostream&,net<A,B> const&);
		net& randomize()
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			std::normal_distribution<DataType> urd(0,1);
			for(size_t i=1;i<_layers.size();++i)
			{
				size_t const limit=_layers[i].neuron_count()*_layers[i-1].neuron_count();
				for(size_t j=0;j<limit;++j)
				{
					_layers[i].weights()[j]=urd(gen);
				}
				for(size_t j=0;j<_layers[i].neuron_count();++j)
				{
					_layers[i].biases()[j]=urd(gen);
				}
			}
			return *this;
		}

	private:
		template<typename T>
		struct is_iterable {
		private:
			template<typename U>
			static constexpr auto val(int)->decltype(std::declval<U>().begin(),std::declval<U>().size(),false)
			{
				return true;
			}
			template<typename U>
			static constexpr bool val(...)
			{
				return false;
			}
		public:
			static constexpr bool value=val<T>(0);
		};
	public:
		template<typename SizeIter>
		net(SizeIter begin,size_t elements)
		{
			if(elements<2)
			{
				throw std::invalid_argument("Need at least 2 layers");
			}
			_layers.reserve(elements);
			_layers.emplace_back(*begin);
			for(size_t i=1;i<elements;++i)
			{
				_layers.emplace_back(begin[i],begin[i-1]);
			}
		}

		template<typename T>
		net(std::vector<T> const& layer_info):net(layer_info.begin(),layer_info.size())
		{}
		template<typename T>
		net(std::initializer_list<T> layer_info):net(layer_info.begin(),layer_info.size())
		{}
		template<typename T,size_t N>
		net(std::array<T,N> const& layer_info):net(layer_info.begin(),layer_info.size())
		{}

		template<typename... T>
		net(size_t a,size_t b,T... ts):net(std::initializer_list<size_t>{a,b,static_cast<size_t>(ts)...})
		{}

		template<typename FloatIter>
		void feed_forward_store(results& ret,FloatIter input) const
		{
			for(size_t i=0;i<_layers.front().neuron_count();++i)
			{
				ret[0][i]=input[i];
			}
			for(size_t i=1;i<_layers.size();++i)
			{
				auto const connections=_layers[i-1].neuron_count();
				auto const& weights=_layers[i];
				auto const nc=weights.neuron_count();
				auto& dst=ret[i];
				auto& src=ret[i-1];
				//initialize with weights
				//std::copy(weights.biases(),weights.biases()+nc,dst.get());
				std::memcpy(dst.get()+nc,weights.biases(),nc*sizeof(DataType));
				//calculate z
				matrix_t_col(dst.get()+nc,weights.weights(),src.get(),nc,connections);
				//calculate Act(z)
				std::transform(dst.get()+nc,dst.get()+size_t{2}*nc,dst.get(),[this](DataType f)
				{
					return ActivationFunc::operator()(f);
				});
				/*for(size_t j=0;j<weights.neuron_count();++j)
				{
				auto const weight_row=weights.weights()+j*connections;
				DataType sum=weights.biases()[j];
				for(size_t k=0;k<connections;++k)
				{
				sum+=src[k]*weight_row[k];
				}
				dst[j]=sigmoid(sum);
				}*/
			}
		}

		template<typename FloatIter>
		results feed_forward_store(FloatIter input) const
		{
			results ret(_layers);
			feed_forward_store(ret,input);
			return ret;
		}

		void update_weights(DataType* weights,DataType* biases,DataType const* activations,DataType const* deltas,size_t far_nodes,size_t near_nodes,DataType learning_rate)
		{
			for(size_t j=0;j<far_nodes;++j)
			{
				DataType const adjusted_rate=deltas[j]*learning_rate;
				assert(!std::isnan(adjusted_rate));
				biases[j]-=adjusted_rate;
				DataType* weight_row=weights+j*near_nodes;
				for(size_t k=0;k<near_nodes;++k)
				{
					weight_row[k]-=activations[k]*adjusted_rate;
				}
			}
		}

		void update_weights(layer const* gradients)
		{
			for(size_t i=1;i<_layers.size();++i)
			{
				size_t limit=_layers[i-1].neuron_count()*_layers[i].neuron_count();
				for(size_t j=0;j<limit;++j)
				{
					_layers[i].weights()[j]-=gradients[i].weights()[j];
				}
				for(size_t j=0;j<_layers[i].neuron_count();++j)
				{
					_layers[i].biases()[j]-=gradients[j].biases()[j];
				}
			}
		}

		void update_weights(layer const* gradients,float learning_rate)
		{
			for(size_t i=1;i<_layers.size();++i)
			{
				size_t limit=_layers[i-1].neuron_count()*_layers[i].neuron_count();
				for(size_t j=0;j<limit;++j)
				{
					_layers[i].weights()[j]-=gradients[i].weights()[j]*learning_rate;
				}
				for(size_t j=0;j<_layers[i].neuron_count();++j)
				{
					_layers[i].biases()[j]-=gradients[i].biases()[j]*learning_rate;
				}
			}
		}

		void calculate_grads(layer* out,delta_t const& deltas,results const& activations) const
		{
			for(size_t i=_layers.size()-1;i>0;)
			{
				size_t prev=i-1;
				DataType* const weights=out[i].weights();
				DataType* const biases=out[i].biases();
				DataType const* const acts=activations[prev].get();
				DataType const* const dels=deltas[i].get();
				size_t far_nodes=_layers[i].neuron_count();
				size_t near_nodes=_layers[prev].neuron_count();
				for(size_t j=0;j<far_nodes;++j)
				{
					DataType const adjusted_rate=dels[j];
					assert(!std::isnan(adjusted_rate));
					biases[j]=dels[j];
					DataType* weight_row=weights+j*near_nodes;
					for(size_t k=0;k<near_nodes;++k)
					{
						weight_row[k]=acts[k]*dels[j];
					}
				}
				i=prev;
			}
		}

		void update_weights(delta_t const& deltas,results const& activations,DataType learning_rate)
		{
			for(size_t i=_layers.size()-1;i>0;)
			{
				size_t prev=i-1;
				update_weights(
					_layers[i].weights(),
					_layers[i].biases(),
					activations[prev].get(),
					deltas[i].get(),
					_layers[i].neuron_count(),
					_layers[prev].neuron_count(),
					learning_rate);
				i=prev;
			}
		}

		void calculate_deltas(delta_t& deltas,results const& values,DataType const* answers) const
		{
			size_t const last=_layers.size()-1;
			auto const c=_layers[last].neuron_count();
			for(size_t j=0;j<c;++j)
			{
				DataType const a=values[last][j];
				DataType const dc_da=a-answers[j];
				DataType const deriv=Deriv::operator()(values[last][j+c]);
				deltas[last][j]=dc_da*deriv;
				assert(!std::isnan(deltas[last][j]));
			}
			for(size_t i=last;i>1;)
			{
				auto const prev=i-1;
				auto& delta_prev=deltas[prev];
				auto& delta=deltas[i];
				auto const nc=_layers[i].neuron_count();
				auto const pnc=_layers[prev].neuron_count();
				//initialize delta_prev to zero
				std::fill(delta_prev.get(),delta_prev.get()+pnc,0.0f);
				//calculate first part of delta_prev
				matrix_trans_t_col(delta_prev.get(),_layers[i].weights(),delta.get(),nc,pnc);
				//calculate second part of delta_prev
				for(size_t j=0;j<pnc;++j)
				{
					delta_prev[j]*=Deriv::operator()(values[prev][j+pnc]);
				}
				i=prev;
			}
		}

		void backpropagate(results const& values,DataType const* answers,DataType learning_rate=1)
		{
			delta_t deltas(_layers);
			calculate_deltas(deltas);
			update_weights(deltas,values,learning_rate);
		}

		void train(DataType const* input_data,DataType const* answers,size_t num_inputs,DataType learning_rate=1)
		{
			if(num_inputs==0)
			{
				return;
			}
			auto const out_count=_layers.back().neuron_count();
			auto const in_count=_layers.front().neuron_count();
			results res(_layers);
			delta_t dels(_layers);
			std::vector<layer> average=construct_layers(_layers),single=construct_layers(_layers);
			for(size_t i=1;i<average.size();++i)
			{
				size_t limit=average[i-1].neuron_count()*average[i].neuron_count();
				for(size_t j=0;j<limit;++j)
				{
					average[i].weights()[j]=0;
				}
				for(size_t j=0;j<average[i].neuron_count();++j)
				{
					average[i].biases()[j]=0;
				}
			}
			for(size_t n=0;n<num_inputs;++n)
			{
				auto const in_offset=in_count*n;
				auto const out_offset=out_count*n;
				feed_forward_store(res,input_data+in_offset);
				calculate_deltas(dels,res,answers+out_offset);
				calculate_grads(single.data(),dels,res);
				for(size_t i=1;i<_layers.size();++i)
				{
					size_t limit=average[i-1].neuron_count()*average[i].neuron_count();
					for(size_t j=0;j<limit;++j)
					{
						average[i].weights()[j]+=single[i].weights()[j];
					}
					for(size_t j=0;j<average[i].neuron_count();++j)
					{
						average[i].biases()[j]+=single[i].weights()[j];
					}
				}
			}
			auto adjustment=learning_rate/num_inputs;
			for(size_t i=1;i<_layers.size();++i)
			{
				size_t limit=average[i-1].neuron_count()*average[i].neuron_count();
				for(size_t j=0;j<limit;++j)
				{
					average[i].weights()[j]*=adjustment;
				}
				for(size_t j=0;j<average[i].neuron_count();++j)
				{
					average[i].biases()[j]*=adjustment;
				}
			}
			update_weights(average.data());
		}
		template<typename Stream>
		auto save(Stream& file) -> typename std::enable_if<!std::is_convertible<Stream,char const*>::value>::type
		{
			auto bwrite=[&](auto a)
			{
				file.write(reinterpret_cast<char const*>(&a),sizeof(decltype(a)));
			};
			auto bwrite_row=[&](auto const* a,size_t count)
			{
				file.write(reinterpret_cast<char const*>(a),count*sizeof(decltype(*a)));
			};
			bwrite(uint64_t{_layers.size()});
			for(auto const& l:_layers)
			{
				bwrite(uint64_t{l.neuron_count()});
			}
			for(size_t i=1;i<_layers.size();++i)
			{
				auto const rows=_layers[i].neuron_count();
				auto const cols=_layers[i-1].neuron_count();
				bwrite_row(_layers[i].biases(),rows);
				auto const data=_layers[i].weights();
				bwrite_row(data,cols*rows);
			}
			bwrite('\0');
		}
		void save(char const* path)
		{
			std::ofstream file(path,std::ios::out|std::ios::binary);
			if(file)
			{
				save(file);
			}
			else
			{
				throw std::runtime_error("Failed to open path");
			}
		}

		template<typename Stream>
		auto load(Stream& src) -> typename std::enable_if<!std::is_convertible<Stream,char const*>::value>::type
		{
			auto ensure_fit=[](uint64_t s)
			{
				if constexpr(!std::is_same_v<uint64_t,size_t>)
				{
					if(s>uint64_t{std::numeric_limits<size_t>::max()})
					{
						throw std::invalid_argument("Net sizes too large.");
					}
				}
				return static_cast<size_t>(s);
			};
			constexpr auto s=sizeof(uint64_t);
			uint64_t layer_count;
			uint64_t size;
			char* buffer=reinterpret_cast<char*>(&size);
			src.read(reinterpret_cast<char*>(&layer_count),s);
			ensure_fit(layer_count);
			if(layer_count<=1)
			{
				throw std::runtime_error("Invalid number of layers; must be >= 2");
			}
			std::vector<layer> layers;
			layers.reserve(layer_count);
			src.read(buffer,s);
			layers.emplace_back(ensure_fit(size));
			for(size_t i=1;i<layer_count;++i)
			{
				src.read(buffer,s);
				layers.emplace_back(ensure_fit(size),layers[i-1].neuron_count());
			}
			for(size_t i=1;i<layer_count;++i)
			{
				auto const p=layers[i-1].neuron_count();
				auto const c=layers[i].neuron_count();
				auto const b=layers[i].biases();
				auto const w=layers[i].weights();
				src.read(reinterpret_cast<char*>(b),c*sizeof(float));
				src.read(reinterpret_cast<char*>(w),p*c*sizeof(float));
			}
			_layers=std::move(layers);
		}
		void load(char const* path)
		{
			std::ifstream e(path,std::ios::in|std::ios::binary);
			if(e)
			{
				load(e);
			}
			else
			{
				throw std::runtime_error("Failed to open path");
			}
		}

		net(std::ifstream& src)
		{
			load(src);
		}
		net(std::fstream& src)
		{
			load(src);
		}
		net(char const* path)
		{
			load(path);
		}
	};

	template<size_t len=15>
	std::string fix_length(DataType f)
	{
		auto str=std::to_string(f);
		if(str[0]!='-')
		{
			str.insert(str.begin(),' ');
		}
		if(str.size()<len)
		{
			str.append("                ",len-str.size());
		}
		return str;
	};

	template<typename AF,typename D>
	std::ostream& operator<<(std::ostream& os,net<AF,D> const& n)
	{
		auto const& layers=n._layers;
		os<<layers.size()<<" layers\n";
		for(size_t i=1;i<layers.size();++i)
		{
			size_t const p=i-1;
			auto const& layer=layers[i];
			auto const rows=layers[i].neuron_count();
			auto const cols=layers[p].neuron_count();
			os<<"Layer ";
			os<<p;
			os<<" to ";
			os<<i;
			os<<": ";
			os<<rows<<" rows and "<<cols<<" columns:\n";
			for(size_t r=0;r<rows;++r)
			{
				os<<'[';
				auto const row=layer.weights()+r*cols;
				for(size_t c=0;c<cols;++c)
				{
					os<<fix_length(row[c]);
				}
				os<<"]   [";
				os<<fix_length(layer.biases()[r])<<"]\n";
			}
		}
		return os;
	}
}
#endif
#endif