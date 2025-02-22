#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <queue>
#include <stdexcept>
#include <functional>
#include <mutex>
#include <condition_variable>

// lvalue - existing objects
// rvalue - temp objects

class ThreadPool {
private:
    int pool_size_;
    std::vector<std::thread> thread_pool_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mtx;
    std::condition_variable cv;
    bool stop_;
public:
    ThreadPool(int pool_size) : pool_size_{ pool_size }, stop_(false) {
        for (int i = 0; i < pool_size_; i++) {
            // using instead of push_back(copying operation) because it's moving operation
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
            th.join(); // Wait for all threads to finish
        }
    }

    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            tasks_.push(std::move(task));
        }
        cv.notify_one();
    }

};

void print_hw2() {
    std::cout << "Hello world" << std::endl;
}

void print_hw(int i) {
    try {
        std::string text = "Hello world " + std::to_string(i) + "\r\n";
        std::cout << text;
    } catch (const std::exception& e) {
        std::cerr << " Caught an exception2: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "Caught an unknown exception2" << std::endl;
    }
}

int main() {
    ThreadPool tp(4);
    tp.enqueue(print_hw2);

    tp.shutdown();
    return 0;
}