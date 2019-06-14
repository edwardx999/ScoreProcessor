/*
Copyright 2018-2019 Edward Xie

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef EXLIB_THREAD_POOL_H
#define EXLIB_THREAD_POOL_H
#include <functional>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <tuple>
#include <queue>
#include <memory>
#include <type_traits>
#include <thread>
#include <assert.h>
#include <algorithm>
#ifdef _MSVC_LANG
#define _EXLIB_THREAD_POOL_HAS_CPP_20 (_MSVC_LANG>=202000L)
#define _EXLIB_THREAD_POOL_HAS_CPP_17 (_MSVC_LANG>=201700L)
#define _EXLIB_THREAD_POOL_HAS_CPP_14 (_MSVC_LANG>=201400L)
#else
#define _EXLIB_THREAD_POOL_HAS_CPP_20 (__cplusplus>=202000L)
#define _EXLIB_THREAD_POOL_HAS_CPP_17 (__cplusplus>=201700L)
#define _EXLIB_THREAD_POOL_HAS_CPP_14 (__cplusplus>=201400L)
#endif
#if _EXLIB_THREAD_POOL_HAS_CPP_17
#define _EXLIB_THREAD_POOL_NODISCARD [[nodiscard]]
#else
#define _EXLIB_THREAD_POOL_NODISCARD 
#endif
#include <future>
namespace exlib {

	namespace thread_pool_detail {

		template<typename T>
		struct wrap_reference {
			using type=T;
		};

		template<typename T>
		struct wrap_reference<T&> {
			using type=std::reference_wrapper<T>;
		};

		template<typename T>
		using wrap_reference_t=typename wrap_reference<T>::type;

#if _EXLIB_THREAD_POOL_HAS_CPP_14
		using std::index_sequence;

		using std::make_index_sequence;
#else
		template<size_t... Is>
		struct index_sequence {};

		template<typename T,typename U>
		struct concat_index_sequence;

		template<size_t... I,size_t... J>
		struct concat_index_sequence<index_sequence<I...>,index_sequence<J...>> {
			using type=index_sequence<I...,J...>;
		};

		template<typename T,typename U>
		using concat_index_sequence_t=typename concat_index_sequence<T,U>::type;

		template<size_t I,size_t J,bool adjacent=I+1==J>
		struct make_index_sequence_h {
			static constexpr size_t H=I+(J-I)/2;
			using type=concat_index_sequence_t<typename make_index_sequence_h<I,H>::type,typename make_index_sequence_h<H,J>::type>;
		};

		template<size_t I>
		struct make_index_sequence_h<I,I,false> {
			using type=index_sequence<>;
		};

		template<size_t I,size_t J>
		struct make_index_sequence_h<I,J,true> {
			using type=index_sequence<I>;
		};

		template<size_t N>
		using make_index_sequence=typename make_index_sequence_h<0,N>::type;
#endif


#if _EXLIB_THREAD_POOL_HAS_CPP_17
		using std::apply;
#else

		template<typename Func,typename Tuple,size_t... Indices>
		void apply_h(Func&& f,Tuple&& tpl,index_sequence<Indices...>)
		{
			using std::get;
			std::forward<Func>(f)(get<Indices>(std::forward<Tuple>(tpl))...);
		}

		template<typename Func,typename Tuple>
		void apply(Func&& f,Tuple&& tpl)
		{
			apply_h(std::forward<Func>(f),std::forward<Tuple>(tpl),make_index_sequence<std::tuple_size<typename std::remove_reference<Tuple>::type>::value>{});
		}

#endif
		template<typename Func,typename FirstArg,typename Tuple,size_t... Indices>
		void apply_fa_h(Func&& f,FirstArg&& fa,Tuple&& tpl,index_sequence<Indices...>)
		{
			using std::get;
			std::forward<Func>(f)(std::forward<FirstArg>(fa),get<Indices>(std::forward<Tuple>(tpl))...);
		}

		template<typename Func,typename FirstArg,typename Tuple>
		void apply_fa(Func&& f,FirstArg&& fa,Tuple&& tpl)
		{
			apply_fa_h(std::forward<Func>(f),std::forward<FirstArg>(fa),std::forward<Tuple>(tpl),make_index_sequence<std::tuple_size<typename std::remove_reference<Tuple>::type>::value>{});
		}

#if _EXLIB_THREAD_POOL_HAS_CPP_17
		template<typename... Args>
		using no_rvalue_references=std::bool_constant<(!std::is_rvalue_reference_v<Args>&& ...)>;

		template<typename... Args>
		using all_nothrow_copyable=std::bool_constant<(std::is_nothrow_copy_constructible_v<Args>&& ...)>;
#else
		template<typename... Args>
		struct no_rvalue_references;

		template<>
		struct no_rvalue_references<>:std::true_type {};

		template<typename T,typename... Rest>
		struct no_rvalue_references<T,Rest...>:std::integral_constant<bool,!std::is_rvalue_reference<T>::value&& no_rvalue_references<Rest...>::value> {};

		template<typename... Args>
		struct all_nothrow_copyable;

		template<>
		struct all_nothrow_copyable<>:std::true_type {};

		template<typename T,typename... Rest>
		struct all_nothrow_copyable<T,Rest...>:std::integral_constant<bool,std::is_nothrow_copy_constructible<T>::value&& all_nothrow_copyable<Rest...>::value> {};
#endif

#if _EXLIB_THREAD_POOL_HAS_CPP_20
		template<typename T>
		using std::remove_cvref_t;
#else
		template<typename T>
		using remove_cvref_t=typename std::remove_cv<typename std::remove_reference<T>::type>::type;
#endif

		class joining_thread:public std::thread {
		public:
			using std::thread::thread;
			joining_thread() noexcept=default;
			joining_thread(joining_thread&&) noexcept=default;
			joining_thread& operator=(joining_thread&&) noexcept=default;
			~joining_thread() noexcept
			{
				if(this->joinable())
				{
					this->join();
				}
			}
		};

		/*
			The base of the threadpool that does not depend on special arguments.
		*/
		class thread_pool_base {
		public:
			/*
				Makes threads look for tasks. Useful after stop() has been called to reactivate job search.
			*/
			void reactivate() noexcept
			{
				_active=true;
				_signal_start.notify_all();
			}
			/*
				Makes threads stop looking for jobs.
			*/
			void stop()
			{
				_active=false;
				std::lock_guard<std::mutex> lock(_mtx);
			}

			/*
				Whether the threads should be running. Not a guarantee that threads are or are not running.
			*/
			_EXLIB_THREAD_POOL_NODISCARD bool running() const noexcept
			{
				return _running;
			}

			/*
				Whether the threads are looking for tasks.
			*/
			_EXLIB_THREAD_POOL_NODISCARD bool active() const noexcept
			{
				return _active;
			}

			/*
				Locks access to the job queue.
			*/
			void lock()
			{
				_mtx.lock();
			}
			/*
				Unlocks access to the job queue.
			*/
			void unlock()
			{
				_mtx.unlock();
			}
			/*
				Tries locking access to the job queue.
			*/
			_EXLIB_THREAD_POOL_NODISCARD bool try_lock()
			{
				return _mtx.try_lock();
			}

			/*
				Signals thread pool to stop looking for work.
			*/
			void signal_stop()
			{
				this->_active=false;
				this->_jobs_done.notify_all();
			}

			/*
				Signals thread pool to terminate.
			*/
			void signal_terminate()
			{
				stop_running();
				_jobs_done.notify_all();
			}

		protected:

			void notify_count(size_t count)
			{
				if(count==1)
				{
					_signal_start.notify_one();
				}
				else
				{
					_signal_start.notify_all();
				}
			}
			void join_all()
			{
				for(auto& thread:this->_workers)
				{
					if(thread.joinable())
					{
						thread.join();
					}
				}
			}
			void stop_running()
			{
				this->_running=false;
				this->_signal_start.notify_all();
				this->_active=false;
			}

			thread_pool_base(size_t num_threads,bool start):_workers(num_threads),_running(start),_active(start)
			{}
			std::mutex mutable _mtx;
			std::condition_variable _signal_start;
			std::condition_variable _jobs_done;
			std::vector<joining_thread> _workers;
			std::atomic<std::size_t> _active_thread_count;
			//whether threads are running
			std::atomic<bool> _running;
			//whether threads are actively looking for jobs
			std::atomic<bool> _active;
		};

		template<typename T>
		class ptr_wrapper {
			T _val;
		public:
			template<typename... Args>
			ptr_wrapper(Args&& ... args):_val(std::forward<Args>(args)...)
			{}
			T& operator*() const
			{
				return _val;
			}
			T* operator->() const
			{
				return &_val;
			}
		};

		template<typename Base,typename Functor>
		class transform_iterator:Functor {
			Base _base;
		public:
			using iterator_category=typename std::iterator_traits<Base>::iterator_category;
			using difference_type=typename std::iterator_traits<Base>::difference_type;
			using value_type=decltype(std::declval<Functor const&>()(std::declval<typename std::iterator_traits<Base>::value_type>()));
			using reference=value_type;
			using pointer=ptr_wrapper<value_type>;
			template<typename F>
			transform_iterator(Base base,F&& f):_base(base),Functor(std::forward<F>(f))
			{}
			transform_iterator(Base base):transform_iterator(base,Functor{})
			{}
#define make_op_for_gi(op)  transform_iterator operator op(difference_type d) const{return {_base op d,static_cast<Functor const&>(*this)}; } transform_iterator& operator op##=(difference_type d) {_base op##= d; return *this; }
			make_op_for_gi(-)
				make_op_for_gi(+)
#undef make_op_for_gi
				Base base() const
			{
				return _base;
			}
			difference_type operator-(transform_iterator const& o) const
			{
				return _base-o._base;
			}
			transform_iterator& operator++()
			{
				++_base;
				return *this;
			}
			transform_iterator& operator--()
			{
				--_base;
				return *this;
			}
			transform_iterator operator++(int)
			{
				auto copy(*this);
				++_base;
				return copy;
			}
			transform_iterator operator--(int)
			{
				auto copy(*this);
				--_base;
				return copy;
			}
			reference operator*() const
			{
				return Functor::operator()(*_base);
			}
			pointer operator->() const
			{
				return ptr_wrapper<value_type>(operator*());
			}
		};

#define make_comp_op_for_gi(op) template<typename Base,typename Functor> auto operator op(transform_iterator<Base,Functor> const& a,transform_iterator<Base,Functor> const& b) -> decltype(std::declval<Base const&>() op std::declval<Base const&>()) {return a.base() op b.base();}
		make_comp_op_for_gi(==)
			make_comp_op_for_gi(!=)
			make_comp_op_for_gi(<)
			make_comp_op_for_gi(>)
			make_comp_op_for_gi(<=)
			make_comp_op_for_gi(>=)
#if _EXLIB_THREAD_POOL_HAS_CPP_20
			template<typename Base,typename Functor>
		auto operator<=>(transform_iterator<Base,Functor> const& a,transform_iterator<Base,Functor> const& b) -> decltype(std::declval<Base const&>()<=>std::declval<Base const&>())
		{
			return a.base()<=>b.base();
		}
#endif
#undef make_comp_op_for_gi

		template<typename Base,typename Functor>
		transform_iterator<Base,typename std::decay<Functor>::type> make_transform_iterator(Base base,Functor&& f)
		{
			return transform_iterator<Base,typename std::decay<Functor>::type>(base,std::forward<Functor>(f));
		}

		template<typename ReturnType>
		struct set_promise_value {
			template<typename Task,typename... Args>
			static void set(std::promise<ReturnType>& promise,Task& task,Args&& ... args)
			{
				promise.set_value(task(std::forward<Args>(args)...));
			}
		};
		template<>
		struct set_promise_value<void> {
			template<typename Task,typename... Args>
			static void set(std::promise<void>& promise,Task& task,Args&& ... args)
			{
				task(std::forward<Args>(args)...);
				promise.set_value();
			}
		};

		template<bool no_throw>
		struct TaskDoerImpl {
			template<typename Task,typename ReturnType,typename... Args>
			static void run(Task& task,std::promise<ReturnType>& promise,Args&& ... args)
			{
				try
				{
					thread_pool_detail::set_promise_value<ReturnType>::set(promise,task,std::forward<Args>(args)...);
				}
				catch(...)
				{
					promise.set_exception(std::current_exception());
				}
			}
		};

		template<>
		struct TaskDoerImpl<true> {
			template<typename Task,typename ReturnType,typename... Args>
			static void run(Task& task,std::promise<ReturnType>& promise,Args&& ... args)
			{
				thread_pool_detail::set_promise_value<ReturnType>::set(promise,task,std::forward<Args>(args)...);
			}
		};

		template<typename... Args,typename Task>
		auto fake_invoke(Task task) noexcept(noexcept(task(std::declval<Args>()...))) -> decltype(task(std::declval<Args>()...));

		template<typename Task,typename... Args>
		struct TaskDoer {
			typename std::decay<Task>::type task;
			using ReturnType=decltype(fake_invoke<Args...>(std::move(task)));
			std::promise<ReturnType> promise;
			void operator()(Args... args) noexcept
			{
				TaskDoerImpl<noexcept(task(std::declval<Args>()...))>::run(task,promise,std::forward<Args>(args)...);
			}
		};

	}

	struct delay_start_t {};

#if _EXLIB_THREAD_POOL_HAS_CPP_17
	constexpr delay_start_t delay_start{};
#endif

	/*
		Returns the hardware_concurrency, unless that returns 0, in which case returns def_val.
	*/
	inline decltype(std::thread::hardware_concurrency()) hardware_concurrency_or(decltype(std::thread::hardware_concurrency()) def_val=1)
	{
		auto const nt=std::thread::hardware_concurrency();
		if(nt)
		{
			return nt;
		}
		return def_val;
	}

	namespace thread_pool_impl {
		/*
			Thread pool where the threadpool stores the given arguments and each task is given those arguments.
			There should only be one controlling thread.
			No tasks added should throw.
			Recommended using thread_pool if using a thread pool multiple times to avoid code bloat.
		*/
		template<typename... Args>
		class thread_pool_a:thread_pool_detail::thread_pool_base {
		public:
			static_assert(thread_pool_detail::no_rvalue_references<Args...>::value,"rvalue references not allowed as arguments");
			static_assert(thread_pool_detail::all_nothrow_copyable<Args...>::value,"Arguments must be no-throw copyable (thread_pool_a has no exception handling mechanism).");

			friend class parent_ref;
			/*
				A reference to the parent that child tasks can accept. Should be passed by value.
				Contains the methods safe to call by child threads.
			*/
			class parent_ref {
				friend class thread_pool_a;
				thread_pool_a& parent;
				parent_ref(thread_pool_a& p):parent(p)
				{}
			public:
				/*
					Signals threads to stop looking for tasks and will signal a waiting master thread.
				*/
				void stop()
				{
					parent.signal_stop();
				}
				/*
					See thread_pool_a::push_back
				*/
				template<typename... Tasks>
				void push_back(Tasks&& ... tasks)
				{
					parent.push_back(std::forward<Tasks>(tasks)...);
				}
				template<typename... Tasks>
				void push_back_no_sync(Tasks&& ... tasks)
				{
					parent.push_back_no_sync(std::forward<Tasks>(tasks)...);
				}
				/*
					See thread_pool_a::push_front
				*/
				template<typename... Tasks>
				void push_front(Tasks&& ... tasks)
				{
					parent.push_front(std::forward<Tasks>(tasks)...);
				}
				template<typename... Tasks>
				void push_front_no_sync(Tasks&& ... tasks)
				{
					parent.push_front_no_sync(std::forward<Tasks>(tasks)...);
				}
				/*
					See thread_pool_a::append
				*/
				template<typename Iter>
				size_t append(Iter begin,Iter end)
				{
					return parent.append(begin,end);
				}
				template<typename Iter>
				size_t append_no_sync(Iter begin,Iter end)
				{
					return parent.append_no_sync(begin,end);
				}
				/*
					See thread_pool_a::prepend
				*/
				template<typename Iter>
				size_t prepend(Iter begin,Iter end)
				{
					return parent.prepend(begin,end);
				}
				template<typename Iter>
				size_t prepend_no_sync(Iter begin,Iter end)
				{
					return parent.prepend_no_sync(begin,end);
				}
				/*
					Clears tasks.
				*/
				void clear()
				{
					parent.clear();
				}
				void clear_no_sync()
				{
					parent.clear_no_sync();
				}
				/*
					Signals the threads to end. Does not join them.
				*/
				void terminate()
				{
					parent.signal_terminate();
				}
				void lock()
				{
					parent.lock();
				}
				_EXLIB_THREAD_POOL_NODISCARD bool try_lock()
				{
					return parent.try_lock();
				}
				void unlock()
				{
					return parent.unlock();
				}
				_EXLIB_THREAD_POOL_NODISCARD bool empty() const
				{
					return parent.empty();
				}
				_EXLIB_THREAD_POOL_NODISCARD bool empty_no_sync() const
				{
					return parent.empty_no_sync();
				}
				_EXLIB_THREAD_POOL_NODISCARD size_t num_jobs() const
				{
					return parent.num_jobs();
				}
				_EXLIB_THREAD_POOL_NODISCARD size_t num_jobs_no_sync() const
				{
					return parent.num_jobs_no_sync();
				}
				_EXLIB_THREAD_POOL_NODISCARD size_t num_threads() const
				{
					return parent.num_threads();
				}
			};

			using const_parent_ref=parent_ref const;
			using thread_pool_base::reactivate;
			using thread_pool_base::active;
			using thread_pool_base::stop;
			using thread_pool_base::running;
			using thread_pool_base::lock;
			using thread_pool_base::unlock;
			using thread_pool_base::try_lock;
			using thread_pool_base::signal_stop;
			using thread_pool_base::signal_terminate;

			/*
				Starts the threadpool with a certain number of threads and arguments initialized to the given arguments.
			*/
			template<typename... T>
			explicit thread_pool_a(size_t num_threads,T&& ... args):thread_pool_base(num_threads,true),_input(std::forward<T>(args)...)
			{
				create_threads();
			}

			/*
				Initializes the threadpool with a certain number of threads and arguments initialized to the given arguments.
				Threads are not started.
			*/
			template<typename... T>
			explicit thread_pool_a(size_t num_threads,delay_start_t,T&& ... args):thread_pool_base(num_threads,false),_input(std::forward<T>(args)...)
			{}

			/*
				Starts the threadpool with number of threads equal to the hardware concurrency.
			*/
			thread_pool_a():thread_pool_a(hardware_concurrency_or(1))
			{}

			/*
				Initializes the threadpool with number of threads equal to the hardware concurrency.
				Threads not started.
			*/
			explicit thread_pool_a(delay_start_t t):thread_pool_a(hardware_concurrency_or(1),t)
			{}

			/*
				Set the args passed to the threads. Unsynchronized as you should not be modifying args that are actively being read from.
			*/
			template<typename... TplArgs>
			void set_args(TplArgs&& ... args)
			{
				_input=TaskInput(std::forward<TplArgs>(args)...);
			}

			/*
				Starts threads.
			*/
			void start()
			{
				if(!this->_running)
				{
					this->_running=true;
					this->_active=true;
					create_threads();
				}
			}

			/*
				Makes threads look for tasks. Useful after stop() has been called to reactivate job search.
				Resets the args passed to the threads.
			*/
			template<typename... TupleArgs>
			void reactivate(TupleArgs&& ... args)
			{
				set_args(std::forward<TupleArgs>(args)...);
				this->reactivate();
			}

			/*
				Waits for all jobs to be finished or for it to be stop()ed (be inactivated).
				Threads keep running (if they already were), but tasks can be added without synchronization.
			*/
			void wait()
			{
				std::unique_lock<std::mutex> lock(this->_mtx);
				this->_jobs_done.wait(lock,[this]
					{
						return this->idle();
					});
			}

			enum wait_state:bool {
				wait_no_timeout=true,wait_timeout=false
			};

			/*
				Waits for all jobs to be finished or to be stop()ed.
				Returns false if timeout expired, otherwise true.
			*/
			template<typename Rep,typename Period>
			wait_state wait_for(std::chrono::duration<Rep,Period> const& rel_time)
			{
				auto absolute_time=std::chrono::steady_clock::now()+rel_time;
				return wait_until(absolute_time);
			}

			/*
				Waits for all jobs to be finished or to be stop()ed.
				Returns false if timeout expired, otherwise true.
			*/
			template<typename Clock,typename Duration>
			wait_state wait_until(std::chrono::time_point<Clock,Duration> const& rel_time)
			{
				std::unique_lock<std::mutex> lock(this->_mtx);
				return this->_jobs_done.wait_until(lock,rel_time,[this]
					{
						return this->idle();
					});
			}

			/*
				Makes threads stop looking for jobs and ends threads.
			*/
			void terminate()
			{
				this->stop_running();
				this->join_all();
			}

			/*
				Waits for all jobs to finish and ends threads.
			*/
			void join()
			{
				join_base();
				this->join_all();
			}

			/*
				calls join() and then does normal destruction
			*/
			~thread_pool_a() noexcept //if join errors, something's wrong with the threads and program should crash
			{
				join_base();
			}

			/*
				Adds a task to the thread pool without synchronization.
				Task must define operator() that can take in Args...
				or optionally parent_ref as a first argument and then Args...
			*/
			template<typename Task>
			void push_back_no_sync(Task&& task)
			{
				this->_jobs.push_back(make_job(std::forward<Task>(task)));
			}

			/*
				Adds a task to the front of thread pool without synchronization.
				Task must define operator() that can take in Args...
				or optionally parent_ref as a first argument and then Args...
			*/
			template<typename Task>
			void push_front_no_sync(Task&& task)
			{
				this->_jobs.push_front(make_job(std::forward<Task>(task)));
			}

			/*
				Adds tasks to the thread pool without synchronization.
				Tasks must define operator() that can take in Args...
				or optionally parent_ref as a first argument and then Args...
			*/
			template<typename FirstTask,typename... Rest>
			void push_back_no_sync(FirstTask&& first,Rest&& ... rest)
			{
				push_back_no_sync(std::forward<FirstTask>(first));
				push_back_no_sync(std::forward<Rest>(rest)...);
			}

			/*
				Adds tasks to the front of the thread pool without synchronization.
				Tasks must define operator() that can take in Args...
				or optionally parent_ref as a first argument and then Args...
			*/
			template<typename FirstTask,typename... Rest>
			void push_front_no_sync(FirstTask&& first,Rest&& ... rest)
			{
				push_front_no_sync(std::forward<FirstTask>(first));
				push_front_no_sync(std::forward<Rest>(rest)...);
			}

			/*
				Adds task(s) to the thread pool with synchronization and wakes an appropriate number of threads.
				Tasks must define operator() that can take in Args...
				or optionally parent_ref as a first argument and then Args...
			*/
			template<typename... Tasks>
			void push_back(Tasks&& ... tasks)
			{
				{
					std::lock_guard<std::mutex> guard(this->_mtx);
					push_back_no_sync(std::forward<Tasks>(tasks)...);
				}
				this->notify_count(sizeof...(Tasks));
			}

			/*
				Adds task(s) to the front of the thread pool with synchronization and wakes an appropriate number of threads.
				Tasks must define operator() that can take in Args...
				or optionally parent_ref as a first argument and then Args...
			*/
			template<typename... Tasks>
			void push_front(Tasks&& ... tasks)
			{
				{
					std::lock_guard<std::mutex> guard(this->_mtx);
					push_front_no_sync(std::forward<Tasks>(tasks)...);
				}
				this->notify_count(sizeof...(Tasks));
			}

			/*
				Adds task(s) to the thread pool without synchronization reading	from the given iterators.
				Tasks must define operator() that can take in Args...
				or optionally parent_ref as a first argument and then Args...
				Returns the number of tasks added.
			*/
			template<typename Iter>
			size_t append_no_sync(Iter begin,Iter end)
			{
				using category=typename std::iterator_traits<Iter>::iterator_category;
				return append_no_sync(begin,end,category{});
			}

			/*
				Adds task(s) to the front of the thread pool without synchronization reading from the given iterators.
				Tasks must define operator() that can take in Args...
				or optionally parent_ref as a first argument and then Args...
				Returns the number of tasks added.
			*/
			template<typename Iter>
			size_t prepend_no_sync(Iter begin,Iter end)
			{
				using category=typename std::iterator_traits<Iter>::iterator_category;
				return prepend_no_sync(begin,end,category{});
			}

			/*
				Adds task(s) to the thread pool with synchronization and wakes an appropriate number of threads reading
				from the given iterators.
				Tasks must define operator() that can take in Args...
				or optionally parent_ref as a first argument and then Args...
				Returns the number of tasks added.
			*/
			template<typename Iter>
			size_t append(Iter begin,Iter end)
			{
				size_t count;
				{
					std::lock_guard<std::mutex> guard(this->_mtx);
					count=append_no_sync(begin,end);
				}
				this->notify_count(count);
				return count;
			}

			/*
				Adds task(s) to the front of the thread pool with synchronization and wakes an appropriate number of threads reading
				from the given iterators.
				Tasks must define operator() that can take in Args...
				or optionally parent_ref as a first argument and then Args...
				Returns the number of tasks added.
			*/
			template<typename Iter>
			size_t prepend(Iter begin,Iter end)
			{
				size_t count;
				{
					std::lock_guard<std::mutex> guard(this->_mtx);
					count=prepend_no_sync(begin,end);
				}
				this->notify_count(count);
				return count;
			}

			/*
				Clears the jobs handled by this threadpool without synchronization.
			*/
			void clear_no_sync() noexcept
			{
				this->_jobs.clear();
			}

			/*
				Clears the jobs handled by this threadpool.
			*/
			void clear()
			{
				std::lock_guard<std::mutex> guard(this->_mtx);
				this->clear_no_sync();
			}

			/*
				Changes the number of threads.
			*/
			void num_threads(size_t size)
			{
				assert(size!=0);
				if(size==num_threads())
				{
					return;
				}
				if(this->_running)
				{
					if(size>this->_workers.size())
					{
						make_worker_t maker{this};
						struct count_iter {
							using iterator_category=std::random_access_iterator_tag;
							using difference_type=std::ptrdiff_t;
							using value_type=size_t;
							using reference=value_type;
							using pointer=value_type const*;
							size_t _count;
							size_t operator*() const
							{
								return _count;
							}
							pointer operator->() const
							{
								return &_count;
							}
							std::ptrdiff_t operator-(count_iter o) const
							{
								return _count-o._count;
							}
							count_iter& operator++()
							{
								++_count;
								return *this;
							}
							count_iter operator--()
							{
								auto copy{*this};
								++_count;
								return *this;
							}
							bool operator==(count_iter o) const
							{
								return _count==o._count;
							}
							bool operator!=(count_iter o) const
							{
								return _count!=o._count;
							}
						};
						auto begin=thread_pool_detail::make_transform_iterator(count_iter{0},maker);
						decltype(begin) end{count_iter{this->_workers.size()}};
						_workers.insert(_workers.end(),begin,end);
					}
					else
					{
						this->terminate();
						this->_workers.resize(size);
						this->start();
					}
				}
				else
				{
					this->_workers.resize(size);
				}
			}

			/*
				The number of threads.
			*/
			_EXLIB_THREAD_POOL_NODISCARD size_t num_threads() const noexcept
			{
				return this->_workers.size();
			}

			/*
				The number of jobs left.
			*/
			_EXLIB_THREAD_POOL_NODISCARD size_t num_jobs() const
			{
				std::unique_lock<std::mutex> lock(this->_mtx);
				return num_jobs_no_sync();
			}

			/*
				The number of jobs left read unsynchronized from the job queue.
			*/
			_EXLIB_THREAD_POOL_NODISCARD size_t num_jobs_no_sync() const noexcept
			{
				return this->_jobs.size();
			}

			/*
				Whether job queue is empty.
			*/
			_EXLIB_THREAD_POOL_NODISCARD bool empty() const
			{
				return num_jobs()==0;
			}

			/*
				Whether job queue is empty read unsynchronized from the job queue.
			*/
			_EXLIB_THREAD_POOL_NODISCARD bool empty_no_sync() const noexcept
			{
				return num_jobs()==0;
			}

			/*
				Returns a future representing the result of running the given task asynchronously.
			*/
			template<typename Task>
			_EXLIB_THREAD_POOL_NODISCARD auto async(Task&& task) -> std::future<decltype(thread_pool_detail::fake_invoke<parent_ref,Args...>(std::forward<Task>(task)))>
			{
				thread_pool_detail::TaskDoer<Task,parent_ref,Args...> doer{std::forward<Task>(task)};
				auto future=doer.promise.get_future();
				push_back(std::move(doer));
				return future;
			}

			/*
				Returns a future representing the result of running the given task asynchronously.
			*/
			template<typename Task,typename... Extra>
			_EXLIB_THREAD_POOL_NODISCARD auto async(Task&& task,Extra...) -> std::future<decltype(thread_pool_detail::fake_invoke<Args...>(std::forward<Task>(task)))>
			{
				static_assert(sizeof...(Extra)==0,"These arguments are only for SFINAE. Use lambda capture/bind if you want to arguments passed to a functor.");
				thread_pool_detail::TaskDoer<Task,Args...> doer{std::forward<Task>(task)};
				auto future=doer.promise.get_future();
				push_back(std::move(doer));
				return future;
			}

		private:

			void join_base()
			{
				wait();
				this->_active=false;
				this->_running=false;
				this->_signal_start.notify_all();
			}

			friend struct make_worker_t;
			struct make_worker_t {
				thread_pool_a* parent;
				thread_pool_detail::joining_thread operator()(size_t const&) const
				{
					return thread_pool_detail::joining_thread(&thread_pool_a::task_loop,parent);
				}
			};

			friend struct make_job_functor_t;
			template<typename Iter>
			size_t append_no_sync(Iter begin,Iter end,std::random_access_iterator_tag)
			{
				auto gbegin=thread_pool_detail::make_transform_iterator(begin,make_job_functor_t{});
				auto gend=thread_pool_detail::make_transform_iterator(end,make_job_functor_t{});
				this->_jobs.insert(this->_jobs.end(),gbegin,gend);
				return end-begin;
			}
			template<typename Iter>
			size_t append_no_sync(Iter begin,Iter end,std::input_iterator_tag)
			{
				size_t count=0;
				using input_type=typename std::iterator_traits<Iter>::reference;
				std::for_each(begin,end,[this,&count](input_type task)
					{
						this->push_back_no_sync(std::forward<input_type>(task));
						++count;
					});
				return count;
			}
			template<typename Iter>
			size_t prepend_no_sync(Iter begin,Iter end,std::random_access_iterator_tag)
			{
				auto gbegin=thread_pool_detail::make_transform_iterator(begin,make_job_functor_t{});
				auto gend=thread_pool_detail::make_transform_iterator(end,make_job_functor_t{});
				this->_jobs.insert(_jobs.begin(),gbegin,gend);
				return end-begin;
			}
			template<typename Iter>
			size_t prepend_no_sync(Iter begin,Iter end,std::input_iterator_tag)
			{
				size_t count=0;
				using input_type=typename std::iterator_traits<Iter>::reference;
				std::for_each(begin,end,[this,&count](input_type task)
					{
						this->push_front_no_sync(std::forward<input_type>(task));
						++count;
					});
				return count;
			}
			bool idle() const noexcept
			{
				return !this->_active||(this->_jobs.empty()&&this->_active_thread_count==0);
			}
			void create_threads()
			{
				assert(num_threads()!=0);
				this->_active_thread_count=0;
				for(auto& worker:this->_workers)
				{
					worker=thread_pool_detail::joining_thread(&thread_pool_a::task_loop,this);
				}
			}
			using TaskInput=std::tuple<thread_pool_detail::wrap_reference_t<Args>...>;
			struct job {
				virtual void operator()(parent_ref,TaskInput const& input) noexcept=0;
				virtual ~job()=default;
			};
			template<typename BaseFunc>
			struct job_impl:job {
				BaseFunc task;
				template<typename F>
				job_impl(F&& f):task(std::forward<F>(f))
				{}
				void operator()(parent_ref,TaskInput const& input) noexcept override
				{
					thread_pool_detail::apply(task,input);
				}
			};
			template<typename BaseFunc>
			struct job_impl_accept_parent:job {
				BaseFunc task;
				template<typename F>
				job_impl_accept_parent(F&& f):task(std::forward<F>(f))
				{}
				void operator()(parent_ref tp,TaskInput const& input) noexcept override
				{
					thread_pool_detail::apply_fa(task,tp,input);
				}
			};

			struct make_job_functor_t {
				template<typename Task>
				std::unique_ptr<job> operator()(Task&& task) const
				{
					return thread_pool_a::make_job(std::forward<Task>(task));
				}
			};

			//overload to try to fit to pass arguments without parent
			template<typename Task,typename... Extra>
			static std::unique_ptr<job> make_job(Task&& the_task,Extra...)
			{
				return make_job2(std::forward<Task>(the_task));
			}

			//overload to try to fit to pass arguments with parent
			template<typename Task>
			static auto make_job(Task&& the_task) -> decltype(thread_pool_detail::fake_invoke<parent_ref,Args...>(std::forward<Task>(the_task)),std::unique_ptr<job>())
			{
				static_assert(noexcept(thread_pool_detail::fake_invoke<parent_ref,Args...>(std::forward<Task>(the_task))),"Tasks cannot throw (thread_pool_a has no exception handling mechanism); use async/promise if you need errors.");
				return std::unique_ptr<job>(new job_impl_accept_parent<thread_pool_detail::remove_cvref_t<Task>>(std::forward<Task>(the_task)));
			}

			template<typename Task,typename... Extra>
			static std::unique_ptr<job> make_job2(Task&& the_task,Extra...)
			{
				static_assert(!std::is_same<Task,Task>::value,"Task fails to accepts proper arguments; must accept (parent_ref, Args...), or (Args...)");
			}

			template<typename Task>
			static auto make_job2(Task&& the_task) -> decltype(thread_pool_detail::fake_invoke<Args...>(std::forward<Task>(the_task)),std::unique_ptr<job>())
			{
				static_assert(noexcept(thread_pool_detail::fake_invoke<Args...>(std::forward<Task>(the_task))),"Tasks cannot throw (thread_pool_a has no exception handling mechanism); use async/promise if you need errors.");
				return std::unique_ptr<job>(new job_impl<thread_pool_detail::remove_cvref_t<Task>>(std::forward<Task>(the_task)));
			}

			void task_loop() noexcept
			{
				std::unique_ptr<job> task;
				while(true)
				{
					{
						if(!this->_running)
						{
							return; //don't bother locking if not running
						}
						std::unique_lock<std::mutex> lock(this->_mtx);
						while(true)
						{
							if(!this->_running)
							{
								return;
							}
							if(this->_active&&!this->_jobs.empty())
							{
								task=std::move(this->_jobs.front());
								this->_jobs.pop_front();
								++this->_active_thread_count;
								break;
							}
							this->_signal_start.wait(lock);
						}
					}
					(*task)(parent_ref{*this},this->_input);
					auto const active=--this->_active_thread_count;
					if(active==0)
					{
						this->_jobs_done.notify_all();
					}
				}
			}
			std::deque<std::unique_ptr<job>> _jobs;
			TaskInput _input;
		};
	}

	using thread_pool_impl::thread_pool_a;
	using thread_pool=thread_pool_a<>;
}
#endif