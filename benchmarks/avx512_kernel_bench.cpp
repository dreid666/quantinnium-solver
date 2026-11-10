#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#define QNS_X86 1
#else
#define QNS_X86 0
#endif

static float scalar_dot(const float* a, const float* b, std::size_t n) {
    double sum = 0.0;
    for (std::size_t i = 0; i < n; ++i) sum += static_cast<double>(a[i]) * b[i];
    return static_cast<float>(sum);
}

#if QNS_X86 && (defined(__GNUC__) || defined(__clang__))
__attribute__((target("avx512f,fma")))
static float avx512_dot(const float* a, const float* b, std::size_t n) {
    __m512 acc = _mm512_setzero_ps();
    std::size_t i = 0;
    for (; i + 16 <= n; i += 16) {
        acc = _mm512_fmadd_ps(_mm512_loadu_ps(a + i), _mm512_loadu_ps(b + i), acc);
    }
    alignas(64) float lanes[16];
    _mm512_store_ps(lanes, acc);
    double sum = 0.0;
    for (float lane : lanes) sum += lane;
    for (; i < n; ++i) sum += static_cast<double>(a[i]) * b[i];
    return static_cast<float>(sum);
}
static bool avx512_available() {
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx512f") && __builtin_cpu_supports("fma");
}
#define QNS_HAVE_RUNTIME_AVX512 1
#else
#define QNS_HAVE_RUNTIME_AVX512 0
#endif

using BenchFn = float (*)(const float*, const float*, std::size_t);
static double median_ns(BenchFn fn, const float* a, const float* b,
                        std::size_t n, int iterations) {
    std::vector<double> samples;
    samples.reserve( nine );
    volatile float sink = 0.0f;
    for (int sample = 0; sample < 9; ++sample) {
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i) sink = sink + fn(a, b, n);
        const auto end = std::chrono::steady_clock::now();
        samples.push_back(std::chrono::duration<double, std::nano>(end - start).count() / iterations);
    }
    (void)sink;
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}

int main(int argc, char** argv) {
    const std::size_t n = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : (1ULL << 20);
    const int iterations = argc > 2 ? std::max(1, std::atoi(argv[2])) : 50;
    if (n == 0) {
        std::cerr << "length must be positive\n";
        return 2;
    }
    std::vector<float> a(n), b(n);
    for (std::size_t i = 0; i < n; ++i) {
        a[i] = static_cast<float>(static_cast<int>(i % 101) - 50) * 0.001f;
        b[i] = static_cast<float>(static_cast<int>(i % 79) - 39) * 0.002f;
    }
    const float reference = scalar_dot(a.data(), b.data(), n);
    const double scalar_time = median_ns(scalar_dot, a.data(), b.data(), n, iterations);
    std::cout << "n=" << n << " iterations=" << iterations
              << " scalar_median_ns=" << std::fixed << std::setprecision(1) << scalar_time << '\n';
#if QNS_HAVE_RUNTIME_AVX512
    if (avx512_available()) {
        const float candidate = avx512_dot(a.data(), b.data(), n);
        const float tolerance = 1e-4f * std::max(1.0f, std::abs(reference));
        if (std::abs(reference - candidate) > tolerance) {
            std::cerr << "AVX-512 correctness check failed\n";
            return 1;
        }
        const double simd_time = median_ns(avx512_dot, a.data(), b.data(), n, iterations);
        const double bytes = 2.0 * static_cast<double>(n) * sizeof(float);
        std::cout << "avx512_median_ns=" << simd_time
                  << " speedup=" << (scalar_time / simd_time)
                  << " estimated_GB_s=" << (bytes / simd_time)
                  << " (kernel estimate only)\n";
    } else {
        std::cout << "AVX-512/FMA unavailable; scalar baseline only\n";
    }
#else
    std::cout << "Runtime AVX-512 implementation unavailable; scalar baseline only\n";
#endif
    std::cout << "Record CPU, compiler/flags, OS, frequency policy, and repeated-run variance.\n";
    return 0;
}
