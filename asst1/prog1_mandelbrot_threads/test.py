import subprocess
import time
import re  # 用于正则表达式匹配

def run_cpp_program(num_threads, row_num):
    process = subprocess.Popen(
        ['./mandelbrot', '--threads', str(num_threads), '--row', str(row_num)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    stdout, stderr = process.communicate()
    return process.returncode, stdout.decode('utf-8'), stderr.decode('utf-8')

def test_performance(max_threads):
    results = {}
    max_speedup = 0
    best_num_threads = 0
    best_row_num = 0
    for num_threads in range(2, max_threads + 1):
        for row in range(8, 128, 8):  
            returncode, output, error = run_cpp_program(num_threads, row)
            
            results[num_threads] = {
                'return_code': returncode,
                'output': output,
                'error': error
            }
            
            
            # 打印 C++ 程序的输出
            print(output)
            if error:
                print(f"错误: {error}")

            # 提取加速比
            speedup_match = re.search(r'\(([\d.]+)x speedup from (\d+) threads\)', output)
            if speedup_match:
                speedup = speedup_match.group(1)
                print(f"从 {num_threads} 个线程得到的加速比: {speedup}x")
                if float(speedup) > max_speedup:
                    max_speedup = float(speedup)
                    best_num_threads = num_threads
                    best_row_num = row
    print(f"最佳线程数: {best_num_threads}, 最大加速比: {max_speedup}x, 行数: {best_row_num}")
    return results

if __name__ == "__main__":
    max_threads = 16  # 设置最大线程数
    test_performance(max_threads)
