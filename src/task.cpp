#include "task.h"
using namespace std;

Task::Task() : stop(false) {
    size_t cores = std::thread::hardware_concurrency() - 1;
    if (cores <= 0) {
        cores = 2;
    }
    w.reserve(cores);
    for (size_t i = 0; i < cores; ++i) {
        w.emplace_back([this] {
            while (true) {
                unique_lock<mutex> lock(m);
                c.wait(lock, [this] {
                    return stop || !t.empty();
                });
                if (stop && t.empty()) {
                    return;
                }
                auto task = move(t.front());
                t.pop();
                lock.unlock();
                task();
                t_cnt--;
            }
        });
    }
}

Task::~Task() {
    std::unique_lock<std::mutex> lock(m);
    stop = true;
    c.notify_all();
    for (std::thread& worker : w) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void Task::Excute()
{
    c.notify_all();
    while (t_cnt > 0);
}

void Task::AsynExcute()
{
    c.notify_all();
}

bool Task::Free()
{
    return t_cnt <= 0;
}
