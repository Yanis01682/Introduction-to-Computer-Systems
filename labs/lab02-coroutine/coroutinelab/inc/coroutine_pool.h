#pragma once
#include "context.h"
#include <memory>
#include <thread>
#include <vector>

struct coroutine_pool;
extern coroutine_pool *g_pool;

/**
 * @brief 协程池
 * 保存所有需要同步执行的协程函数。并可以进行并行/串行执行。
 */
struct coroutine_pool {
  std::vector<basic_context *> coroutines;
  int context_id;

  // whether run in threads or coroutines
  bool is_parallel;

  ~coroutine_pool() {
    for (auto context : coroutines) {
      delete context;
    }
  }

  // add coroutine to pool
  template <typename F, typename... Args>
  void new_coroutine(F f, Args... args) {
    coroutines.push_back(new coroutine_context(f, args...));
  }

  /**
   * @brief 以并行多线程的方式执行所有协程函数
   */
  void parallel_execute_all() {
    g_pool = this;
    is_parallel = true;
    std::vector<std::thread> threads;
    for (auto p : coroutines) {
      threads.emplace_back([p]() { p->run(); });
    }

    for (auto &thread : threads) {
      thread.join();
    }
  }

  /**
   * @brief 以协程执行的方式串行并同时执行所有协程函数
   * TODO: Task 1, Task 2
   * 在 Task 1 中，我们不需要考虑协程的 ready
   * 属性，即可以采用轮询的方式挑选一个未完成执行的协程函数进行继续执行的操作。
   * 在 Task 2 中，我们需要考虑 sleep 带来的 ready
   * 属性，需要对协程函数进行过滤，选择 ready 的协程函数进行执行。
   *
   * 当所有协程函数都执行完毕后，退出该函数。
   */
  void serial_execute_all() {
    is_parallel = false;
    g_pool = this;

    // 循环，直到所有协程都执行完毕
    while (true) {
        bool all_finished = true; //假设大家都跑完了

        // 遍历每一个协程
        for (int i = 0; i < coroutines.size(); i++) {
            auto context = coroutines[i];

            //如果发现有协程还没跑完
            if (!context->finished) {
                all_finished = false; // 那就说明还得继续循环

                // Task 1 逻辑：只要没结束，就认为是 Ready 的，直接调度
                // if (context->ready) {
                //     context_id = i;    // 记录当前是谁在跑 (给 yield 用)
                //     context->resume(); // 切进去
                // }
                // Task 2 新增逻辑: 检查睡眠是否结束
                if (!context->ready) {
                    // 调用刚才在 sleep 里写的 lambda 函数
                    if (context->ready_func && context->ready_func()) {
                        context->ready = true; // 醒了！标记为 Ready
                    }
                }
                if (context->ready) {
                    context_id = i;
                    context->resume();
                }
            }
        }

        // 如果遍历一圈发现所有人都 finished 了，那就收工
        if (all_finished) break;
    }

    // 清理内存
    for (auto context : coroutines) {
      delete context;
    }
    coroutines.clear();
  }
};
