#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <queue>
#include <stdexcept>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

// lvalue - existing objects
// rvalue - temp objects

class ThreadPool {
private:
    int pool_size_;
    std::vector<std::thread> thread_pool_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mtx;
    std::condition_variable cv;
    // Using atomic for thread-safe flag operations
    std::atomic<bool> stop_;
public:
    ThreadPool(int pool_size) : pool_size_{ pool_size }, stop_(false) {
        try {
            for (int i = 0; i < pool_size_; i++) {
                // using emplace_back instead of push_back(copying operation) because it's moving operation
                // [this] - lambda capture clause
                thread_pool_.emplace_back([this] {
                    while(true) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(mtx);
                            // - on false we wait, releasing lock and go to sleep
                            //   and wait on notify to check condition again;
                            // - on true we process the task
                            cv.wait(lock, [this] {return !tasks_.empty() || stop_; });

                            if (stop_ || tasks_.empty()) {
                                return; // exit thread
                            }
                            
                            // move from ownership of reference from
                            // queue to local variable
                            task = std::move(tasks_.front());
                            tasks_.pop();
                        }
                        // cv.notify_one(); // notify any thread
                        try {
                            task(); // Execute the task
                        } catch (const std::exception& e) {
                            // Handle the exception (e.g., log it)
                            std::cerr << "Task execution error: " << e.what() << std::endl;
                        }
                    }
                });
            }
        // Catch all exception no matter the type
        } catch (...) {
            shutdown();
            // without argument re-throws the original exception
            throw;
        }
    }


    ~ThreadPool() {
        shutdown();
    }

    void shutdown() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            stop_ = true;
        }
        cv.notify_all(); // Wake all threads to check for stop condition
        for (std::thread &th : thread_pool_) {
            if (th.joinable()) {
                th.join(); // Wait for all threads to finish
            }
        }
    }

    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            if (stop_) {
                throw std::runtime_error("Cannot enqueue on stopped ThreadPool");
            }
            tasks_.push(std::move(task));
        }
        cv.notify_one();
    }

};

void print_hw() {
    std::cout << "Hello world" << std::endl;
}

void print_hw_i(int i) {
    try {
        std::string text = "Hello world " + std::to_string(i) + "\r\n";
        std::cout << text;
    } catch (const std::exception& e) {
        std::cerr << " Caught an exception: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "Caught an unknown exception" << std::endl;
    }
}

int main() {
    ThreadPool tp(4);
    tp.enqueue(print_hw);
    // lambda function
    tp.enqueue([](){ print_hw_i(1); });
    tp.enqueue([](){ print_hw_i(2); });
    tp.enqueue([](){ print_hw_i(3); });

    // wait a bit for tasks to proceed or race condition happens with shutdown
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    tp.shutdown();
    return 0;
}