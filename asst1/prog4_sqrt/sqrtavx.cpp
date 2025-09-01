#include <cmath>
#include <cstdio>
#include <immintrin.h>
#include <omp.h>


// 使用 AVX 指令集实现平方根计算（牛顿迭代法）
// N: 数组长度
// initialGuess: 迭代初始值
// values: 输入数组，存储需要计算平方根的数
// output: 输出数组，存储计算结果
// 方程：1 / guess^2 = x
void sqrt_avx(int N, float initialGuess, float* value, float* output){
    if(!value || !output || N<0) return;

    static const float kThreshold = 0.00001f;

     // 计算能被 8 整除的最大长度（AVX2 可以同时处理 8 个 float）
    int aligned_n = (N / 8) * 8;

    for(int i=0; i<aligned_n; i+=8){
        __m256 threshold = _mm256_set1_ps(kThreshold);
        __m256 guess = _mm256_set1_ps(initialGuess);
        __m256 x = _mm256_loadu_ps(&value[i]);
        // 误差 |guess^2 * x - 1|
        /*
             `-0.0f` 的二进制表示是 `1000 0000 0000 0000 0000 0000 0000 0000`
            `_mm256_andnot_ps(a, b)` 的操作相当于 `~a & b`
        */
        __m256 error = _mm256_sub_ps(_mm256_mul_ps(_mm256_mul_ps(guess, guess), x), _mm256_set1_ps(1.f));
        error = _mm256_andnot_ps(_mm256_set1_ps(-0.f), error);

        // _mm256_movemask_ps 是 AVX 指令集中的一个函数，用于从 256 位浮点向量中提取每个元素的符号位，并将这些符号位组合成一个 8 位的整数。
        // 只要又元素的误差大于阈值，就继续迭代
        while(_mm256_movemask_ps(_mm256_cmp_ps(error, threshold, _CMP_GT_OQ))!=0){
            guess = _mm256_mul_ps(
                _mm256_set1_ps(0.5f),
                _mm256_sub_ps(
                    _mm256_mul_ps(_mm256_set1_ps(3.f), guess),
                    _mm256_mul_ps(
                        _mm256_mul_ps(x, guess),
                        _mm256_mul_ps(guess, guess)
                    )
                )
            );
            error = _mm256_sub_ps(_mm256_mul_ps(_mm256_mul_ps(guess, guess), x), _mm256_set1_ps(1.f));
            error = _mm256_andnot_ps(_mm256_set1_ps(-0.f), error);
        }
        _mm256_storeu_ps(&output[i], _mm256_mul_ps(x, guess));
    }

    for(int i=aligned_n; i<N; i++){
        float x = value[i];
        float guess = initialGuess;
        float error = fabs(guess * guess * x - 1.f);
        while(error > kThreshold){
            guess = (3.f * guess - x * guess * guess * guess) * 0.5f;
            error = fabs(guess * guess * x - 1.f);
        }
        output[i] = x * guess;
    }
}