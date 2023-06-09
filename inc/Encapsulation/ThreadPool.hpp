/*
** EPITECH PROJECT, 2021
** plazza [WSL: Ubuntu]
** File description:
** ThreadPool
*/

#pragma once

#include <atomic>
#include "Thread.hpp"
#include <condition_variable>
#include <functional>
#include <future>
#include <queue>
#include <stdexcept>
#include <vector>
#include <thread>


class ThreadPool {
    public:
        enum Status {
                DEDE,
                STARTED,
                RUNING
        };

    private:
        using Task = std::function<void()>;
        using Tstatus = std::unordered_map<std::thread::id, Status>;
        // 线程池 - thread pool
        std::vector<std::thread> _threads;
        // 任务队列 - task queue
        std::queue<Task> _tasks;
        // 同步 - Sync
        // 条件阻塞 - conditional block
        std::condition_variable cv_task;
        // 是否关闭提交 - Whether to close submission or not
        std::atomic<bool> _stoped;
        //空闲线程数量 - Number of idle threads
        std::atomic<uint16_t> _idlThrNum;
        Tstatus threadsStatusMap;
        void workingTask();

    protected:
        std::mutex _m_lock;
        Status get_status(uint16_t id){ return threadsStatusMap.at(_threads[id].get_id()); }

    public:
        ThreadPool(uint16_t numThreads);

        ~ThreadPool();

        void start();

        // 提交一个任务 - Submit a task
        // 调用.get()获取返回值会等待任务执行完,获取返回值
        // call .get() to get the return value will wait for the task to finish executing, get the return value
        // 有两种方法可以实现调用类成员 - There are two ways to call a class member,
        // 一种是使用 - one way  bind： .commit(std::bind(&Dog::sayHello, &dog));
        // 一种是用 - othre way mem_fn： .commit(std::mem_fn(&Dog::sayHello), &dog)
        template<class F, class... Args>
        auto commit(F &&f, Args &&...args) {
            if (_stoped.load())// stop == true ??
                throw std::runtime_error("commit on ThreadPool is stopped.");

            using RetType = decltype(f(args...));// typename std::result_of<F(Args...)>::type, 函数 f 的返回值类型
            auto task = std::make_shared<std::packaged_task<RetType()>>(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...));// wtf !
            std::future<RetType> future = task->get_future();
            {                                             // 添加任务到队列
                std::scoped_lock lock{_m_lock};//对当前块的语句加锁  lock_guard 是 mutex 的 stack 封装类，构造的时候 lock()，析构的时候 unlock()
                _tasks.emplace(
                        [task]() {// push(Task{...})
                            (*task)();
                        });
            }
            cv_task.notify_one();// 唤醒一个线程执行
            return future;
        }

        Status operator [](uint16_t id){ return get_status(id); }

        //空闲线程数量 - retrun Number of idle threads
        uint16_t idlCount() { return _idlThrNum; }
};
