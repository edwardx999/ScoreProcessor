/*
Copyright(C) 2017 Edward Xie

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <mutex>
#include <queue>
#include <memory>
#include <atomic>
#include <utility>
#include <type_traits>
namespace exlib {

	namespace {
		template<typename... Args>
		using no_references=std::conjunction<std::negation<std::is_reference<Args>>...>;
	}
	/*
	Overload void execute(...) to use this as a task in ThreadPool
	*/
	template<typename... Args>
	class ThreadTaskA {
	protected:
		ThreadTaskA()=default;
	public:
		//static_assert(no_references<Args...>::value,"References not allowed as execute arguments");
		virtual ~ThreadTaskA()=default;
		virtual void execute(Args...)=0;
	};

	using ThreadTask=ThreadTaskA<>;

	/*
		Creates a ThreadTask that calls whatever object of type Task.
	*/
	template<typename Task,typename... Args>
	class AutoTaskA:public ThreadTaskA<Args...> {
		Task task;
	public:
		AutoTaskA(Task task):task(task)
		{}
		void execute(Args... args) override
		{
			task(args...);
		}
	};

	template<typename Task>
	using AutoTask=AutoTaskA<Task>;

	namespace detail {
		struct tracked_thread {
			std::thread thread;
			std::atomic<bool> working;
		};
	}

	struct delay_activation_t {};

	template<typename... Args>
	class ThreadPoolA {
	public:
		typedef ThreadTaskA<Args...> Task;
	private:
		std::vector<detail::tracked_thread> _workers;
		std::queue<std::unique_ptr<Task>> _tasks;
		std::mutex _locker;
		std::atomic<bool> _running;
		std::tuple<Args...> _args;
		inline void task_loop(size_t id)
		{
			while(_running)
			{
				while(_workers[id].working)
				{
					std::unique_ptr<Task> task;
					{
						std::lock_guard<std::mutex> guard(_locker);
						if(!_tasks.empty())
						{
							task=std::move(_tasks.front());
							_tasks.pop();
						}
					}
					if(task)
					{
						std::apply([&](auto&... args)
						{
							task->execute(args...);
						},_args);
					}
					else
					{
						_workers[id].working=false;
					}
				}
			}
		}
	public:
		//static_assert(no_references<Args...>::value,"References not allowed as start arguments");
		ThreadPoolA(ThreadPoolA const&)=delete;
		ThreadPoolA(ThreadPoolA&&)=delete;

		/*
			Threads start running but they do not look for tasks.
		*/
		void activate()
		{
			_running=true;
			for(size_t i=0;i<_workers.size();++i)
			{
				_workers[i].working=false;
				_workers[i].thread=std::thread(&ThreadPoolA::task_loop,this,i);
			}
		}

		/*
			Creates a thread pool with a certain number of threads and initialize the arguments sent.
		*/
		template<typename... T>
		inline explicit ThreadPoolA(size_t num_threads,T&&... args):_workers(num_threads),_args(std::forward<T>(args)...)
		{
			activate();
		}

		/*
			Creates a thread pool with a certain number of threads and initializes the arguments sent.
			Threads are not started.
		*/
		template<typename... T>
		inline explicit ThreadPoolA(size_t num_threads,delay_activation_t,T&&... args):_workers(num_threads),_args(std::forward<T>(args)...)
		{}

		/*
			Creates a thread pool with number of threads equal to the hardware concurrency
		*/
		inline ThreadPoolA():ThreadPoolA([]()->size_t
		{
			auto const nt=std::thread::hardware_concurrency();
			if(nt)
			{
				return nt;
			}
			return 1;
		}())
		{}

		/*
			Adds a task of type Task constructed with args unsynchronized with running threads
		*/
		template<typename ConsTask,typename... ConsArgs>
		void add_task(ConsArgs&&... args)
		{
			_tasks.push(std::make_unique<ConsTask>(std::forward<ConsArgs>(args)...));
		}

	private:
		template<typename T>
		struct is_thread_task:public std::is_convertible<T*,ThreadTaskA<Args...>*> {};

		template<typename T>
		struct is_thread_task<T const&>:public is_thread_task<T> {};

		template<typename T>
		struct is_thread_task<T&>:public is_thread_task<T> {};

		template<typename T>
		struct is_thread_task<T&&>:public is_thread_task<T> {};
	public:

		/*
			Overload for lambdas, std::functions, function pointers...,
			anything with operator(Args...) defined and which is not a ThreadTaskA<Args...>
		*/
		template<typename Function>
		auto add_task(Function func) -> decltype(func(std::declval<Args>()...),std::enable_if<!is_thread_task<Function>::value>::type())
		{
			_tasks.push(std::make_unique<AutoTaskA<decltype(func),Args...>>(func));
		}

		/*
			Overload for copying or moving existing ThreadTasks
		*/
		template<typename ATask>
		auto add_task(ATask&& task) -> decltype(std::enable_if<is_thread_task<ATask>::value>::type())
		{
			_tasks.push(std::make_unique<std::remove_reference<ATask>::type>(std::forward<ATask>(task)));
		}

		/*
			Adds a task of type Task constructed with args synchronized with running threads
		*/
		template<typename ConsTask,typename... ConsArgs>
		void add_task_sync(ConsArgs&&... args)
		{
			std::lock_guard<std::mutex> guard(_locker);
			add_task<ConsTask>(std::forward<ConsArgs>(args)...);
		}

		/*
		Overload for copying or moving existing ThreadTasks
		*/
		template<typename ATask>
		void add_task_sync(ATask&& task)
		{
			std::lock_guard<std::mutex> guard(_locker);
			add_task(std::forward<ATask>(task));
		}

	private:
		template<typename First>
		void add_tasks_base(First&& first)
		{
			add_task(std::forward<First>(first));
		}

		template<typename First,typename... Rest>
		void add_tasks_base(First&& f,Rest&&... rest)
		{
			add_task(std::forward<First>(f));
			add_tasks_base(std::forward<Rest>(rest)...);
		}

	public:
		template<typename... Tasks>
		void add_tasks_sync(Tasks&&... tasks)
		{
			static_assert(sizeof...(tasks)>0,"Arguments needed");
			std::lock_guard<std::mutex> guard(_locker);
			add_tasks_base(std::forward<Tasks>(tasks)...);
		}
		/*
			Whether the thread pool is running
		*/
		bool is_running() const
		{
			return _running;
		}

		template<typename... T>
		void set_args(T&&... args)
		{
			_args=decltype(_args)(std::forward<T>(args)...);
		}

		/*
			Makes threads start looking for tasks.
		*/
		void start()
		{
			for(auto& w:_workers)
			{
				w.working=true;
			}
		}

		/*
			Causes threads to no longer look for jobs
			wait cannot be safely called after invoking this; join can
		*/
		void give_up()
		{
			for(auto& w:_workers)
			{
				w.working=false;
			}
		}

		/*
			Starts all the threads
			Calling start on a pool that has not been stopped will result in undefined behavior
		*/
		template<typename... T>
		void start(T&&... args)
		{
			set_args(std::forward<T>(args)...);
			start();
		}

		/*
			Waits for all tasks to be finished. Threads keep running, but you can add tasks without synchronization.
	   */
		inline void wait()
		{
			while(true)
			{
				for(auto& w:_workers)
				{
					if(w.working)
					{
						goto cont;
					}
				}
				break;
			cont:;
			}
		}

		/*
			Waits for all tasks to be finished. Threads stop.
		*/
		void join()
		{
			_running=false;
			for(auto& w:_workers)
			{
				if(w.thread.joinable())
				{
					w.thread.join();
				}
			}
		}

		/*
		Destroys the thread pool after waiting for its threads
		*/
		inline ~ThreadPoolA()
		{
			join();
		}

		size_t thread_count() const
		{
			return _workers.size();
		}
	};

	using ThreadPool=ThreadPoolA<>;

	template<typename Output>
	class Logger {
		std::mutex locker;
		Output* output;
	public:
		Logger(Output& out):output(&out)
		{}
		/*
		Logs to the output with a mutex lock. Do not call if your thread is holding onto the lock from get_lock().
		*/
		template<typename T,typename... U>
		void log(T const& arg,U const&... args);

		/*
		Logs to the output without a mutex lock.
		*/
		template<typename T>
		void log_unsafe(T const& in);

		template<typename T,typename... U>
		void log_unsafe(T const& in,U const&... args);

		/*
		Returns a lock on this logger. Can use log_unsafe() if holding onto the lock.
		*/
		std::unique_lock<std::mutex> get_lock();
	};

	template<typename Output>
	std::unique_lock<std::mutex> Logger<Output>::get_lock()
	{
		return std::unique_lock<std::mutex>(locker);
	}

	template<typename Output>
	template<typename T,typename... U>
	void Logger<Output>::log(T const& arg0,U const&... args)
	{
		std::lock_guard<std::mutex> guard(locker);
		log_unsafe(arg0,args...);
	}

	template<typename Output>
	template<typename T>
	void Logger<Output>::log_unsafe(T const& in)
	{
		(*output)<<in;
	}

	template<typename Output>
	template<typename T,typename... U>
	void Logger<Output>::log_unsafe(T const& in,U const&... args)
	{
		log_unsafe(in);
		log_unsafe(args...);
	}

	template<>
	template<typename T>
	void Logger<std::string>::log_unsafe(T const& in)
	{
		(*output)+=in;
	}

	template<>
	template<typename T>
	void Logger<std::wstring>::log_unsafe(T const& in)
	{
		(*output)+=in;
	}

	typedef Logger<std::ofstream> FileLogger;
	typedef Logger<std::ostream> OstreamLogger;
	typedef Logger<std::wostream> WOstreamLogger;
	typedef Logger<std::string> StringLogger;
	typedef Logger<std::wstring> WStringLogger;
}
#endif // !THREAD_POOL_H
