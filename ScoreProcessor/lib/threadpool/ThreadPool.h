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
		struct no_references;

		template<>
		struct no_references<> {
			constexpr static bool const value=true;
		};

		template<typename T>
		struct no_references<T> {
			constexpr static bool const value=!std::is_reference<T>::value;
		};

		template<typename First,typename... Args>
		struct no_references<First,Args...> {
			constexpr static bool const value=no_references<First>::value&&no_references<Args...>::value;
		};
	}
	/*
	Overload void execute(...) to use this as a task in ThreadPool
	*/
	template<typename... Args>
	class ThreadTaskA {
	protected:
		ThreadTaskA()=default;
	public:
		static_assert(no_references<Args...>::value,"References not allowed as execute arguments");
		virtual ~ThreadTaskA()=default;
		virtual void execute(Args...)=0;
	};

	using ThreadTask=ThreadTaskA<>;
	/*
	Creates a ThreadTask that calls whatever object of type Task.
	*/
	template<typename Task>
	class AutoTask:public ThreadTaskA<> {
		Task task;
	public:
		AutoTask(Task task):task(task)
		{}
		void execute() override
		{
			task();
		}
	};

	template<typename... Args>
	class ThreadPoolA {
	private:
		typedef ThreadTaskA<Args...> Task;
		std::vector<std::thread> workers;
		std::queue<std::unique_ptr<Task>> tasks;
		std::mutex locker;
		std::atomic<bool> running;
		inline void task_loop(Args... args)
		{
			while(running)
			{
				std::unique_ptr<Task> task;
				bool has_task;
				{
					std::lock_guard<std::mutex> guard(locker);
					if(has_task=!tasks.empty())
					{
						task=std::move(tasks.front());
						tasks.pop();
					}
				}
				if(has_task)
				{
					task->execute(args...);
				}
				else
				{
					running=false;
				}
			}
		}
	public:
		static_assert(no_references<Args...>::value,"References not allowed as start arguments");
		ThreadPoolA(ThreadPoolA const&)=delete;
		ThreadPoolA(ThreadPoolA&&)=delete;

		/*
		Creates a thread pool with a certain number of threads
		*/
		inline explicit ThreadPoolA(size_t num_threads):workers(num_threads)
		{}
		/*
		Creates a thread pool with number of threads equal to the hardware concurrency
		*/
		inline ThreadPoolA():ThreadPoolA(std::thread::hardware_concurrency())
		{}

		/*
		Adds a task of type Task constructed with args unsynchronized with running threads
		*/
		template<typename ConsTask,typename... ConsArgs>
		void add_task(ConsArgs&&... args)
		{
			tasks.push(std::make_unique<ConsTask>(std::forward<ConsArgs>(args)...));
		}

		/*
		Adds a task of type Task constructed with args synchronized with running threads
		*/
		template<typename ConsTask,typename... ConsArgs>
		void add_task_sync(ConsArgs&&... args)
		{
			std::lock_guard<std::mutex> guard(locker);
			add_task<ConsTask>(std::forward<ConsArgs>(args)...);
		}

		/*
		Whether the thread pool is running
		*/
		bool is_running() const
		{
			return running;
		}
		/*
		Starts all the threads
		Calling start on a pool that has not been stopped will result in undefined behavior
		*/
		void start(Args... args)
		{
			running=true;
			for(size_t i=0;i<workers.size();++i)
			{
				workers[i]=std::thread(&ThreadPoolA::task_loop,this,args...);
			}
		}

		/*
		Waits for all tasks to be finished and then stops the thread pool
		*/
		inline void wait()
		{
			for(size_t i=0;i<workers.size();++i)
			{
				if(workers[i].joinable())
					workers[i].join();
			}
		}

		/*
			Makes threads no longer attempt to find new tasks; does not wait for them to stop.
		*/
		inline void deactivate()
		{
			running=false;
		}

		/*
		Stops as soon as all threads are done with their current tasks
		*/
		inline void stop()
		{
			deactivate();
			wait();
		}



		inline void join()
		{
			wait();
		}

		/*
		Destroys the thread pool after waiting for its threads
		*/
		inline ~ThreadPoolA()
		{
			wait();
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
