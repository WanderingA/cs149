#include <condition_variable>
#include <mutex>
#include <thread>

#include <stdio.h>

/*
 Wrapper class around an integer counter and a mutex.
 */
class Counter {
    public:
        int counter_;
        std::mutex* mutex_;
        Counter() {
            counter_ = 0;
            mutex_ = new std::mutex();
        }
        ~Counter() {
            delete mutex_;
        }
};

void increment_counter_fn(Counter* counter) {
    for (int i = 0; i < 10000; i++) {
        // Call lock() method to acquire lock.
        counter->mutex_->lock();
        // Since multiple threads are trying to perform an increment, the
        // increment needs to be protected by a mutex.
        counter->counter_++;
        // Call unlock() method to release lock.
        counter->mutex_->unlock();
    }
}

/*
 * Threads increment a shared counter in a tight for loop 10,000 times.
 */
void mutex_example() {
    int num_threads = 8;

    printf("==============================================================\n");
    printf("Starting %d threads to increment counter...\n", num_threads);
    std::thread* threads = new std::thread[num_threads];
    Counter* counter = new Counter();
    // `num_threads` threads will call `increment_counter_fn`, trying to
    // increment `counter`.
    for (int i = 0; i < num_threads; i++) {
        threads[i] = std::thread(increment_counter_fn, counter);
    }
    // Wait for spawned threads to complete.
    for (int i = 0; i < num_threads; i++) {
        threads[i].join();
    }
    // Verify that final counter value is (10000 * `num_threads`).
    printf("Final counter value: %d...\n", counter->counter_);
    printf("==============================================================\n");

    delete counter;
    delete[] threads;
}

/*
 * 管理线程状态
 */
class ThreadState {
    public:
        std::condition_variable* condition_variable_;
        std::mutex* mutex_;
        int counter_;              // 事件个数
        int num_waiting_threads_;  // 当前等待的线程数
        ThreadState(int num_waiting_threads) {
            condition_variable_ = new std::condition_variable();
            mutex_ = new std::mutex();
            counter_ = 0;
            num_waiting_threads_ = num_waiting_threads;
        }
        ~ThreadState() {
            delete condition_variable_;
            delete mutex_;
        }
};

void signal_fn(ThreadState* thread_state) {
    // 确保数据一致性
    thread_state->mutex_->lock();
    while (thread_state->counter_ < thread_state->num_waiting_threads_) {
        thread_state->mutex_->unlock();
        // Release the mutex before calling `notify_all()` to make sure
        // waiting threads have a chance to make progress.
        thread_state->condition_variable_->notify_all();
        // Re-acquire the mutex to read the shared counter again.
        thread_state->mutex_->lock();
    }
    thread_state->mutex_->unlock();
}

void wait_fn(ThreadState* thread_state) {
    // 必须加锁才能等待条件变量。
    // 当调用 `wait()` 时，该锁会在线程休眠前被原子释放。
    // 调用`wait()`时释放。 当调用
    // 使用 `notify_all()` 唤醒线程时，会原子地重新获取锁。
    std::unique_lock<std::mutex> lk(*thread_state->mutex_);
    thread_state->condition_variable_->wait(lk);
    // 用重新获得的锁递增共享计数器，以通知
    // 信号线程已成功获取了锁。
    // 唤醒。
    thread_state->counter_++;
    printf("counter_ = %d...\n", thread_state->counter_);
    printf("Lock re-acquired after wait()...\n");
    lk.unlock();
}

/*
 * Signaling thread spins until each waiting thread increments a shared
 * counter after being woken up from the `wait()` method.
 */
void condition_variable_example() {
    int num_threads = 3;

    printf("==============================================================\n");
    printf("Starting %d threads for signal-and-waiting...\n", num_threads);
    std::thread* threads = new std::thread[num_threads];
    ThreadState* thread_state = new ThreadState(num_threads-1);
    threads[0] = std::thread(signal_fn, thread_state);
    for (int i = 1; i < num_threads; i++) {
        threads[i] = std::thread(wait_fn, thread_state);
    }
    for (int i = 0; i < num_threads; i++) {
        threads[i].join();
    }
    printf("==============================================================\n");

    delete thread_state;
    delete[] threads;
}


int main(int argc, char** argv) {
   mutex_example();
   condition_variable_example();
}
