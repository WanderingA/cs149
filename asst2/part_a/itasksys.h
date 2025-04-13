#ifndef _ITASKSYS_H
#define _ITASKSYS_H
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

typedef int TaskID;

class IRunnable {
    public:
        virtual ~IRunnable();

        /*
          Executes an instance of the task as part of a bulk task launch.
          
           - task_id: the current task identifier. This value will be
              between 0 and num_total_tasks-1.
              
           - num_total_tasks: the total number of tasks in the bulk
             task launch.
         */
        virtual void runTask(int task_id, int num_total_tasks) = 0;
};

class ITaskSystem {
    public:
        /*
          Instantiates a task system.

           - num_threads: 这个task sys中最大的线程数
         */
        ITaskSystem(int num_threads);
        virtual ~ITaskSystem();
        virtual const char* name() = 0;

        /*
          执行一个批量任务，任务总数为 num_total_tasks。 
          任务的执行与调用线程同步，因此 run()
          只有在所有任务执行完毕后才会返回。
        */
        virtual void run(IRunnable* runnable, int num_total_tasks) = 0;

        /*
          执行一次异步批量任务启动，启动量为
          的批量任务，但依赖于之前启动的任务。

          任务运行时必须完成与所有批量任务启动中引用的任务的执行。
          数组 `deps` 中引用的所有批量任务启动相关的任务后，才能开始执行任务。
·
          调用者必须调用 sync() 以保证完成任务的完成。
 
          返回一个标识符，可在后续调用
          runAsnycWithDeps()时使用的标识符。
          批量任务启动对该批量任务启动的依赖性。
         */
        virtual TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                        const std::vector<TaskID>& deps) = 0;

        /*
          Blocks until all tasks created as a result of **any prior**
          runXXX calls are done.
         */
        virtual void sync() = 0;
};
#endif
