#ifndef	TASK_H
#define TASK_H
#include "pub.h"
#include <thread>
#include <queue>
#include <thread>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>

class Task {
public:
    static Task& GetInstance() {
        static Task instance;
        return instance;
    }
    inline uint8_t WokerCnt() {
        return w.size();
    }

    template<typename F>
    void Add(F&& f) {
        t.emplace([f = std::forward<F>(f)] { f(); });
        t_cnt++;
    }

    void Excute();
    void AsynExcute();
    bool Free();
    /*template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) {
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            [func = std::forward<F>(f), ...args = std::forward<Args>(args)] {
                return func(std::forward<Args>(args)...);
            }
        );

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            tasks.emplace([task] { (*task)(); });
        }
    }*/
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
private:
    Task();
    ~Task();

    std::vector<std::thread> w;
    std::queue<std::function<void()>> t;

    std::mutex m;
    std::condition_variable c;
    std::atomic<bool> stop;
    std::atomic<uint8_t> t_cnt;

    size_t cores;
};
#endif