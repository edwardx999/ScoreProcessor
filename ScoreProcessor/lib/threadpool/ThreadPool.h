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
#ifdef THREADPOOL_EXPORTS
#define THREADPOOL_API
#else
#define THREADPOOL_API
#endif
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
	private:
	protected:
		ThreadTask()=default;
	public:
		~ThreadTask()=default;
		virtual void execute()=0;
	};

	class ThreadPool {
		friend class std::thread;
	private:
		std::vector<std::thread> workers;
		std::queue<std::unique_ptr<ThreadTask>> tasks;
		std::mutex locker;
		std::atomic<bool> running;
		THREADPOOL_API void task_loop();
	public:
		/*
		Creates a thread pool with a certain number of threads
		*/
		THREADPOOL_API explicit ThreadPool(size_t num_threads);
		/*
		Creates a thread pool with number of threads equal to the hardware concurrency
		*/
		THREADPOOL_API ThreadPool();
		/*
		Destroys the thread pool after stopping its threads
		*/
		THREADPOOL_API ~ThreadPool();
		/*
		Adds a task of type Task constructed with args unsynchronized with running threads
		*/
		template<typename Task,typename... Args>
		void add_task(Args&&...);
		/*
		Adds a task of type Task constructed with args synchronized with running threads
		*/
		template<typename Task,typename... Args>
		void add_task_sync(Args&&...);
		/*
		Whether the thread pool is running
		*/
		bool is_running() const;
		/*
		Starts all the threads
		Calling start on a pool that has not been stopped will result in undefined behavior
		*/
		THREADPOOL_API void start();
		/*
		Stops as soon as all threads are done with their current tasks
		Calling stop on a pool that is not started will result in undefined behavior
		*/
		THREADPOOL_API void stop();
		/*
		Waits for all tasks to be finished and then stops the thread pool
		Calling wait on a pool that is not started will result in undefined behavior
		*/
		THREADPOOL_API void wait();
	};

	template<typename Task,typename... Args>
	void ThreadPool::add_task(Args&&... arguments)
	{
		tasks.push(std::make_unique<Task>(std::forward<Args>(arguments)...));
	}
	template<typename Task,typename... Args>
	void ThreadPool::add_task_sync(Args&&... arguments)
	{
		std::lock_guard<std::mutex> guard(locker);
		tasks.push(std::make_unique<Task>(std::forward<Args>(arguments)...));
	}
	inline bool ThreadPool::is_running() const
	{
		return running;
	}
}
#endif // !THREAD_POOL_H
