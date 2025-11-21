#include "task.h"
using namespace std;

Task::Task() : stop(false) {
    cores = thread::hardware_concurrency() - 1;
    if (cores <= 0) {
        cores = 2; // may be need send a warnning in here
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
    unique_lock<mutex> lock(m);
    stop = true;
    c.notify_all();
    for (thread& worker : w) {
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

void Task::Parallel2D(uint16_t sizex, uint16_t sizey, uint16_t blockSize,
    function<void(uint16_t x, uint16_t y)> work)
{
#ifdef DEBUG
    auto wCnt = 1;
#else
    auto wCnt = w.size();
#endif
    uint32_t totalBlocks = (sizex + blockSize - 1) / blockSize;
    atomic<uint32_t> nextBlock{ 0 };

    for (uint32_t i = 0; i < wCnt; i++) {
        Add([&]() {
            while (true) {
                uint32_t blockIdx = nextBlock.fetch_add(1);
                if (blockIdx >= totalBlocks) {
                    break;
                }

                uint32_t yStart = blockIdx * blockSize;
                uint32_t yEnd = min(yStart + blockSize, (uint32_t)sizey);
                for (uint16_t y = yStart; y < yEnd; y++) {
                    for (uint16_t x = 0; x < sizex; x++) {
#ifdef DEBUG
                        if (x >= 300 && x <= 500 && y >= 300 && y <= 500)
#endif // DEBUG
                            work(x, y);
                    }
                }
            }
        });
    }
    Excute();
}