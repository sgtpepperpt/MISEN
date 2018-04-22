#include "thread_handler.h"
#include "outside_util.h"

// thread_handler data
thread_data* threads = NULL;
unsigned entered_threads = 0;

// task data
unsigned nr_tasks = 0;

void thread_handler_init(unsigned nr_threads) {
    threads = (thread_data*)malloc(sizeof(thread_data) * nr_threads);
    entered_threads = 0;
    nr_tasks = 0;

    for (unsigned i = 0; i < nr_threads; ++i) {
        threads[i].lock = new std::mutex();
        threads[i].cond_var = new std::condition_variable();
        threads[i].ready = 0;
        threads[i].done = 0;
        threads[i].task_args = NULL;
    }
}

thread_data* thread_handler_add() {
    return threads + entered_threads++;
}

unsigned trusted_util::thread_get_count(){
    return entered_threads;
}

// task related
int trusted_util::thread_add_work(void* (*task)(void*), void* args) {
    if(nr_tasks < entered_threads) {
        threads[nr_tasks].task = task;
        threads[nr_tasks++].task_args = args;
        return 0;
    }

    return 1;
}

void trusted_util::thread_do_work() {
    //outside_util::printf("nr tasks %u\n", nr_tasks);
    // make threads work
    for (unsigned i = 0; i < nr_tasks; ++i) {
        thread_data* t = &threads[i];
        {
            std::lock_guard <std::mutex> lk(*t->lock);
            threads[i].ready = 1;
        }
        t->cond_var->notify_one();
    }

    // wait for completion
    for (unsigned i = 0; i < nr_tasks; ++i) {
        thread_data *t = &threads[i];
        {
            std::unique_lock <std::mutex> lk(*t->lock);
            threads[i].cond_var->wait(lk, [&t] { return t->done; });
        }

        // reset
        threads[i].done = 0;
        threads[i].task_args = NULL;
    }

    // reset for next task
    nr_tasks = 0;
}

