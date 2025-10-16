#pragma once
#include "pch.h"
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
    void Task::Parallel2D(uint16_t x, uint16_t y, uint16_t blockSize,
        std::function<void(uint16_t x, uint16_t y)> work);
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