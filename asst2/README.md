
# Assignment 2: Building A Task Execution Library from the Ground Up #

**Due Thurs Oct 24, 11:59pm**

**100 points total**

## Overview ##

每个人都喜欢快速完成任务，在这项任务中，我们要求您做到这一点！ 您将实现一个 C++ 库，在多核 CPU 上尽可能高效地执行应用程序提供的任务。

在作业的第一部分，您将实现一个版本的任务执行库，该库支持同一任务的多个实例的批量（数据并行）启动。 该功能类似于作业 1 中用于跨内核并行化代码的 [ISPC 任务启动行为](http://ispc.github.io/ispc.html#task-parallelism-launch-and-sync-statements)。

在作业的第二部分，您将扩展任务运行系统，以执行更复杂的任务图，其中任务的执行可能依赖于其他任务产生的结果。 这些依赖关系会限制任务调度系统可以安全地并行运行哪些任务。  在并行机器上调度数据并行任务图的执行是许多流行的并行运行时系统的一项功能，从流行的[Thread Building Blocks](https://github.com/intel/tbb)库到[Apache Spark](https://spark.apache.org/)，再到[PyTorch](https://pytorch.org/)和[TensorFlow](https://www.tensorflow.org/)等现代深度学习框架，不一而足。

这项任务要求您

* 使用线程池管理任务执行
* 使用互斥和条件变量等同步原语协调工作线程的执行
* 实施任务调度程序，反映任务图定义的依赖关系
* 了解工作量特征，做出高效的任务调度决策

### 等等，我好像以前做过这个？ ###

您可能已经在 CS107 或 CS111 等课程中创建了线程池和任务执行库。
但是，当前的作业是更好地了解这些系统的难得机会。
您将实现多个任务执行库，其中一些不包含线程池，另一些包含不同类型的线程池。
通过实施多种任务调度策略并比较它们在不同工作负载上的性能，您将更好地理解在创建并行系统时关键设计选择的影响。

## 环境设置 ##

**我们将在亚马逊 AWS 的 "c7g.4xlarge "实例上对本作业进行评分--我们提供了设置虚拟机的说明 [此处](https://github.com/stanford-cs149/asst2/blob/master/cloud_readme.md)。 请确保您的代码能在该虚拟机上运行，因为我们将使用该虚拟机进行性能测试和评分。**

作业启动代码可在 [Github](https://github.com/stanford-cs149/asst2) 上获取。 请从以下网址下载作业 2 的启动代码：

    https://github.com/stanford-cs149/asst2/archive/refs/heads/master.zip

**IMPORTANT:** 切勿修改所提供的 "Makefile"。 否则可能会破坏我们的分级脚本。

## Part A: Synchronous Bulk Task Launch

在作业 1 中，您使用 ISPC 的任务启动原语启动了 N 个 ISPC 任务实例（"launch[N] myISPCFunction()"）。  在本作业的第一部分，您将在任务执行库中实现类似的功能。

要开始使用，请先了解 `itasksys.h` 中 `ITaskSystem` 的定义。 这个[抽象类](https://www.tutorialspoint.com/cplusplus/cpp_interfaces.htm) 定义了任务执行系统的接口。  该接口有一个方法 `run()`，其签名如下：

    virtual void run(IRunnable* runnable, int num_total_tasks) = 0;

run()`执行指定任务的`num_total_tasks`实例。  由于单次函数调用会导致许多任务的执行，因此我们将每次对 `run()` 的调用称为_bulk task launch_。

tasksys.cpp "中的启动代码包含一个正确但串行的 "TaskSystemSerial::run() "实现，可作为任务系统如何使用 "IRunnable "接口执行批量任务启动的示例（"IRunnable "的定义在 "itasksys.h "中）。 (请注意，在每次调用 `IRunnable::runTask()` 时，任务系统都会为任务提供当前任务标识符（介于 0 和 `num_total_tasks` 之间的整数）以及批量任务启动中的任务总数。  任务的实现将使用这些参数来决定任务应做的工作。

run()`的一个重要细节是，它必须与调用线程同步执行任务。  换句话说，当调用`run()`返回时，应用程序保证任务系统已完成批量任务启动中 ****all tasks**** 的执行。  启动代码中提供的`run()`串行实现在调用线程上执行所有任务，因此满足了这一要求。

### Running Tests ###

启动代码包含一套使用任务系统的测试程序。 有关测试程序的描述，请参阅 `tests/README.md`，有关测试定义本身，请参阅 `tests/tests.h`。 要运行测试，请使用 `runtasks` 脚本。 例如，要运行名为 `mandelbrot_chunked` 的测试，该测试使用批量启动的任务计算曼德尔布罗分形的图像，每个任务处理图像的一个连续块，请键入

```bash
./runtasks -n 16 mandelbrot_chunked
```


不同的测试具有不同的性能特征--有些测试在每个任务中只做很少的工作，有些测试则要进行大量的处理。  有些测试在每次启动时创建大量任务，有些则很少。  有时，一次启动中的所有任务都有相似的计算成本。  而在其他情况下，单个批量启动任务的成本是可变的。 我们已在 `tests/README.md` 中描述了大部分测试，但我们建议您查看 `tests/tests.h` 中的代码，以更详细地了解所有测试的行为。

在执行解决方案时，有一个测试可能有助于调试正确性，那就是 `simple_test_sync`，这是一个非常小的测试，不应该用于测量性能，但它足够小，可以用打印语句或调试器进行调试。 请参见 `tests/tests.h` 中的函数 `simpleTest`。

我们鼓励您创建自己的测试。 请查看 `tests/tests.h` 中的现有测试，以获得灵感。 我们还包含了一个由 `class YourTask` 和函数 `yourTest()` 组成的骨架测试，供您在此基础上创建测试。 对于您创建的测试，请确保将它们添加到 `tests/main.cpp` 中的测试和测试名称列表中，并相应调整变量 `n_tests`。 请注意，虽然您可以使用解决方案运行自己的测试，但无法编译参考解决方案来运行您的测试。

命令行选项"-n "指定任务系统实施可使用的最大线程数。  在上面的例子中，我们选择了 `-n 16`，因为 AWS 实例的 CPU 有 16 个执行上下文。  可通过命令行帮助（`-h` 命令行选项）查看可运行的全部测试列表。

命令行选项"-i "指定了性能测量期间运行测试的次数。 为了精确测量性能，`./runtasks` 会多次运行测试，并记录多次运行的_最小_运行时间；一般来说，默认值就足够了--更大的值可能会产生更精确的测量结果，但代价是更长的测试运行时间。

此外，我们还将为您提供用于性能分级的测试线束：

```bash
>>> python3 ../tests/run_test_harness.py
```

线束有以下命令行参数

```bash
>>> python3 run_test_harness.py -h
usage: run_test_harness.py [-h] [-n NUM_THREADS]
                           [-t TEST_NAMES [TEST_NAMES ...]] [-a]

Run task system performance tests

optional arguments:
  -h, --help            show this help message and exit
  -n NUM_THREADS, --num_threads NUM_THREADS
                        Max number of threads that the task system can use. (16
                        by default)
  -t TEST_NAMES [TEST_NAMES ...], --test_names TEST_NAMES [TEST_NAMES ...]
                        List of tests to run
  -a, --run_async       Run async tests
```

它生成的详细性能报告如下所示：

```bash
>>> python3 ../tests/run_test_harness.py -t super_light super_super_light
python3 ../tests/run_test_harness.py -t super_light super_super_light
================================================================================
Running task system grading harness... (2 total tests)
  - Detected CPU with 16 execution contexts
  - Task system configured to use at most 16 threads
================================================================================
================================================================================
Executing test: super_super_light...
Reference binary: ./runtasks_ref_linux
Results for: super_super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                9.053     9.022       1.00  (OK)
[Parallel + Always Spawn]               8.982     33.953      0.26  (OK)
[Parallel + Thread Pool + Spin]         8.942     12.095      0.74  (OK)
[Parallel + Thread Pool + Sleep]        8.97      8.849       1.01  (OK)
================================================================================
Executing test: super_light...
Reference binary: ./runtasks_ref_linux
Results for: super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                68.525    68.03       1.01  (OK)
[Parallel + Always Spawn]               68.178    40.677      1.68  (NOT OK)
[Parallel + Thread Pool + Spin]         67.676    25.244      2.68  (NOT OK)
[Parallel + Thread Pool + Sleep]        68.464    20.588      3.33  (NOT OK)
================================================================================
Overall performance results
[Serial]                                : All passed Perf
[Parallel + Always Spawn]               : Perf did not pass all tests
[Parallel + Thread Pool + Spin]         : Perf did not pass all tests
[Parallel + Thread Pool + Sleep]        : Perf did not pass all tests
```

在上述输出中，`PERF` 是您的实现的运行时间与参考解决方案的运行时间之比。 因此，小于 1 的值表示您的任务系统实现比参考实现更快。

> [!TIP]
> Mac users: While we provided reference solution binaries for both part a and part b, we will be testing your code using the linux binaries. Therefore, we recommend you check your implementation in the AWS instance before submitting. If you are using a newer Mac with an M1 chip, use the `runtasks_ref_osx_arm` binary when testing locally. Otherwise, use the `runtasks_ref_osx_x86` binary.

> [!IMPORTANT]
我们将使用 `runtasks_ref_linux_arm` 版本的参考解决方案对您在 AWS 上的解决方案进行评分。 请确保您的解决方案能在 AWS ARM 实例上正常运行。

### What You Need To Do ###

您的任务是实现一个任务执行引擎，有效利用多核 CPU。 我们将根据您实现的正确性（必须正确运行所有任务）和性能对您进行评分。  这应该是一个有趣的编码挑战，但也是一项非同小可的工作。 为了帮助您保持正确的方向，在完成作业的 A 部分时，我们会让您实现多个版本的任务系统，慢慢提高实现的复杂性和性能。  您的三个实现将包含在 `tasksys.cpp/.h` 中定义的类中。

* `TaskSystemParallelSpawn`
* `TaskSystemParallelThreadPoolSpinning`
* `TaskSystemParallelThreadPoolSleeping`

__在 `part_a/` 子目录中执行 A 部分的实现，以便与正确的参考实现 (`part_a/runtasks_ref_*`)进行比较。

专业提示：请注意下面的说明是如何采取 "先尝试最简单的改进 "的方法的。 每一步都会增加任务执行系统实现的复杂性，但每走一步，你就会拥有一个可以运行的（完全正确的）任务运行系统。

我们还希望您至少创建一个测试，可以测试正确性或性能。 更多信息，请参阅上文 "运行测试 "部分。

#### Step 1: Move to a Parallel Task System ####

__In this step please implement the class `TaskSystemParallelSpawn`.__

启动代码在 `TaskSystemSerial` 中为您提供了任务系统的工作串行实现。  在这一步作业中，您将扩展启动代码以并行执行批量任务启动。

* 您将需要创建额外的控制线程来执行批量任务启动的工作。  请注意，`TaskSystem` 的构造函数提供了一个参数 `num_threads`，这是您的实现在运行任务时可以使用的 **** 最大工作线程数****。

* 本着 "先做最简单的事 "的精神，我们建议您在 `run()` 开始时创建工作线程，并在 `run()` 返回之前从主线程中加入这些线程。  这将是一种正确的实现方式，但会因频繁创建线程而产生大量开销。

* 如何为工作线程分配任务？  应该考虑静态还是动态地将任务分配给线程？

* 是否有共享变量（任务执行系统的内部状态）需要保护，以免多个线程同时访问？  您可以查看我们的[C++ 同步教程](tutorial/README.md)，了解有关 C++ 标准库中同步原语的更多信息。

#### Step 2: Avoid Frequent Thread Creation Using a Thread Pool ####

__In this step please implement the class `TaskSystemParallelThreadPoolSpinning`.__

由于每次调用 `run()`时都要创建线程，因此步骤 1 中的实现会产生开销。  当任务的计算成本较低时，这种开销尤其明显。  此时，我们建议您转而采用 "线程池 "实现，即您的任务执行系统会预先创建所有工作线程（例如，在构建 "任务系统 "时，或在首次调用 "运行() "时）。

* 作为起步实施，我们建议您将工作线程设计为持续循环，并始终检查是否有更多工作要执行。 (进入 while 循环直到条件为真的线程通常被称为 "旋转"）。  工作线程如何确定还有工作要做？

* 现在要确保 `run()` 实现所需的同步行为并非易事。  您需要如何更改 `run()` 的实现，以确定批量任务启动中的所有任务都已完成？

#### Step 3: Put Threads to Sleep When There is Nothing to Do ####

__In this step please implement the class `TaskSystemParallelThreadPoolSleeping`.__

步骤 2 实现的缺点之一是，线程在 "旋转 "等待任务时会占用 CPU 内核的执行资源。  例如，工作线程可能会循环等待新任务的到来。  再比如，主线程可能会循环等待工作线程完成所有任务，这样它就可以从对 `run()` 的调用中返回。  这会损害性能，因为 CPU 资源被用于运行这些线程，即使这些线程并没有执行有用的工作。

在这部分作业中，我们希望您通过让线程休眠，直到满足它们所等待的条件，来提高任务系统的效率。

* 您可以选择使用条件变量来实现这一行为。  条件变量是一种同步原语，可以让线程在等待条件存在时处于休眠状态（不占用 CPU 处理资源）。 其他线程会 "发出信号 "唤醒等待的线程，以查看它们所等待的条件是否已经满足。 例如，如果没有工作要做，就可以让工作线程进入休眠状态（这样它们就不会占用试图执行有用工作的线程的 CPU 资源）。  再比如，调用 "run() "的应用程序主线程可能希望在等待工作线程完成批量任务启动中的所有任务时处于休眠状态。 (否则旋转的主线程将占用工作线程的 CPU 资源！）。  有关 C++ 中条件变量的更多信息，请参阅我们的 [C++ 同步教程](tutorial/README.md)。

* 您在这部分任务中的实现可能需要考虑棘手的竞赛条件。  你需要考虑许多可能的线程行为交错。

* 你可能需要考虑编写额外的测试用例来锻炼你的系统。  作业启动代码包括评分脚本将用来对您的代码性能进行评分的工作负载，但我们还将使用启动代码中没有提供的更广泛的工作负载集来测试您的实现的正确性！___

## Part B: Supporting Execution of Task Graphs

在作业的 B 部分，您将扩展 A 部分任务系统的实现，以支持异步启动可能依赖于先前任务的任务。  这些任务间的依赖关系会产生调度约束，您的任务执行库必须遵守这些约束。

The `ITaskSystem` interface has an additional method:

    virtual TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                    const std::vector<TaskID>& deps) = 0;

runAsyncWithDeps()`与`run()`类似，也用于批量启动`num_total_tasks`任务。 然而，它与 `run()` 有许多不同之处...

#### Asynchronous Task Launch ####

首先，使用 `runAsyncWithDeps()` 创建的任务由任务系统与调用线程_异步_执行。 这意味着，即使任务尚未执行完毕，`runAsyncWithDeps()` 也应_立即_返回给调用者。 该方法会返回一个与批量任务启动相关的唯一标识符。

调用线程可通过调用 `sync()` 来确定批量任务启动何时实际完成。

    virtual void sync() = 0;

只有当与之前所有批量任务启动相关的任务都已完成时，__才会向调用者返回 `sync()`：

    // assume taskA and taskB are valid instances of IRunnable...

    std::vector<TaskID> noDeps;  // empty vector

    ITaskSystem *t = new TaskSystem(num_threads);

    // bulk launch of 4 tasks
    TaskID launchA = t->runAsyncWithDeps(taskA, 4, noDeps);

    // bulk launch of 8 tasks
    TaskID launchB = t->runAsyncWithDeps(taskB, 8, noDeps);

    // at this point tasks associated with launchA and launchB
    // may still be running

    t->sync();

    // at this point all 12 tasks associated with launchA and launchB
    // are guaranteed to have terminated

如上文的注释所述，在线程调用 `sync()` 之前，调用线程无法保证之前调用 `runAsyncWithDeps()` 的任务已经完成。  准确地说，`runAsyncWithDeps()` 会告诉您的任务系统执行新的批量任务启动，但您的实现可以灵活地在下一次调用`sync()` 之前的任何时间执行这些任务。  请注意，此规范并不保证您的实现会在启动启动 B 的任务之前执行启动 A 的任务！

#### Support for Explicit Dependencies ####

runAsyncWithDeps()`的第二个关键细节是它的第三个参数：一个由 TaskID 标识符组成的向量，该标识符必须指向之前使用`runAsyncWithDeps()`启动的批量任务。  该向量指定了当前批量任务启动中的任务所依赖的先前任务。 因此，在依赖向量中给出的所有启动任务完成之前，任务运行时不能开始执行当前批量任务启动中的任何任务！__ 例如，请考虑下面的示例：

    std::vector<TaskID> noDeps;  // empty vector
    std::vector<TaskID> depOnA;
    std::vector<TaskID> depOnBC;

    ITaskSystem *t = new TaskSystem(num_threads);

    TaskID launchA = t->runAsyncWithDeps(taskA, 128, noDeps);
    depOnA.push_back(launchA);

    TaskID launchB = t->runAsyncWithDeps(taskB, 2, depOnA);
    TaskID launchC = t->runAsyncWithDeps(taskC, 6, depOnA);
    depOnBC.push_back(launchB);
    depOnBC.push_back(launchC);

    TaskID launchD = t->runAsyncWithDeps(taskD, 32, depOnBC);
    t->sync();

上述代码包含四个批量任务启动（任务 A：128 个任务；任务 B：2 个任务；任务 C：6 个任务；任务 D：32 个任务）。  请注意，任务 B 和任务 C 的启动取决于任务 A。 任务 D 的批量启动（"启动 D"）取决于 "启动 B "和 "启动 C "的结果。  因此，虽然允许任务运行时以任何顺序（包括并行）处理与 `launchB` 和 `launchC` 相关的任务，但这些启动的所有任务必须在 `launchA` 的任务完成后才开始执行，而且必须在运行时开始执行 `launchD` 的任何任务之前完成。

我们可以用__任务图__来直观地说明这些依赖关系。 任务图是一个有向无环图（DAG），图中的节点对应于批量任务的启动，从节点 X 到节点 Y 的边表示 Y 依赖于 X 的输出：

<p align="center">
    <img src="figs/task_graph.png" width=400>
</p>

请注意，如果在具有八个执行上下文的 Myth 机器上运行上述示例，那么并行调度来自 `launchB` 和 `launchC` 的任务的能力可能会非常有用，因为这两个批量任务的启动本身都不足以使用机器的所有执行资源。

### Testing ###
分级工具包中包含的测试子集在 `tests/README.md` 中进行了描述，所有测试都可以在 `tests/tests.h` 中找到，并列在 `tests/main.cpp` 中。 为了调试正确性，我们提供了一个小测试 `simple_test_async`。 请查看 `tests/tests.h` 中的 `simpleTest` 函数。 `simple_test_async` 应该足够小，可以在 `simpleTest` 中使用打印语句或断点进行调试。

我们鼓励您创建自己的测试。 请查看 `tests/tests.h` 中的现有测试，从中获得灵感。 我们还包含了一个由 `class YourTask` 和函数 `yourTest()` 组成的骨架测试，供您在此基础上创建测试。 对于您创建的测试，请确保将它们添加到 `tests/main.cpp` 中的测试和测试名称列表中，并相应调整变量 `n_tests`。 请注意，虽然您可以使用解决方案运行自己的测试，但无法编译参考解决方案来运行您的测试。

### What You Need to Do ###

您必须扩展 A 部分中使用线程池（和睡眠）的任务系统实现，以正确实现 `TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps()` 和 `TaskSystemParallelThreadPoolSleeping::sync()`。 我们还希望您至少创建一个测试，可以测试正确性或性能。 更多信息请参阅上文的 "测试 "部分。 需要说明的是，您*需要*在撰写中描述自己的测试，但我们的自动跟踪器将*不*测试您的测试。
**您不需要实现 B 部分中的其他 "任务系统 "类**。

与 A 部分一样，我们为您提供以下入门提示：
* 将 `runAsyncWithDeps()` 的行为视为将与批量任务启动相对应的记录，或者与批量任务启动中的每个任务相对应的记录推送到 "工作队列 "中，可能会有所帮助。  一旦工作记录进入队列，`runAsyncWithDeps()` 就可以返回给调用者。

* 这部分任务的诀窍在于执行适当的簿记以跟踪依赖关系。 当批量任务中的所有任务启动完成后，必须做什么？ (这时可能会有新任务可以运行）。

* 在实现过程中使用两种数据结构可能会有所帮助：(1) 一种结构代表已通过调用 `runAsyncWithDeps()` 添加到系统中的任务，但尚未准备好执行，因为它们依赖于仍在运行的任务（这些任务在 "等待 "其他任务完成）；(2) 一种任务的 "准备队列"，这些任务不需要等待任何先前任务完成，只要有工作线程可以处理，就可以安全地运行。

* 在生成唯一的任务启动 ID 时，无需担心整数绕包问题。 我们不会让您的任务系统承受超过 2^31 次的批量任务启动。

* 您可以假设所有程序都只调用 `run()` 或只调用 `runAsyncWithDeps()`；也就是说，您不需要处理 `run()` 调用需要等待所有对 `runAsyncWithDeps()` 的继续调用完成的情况。 请注意，这一假设意味着您可以使用对 `runAsyncWithDeps()` 和 `sync()` 的适当调用来实现 `run()`。

* 您可以假设唯一的多线程是由您的实现创建/使用的多个线程。 也就是说，我们不会产生额外的线程，也不会从这些线程中调用你的实现。

__Implement your part B implementation in the `part_b/` sub-directory to compare to the correct reference implementation (`part_b/runtasks_ref_*`).__


## Handin ##

请使用 [Gradescope](https://www.gradescope.com/)提交您的作品。  您提交的作品应包括任务系统代码和一篇介绍您的实现过程的文章。  我们希望您提交以下五个文件：

 * part_a/tasksys.cpp
 * part_a/tasksys.h
 * part_b/tasksys.cpp
 * part_b/tasksys.h
 * Your write-up PDF (submit to gradescope write-up assignment)

#### Code Handin ####

我们要求您以压缩文件的形式提交源文件 `part_a/tasksys.cpp|.h` 和 `part_b/tasksys.cpp|.h` 。 您可以创建一个包含子目录 `part_a` 和 `part_b` 的目录（例如命名为 `asst2_submission` ），将相关文件放入其中，通过运行 `tar -czvf asst2.tar.gz asst2_submission` 来压缩该目录，然后上传。 请将**压缩文件**`asst2.tar.gz`提交到 Gradescope 上的作业 *Assignment 2 (Code)*。

在提交源文件之前，请确保所有代码都可编译和运行！ 我们应该能够将这些文件放入一个干净的启动代码树中，输入 `make`，然后执行您的程序，而无需人工干预。

我们的评分脚本将运行启动代码中提供给您的检查器代码，以确定性能点。  我们还将在启动代码中未提供的其他应用程序上运行您的代码，以进一步测试其正确性！_评分脚本将在作业*后运行。




#### Writeup Handin ####

请就 Gradescope 上的作业 *Assignment 2（Write-up）* 提交一份简短的书面材料，内容如下：

 1. 描述您的任务系统实施情况（1 页即可）。  除了对其工作原理的一般描述外，请务必回答以下问题：
  * 您是如何决定管理线程的？
  * 您的系统如何为工作线程分配任务？ 使用的是静态分配还是动态分配？
  * 您如何跟踪 B 部分中的依赖关系以确保任务图的正确执行？

 2. 在 A 部分中，您可能已经注意到，较简单的任务系统实现（如完全串行实现或每次启动都生成线程的实现）与较先进的实现相比，性能相当，有时甚至更好。  请解释为什么会出现这种情况，并举出某些测试作为例子。  例如，顺序任务系统实现在什么情况下表现最好？ 为什么？  在哪些情况下，"每次启动都产生 "的实现方法与使用线程池的更先进并行实现方法性能相同？  什么情况下表现不好？
 3. 描述你为本次作业实施的一个测试。 该测试是做什么的？它的目的是检查什么？你是如何验证你的作业解决方案在测试中表现良好的？ 你添加的测试结果是否导致你改变了作业的实现？



