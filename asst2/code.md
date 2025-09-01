# common
## cycletimer.h
`class CycleTimer`

SysClock，用于表示时钟的类型名

静态方法
* `static SysClock currentTicks()`
  * 获取当前cpu时间
  * `asm volatile("rdtsc" : "=a" (a), "=d" (d));`
    * 是一个rdts指令
    * 返回低32位和高32位，分别存储在 EAX 和 EDX 寄存器中
    * `volatile`表示编译器不优化
    * : "=a" (a), "=d" (d)：表示将 EAX 寄存器的值存储到变量 a 中，将 EDX 寄存器的值存储到变量 d 中。
* `static double currentSeconds()`
  * 转成秒
* `static double ticksPerSecond()`
  * the conversion from seconds to ticks.
* `static const char* tickUnits()`
  * 单位
* `static double secondsPerTick()`
  * the conversion from ticks to seconds.
* `static double msPerTick()`
  * to ms
构造函数是私有的，，表明不创建实例，因为方法都是静态的

## ppm.cpp
`void writePPMImage(int* data, int width, int height, const char *filename, int maxIterations)`

创建ppm的图像，
