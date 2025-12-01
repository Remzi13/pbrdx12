#pragma once

#include <vector>
#include <functional>
#include <tuple>  
#include <utility>
#include <deque>
#include <mutex>
#include <thread>


class TaskManager
{
public:

    TaskManager(int numThreads, int maxTasks);

    template <typename Callable, typename... Args>
    bool add(Callable&& func, Args&&... args) 
    {
        {
            std::lock_guard<std::mutex> lock(tasksMutex_);
            if (!isRunning_ || tasks_.size() >= maxTasks_)
            {
                return false;
            }

            tasks_.emplace_back(
                [func = std::forward<Callable>(func), ...args = std::forward<Args>(args)]() mutable {
                    func(args...);
                }
            );
        } 

        condition_.notify_one();
        return true;
    }
        
    void stop();

private:
    template <typename Callable, typename... Args>
    class Task {
    private:
        Callable func_;
        using DecayedArgs = std::tuple<std::decay_t<Args>...>;
        DecayedArgs args_tuple_;

    public:
        Task(Callable&& func, Args&&... args)
            : func_(std::forward<Callable>(func)),
            args_tuple_(std::make_tuple(std::forward<Args>(args)...))
        {

        }

        void operator()() {

            std::apply(func_, args_tuple_);
        }
    };

    std::mutex tasksMutex_;
    std::deque<std::function<void()>> tasks_;

    std::vector<std::thread> threads_;
    std::condition_variable condition_;
    bool isRunning_;
    int maxTasks_;

};
