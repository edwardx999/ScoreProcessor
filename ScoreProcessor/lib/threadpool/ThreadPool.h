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
namespace exlib {
	/*
	Overload void execute() to use this as a task in ThreadPool
	*/
	class ThreadTask {
	protected:
		ThreadTask()=default;
	public:
		virtual ~ThreadTask()=default;
		virtual void execute()=0;
	};

	class ThreadPool {
	private:
		std::vector<std::thread> workers;
		std::queue<std::unique_ptr<ThreadTask>> tasks;
		std::mutex locker;
		std::atomic<bool> running;
		inline void task_loop()
		{
			while(running)
			{
				std::unique_ptr<ThreadTask> task;
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
					task->execute();
				}
				else
				{
					running=false;
				}
			}
		}
	public:
		ThreadPool(ThreadPool const&)=delete;
		ThreadPool(ThreadPool&&)=delete;

		/*
		Creates a thread pool with a certain number of threads
		*/
		inline explicit ThreadPool(size_t num_threads):workers(num_threads)
		{}
		/*
		Creates a thread pool with number of threads equal to the hardware concurrency
		*/
		inline ThreadPool():ThreadPool(std::thread::hardware_concurrency())
		{}

		/*
		Adds a task of type Task constructed with args unsynchronized with running threads
		*/
		template<typename Task,typename... Args>
		void add_task(Args&&... args)
		{
			tasks.push(std::make_unique<Task>(std::forward<Args>(args)...));
		}
		/*
		Adds a task of type Task constructed with args synchronized with running threads
		*/
		template<typename Task,typename... Args>
		void add_task_sync(Args&&... args)
		{
			std::lock_guard<std::mutex> guard(locker);
			tasks.push(std::make_unique<Task>(std::forward<Args>(args)...));
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
		void start()
		{
			running=true;
			for(size_t i=0;i<workers.size();++i)
			{
				workers[i]=std::thread(&ThreadPool::task_loop,this);
			}
		}

		/*
		Waits for all tasks to be finished and then stops the thread pool
		Calling wait on a pool that is not started will result in undefined behavior
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
		Stops as soon as all threads are done with their current tasks
		Calling stop on a pool that is not started will result in undefined behavior
		*/
		inline void stop()
		{
			running=false;
			wait();
		}

		inline void join()
		{
			wait();
		}

		/*
		Destroys the thread pool after waiting for its threads
		*/
		inline ~ThreadPool()
		{
			wait();
		}
	};

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
