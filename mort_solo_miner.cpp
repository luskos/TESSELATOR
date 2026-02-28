#include <iostream>
#include <string>
#include <chrono>
#include <omp.h>
#include <winsock2.h>
#include <immintrin.h> 
#include <stdint.h>

#pragma comment(lib, "ws2_32.lib")

#define V_SET(x) _mm256_set1_epi32(x)
#define V_ADD(a, b) _mm256_add_epi32(a, b)
#define V_XOR(a, b) _mm256_xor_si256(a, b)
#define V_AND(a, b) _mm256_and_si256(a, b)
#define V_SRL(a, n) _mm256_srli_epi32(a, n)
#define V_SLL(a, n) _mm256_slli_epi32(a, n)
#define V_ROR(x, n) V_XOR(V_SRL(x, n), V_SLL(x, 32-n))

#define S0(x) V_XOR(V_ROR(x, 2), V_XOR(V_ROR(x, 13), V_ROR(x, 22)))
#define S1(x) V_XOR(V_ROR(x, 6), V_XOR(V_ROR(x, 11), V_ROR(x, 25)))
#define CH(x, y, z) V_XOR(V_AND(x, y), V_AND(_mm256_andnot_si256(x, V_SET(0xFFFFFFFF)), z))
#define MAJ(x, y, z) V_XOR(V_AND(x, y), V_XOR(V_AND(x, z), V_AND(y, z)))

#define SHA_ROUND(a, b, c, d, e, f, g, h, k, w_val) { \
    __m256i t1 = V_ADD(h, V_ADD(S1(e), V_ADD(CH(e, f, g), V_ADD(V_SET(k), w_val)))); \
    __m256i t2 = V_ADD(S0(a), MAJ(a, b, c)); \
    d = V_ADD(d, t1); \
    h = V_ADD(t1, t2); \
}

__forceinline void sha256_step_avx2(__m256i* s, const __m256i* w) {
    __m256i a = s, b = s, c = s, d = s;
    __m256i e = s, f = s, g = s, h = s;

    static const uint32_t K = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a317, 0xc67178f2
    };

    for (int i = 0; i < 64; i++) {
        SHA_ROUND(a, b, c, d, e, f, g, h, K[i], w[i]); 
        __m256i tmp = h; h = g; g = f; f = e; e = d; d = c; c = b; b = a; a = tmp;
    }

    s = V_ADD(s, a); s = V_ADD(s, b); s = V_ADD(s, c); s = V_ADD(s, d);
    s = V_ADD(s, e); s = V_ADD(s, f); s = V_ADD(s, g); s = V_ADD(s, h);
}

int main() {
    const uint64_t BATCH = 500000; 
    uint32_t nonce_offset = 0;
    uint64_t total_hashes = 0;
    auto window_start = std::chrono::high_resolution_clock::now();

    while (true) {
        #pragma omp parallel reduction(+:total_hashes)
        {
            __m256i work_vec; 
            for(int i=0; i<64; i++) work_vec[i] = V_SET(0);

            #pragma omp for schedule(static)
            for (int n = 0; n < (int)BATCH; n++) {
                uint32_t base_n = nonce_offset + (n * 8);
                work_vec = _mm256_set_epi32(base_n, base_n+1, base_n+2, base_n+3, 
                                               base_n+4, base_n+5, base_n+6, base_n+7);

                __m256i s;
                s = V_SET(0x6a09e667); s = V_SET(0xbb67ae85); s = V_SET(0x3c6ef372); s = V_SET(0xa54ff53a);
                s = V_SET(0x510e527f); s = V_SET(0x9b05688c); s = V_SET(0x1f83d9ab); s = V_SET(0x5be0cd19);

                sha256_step_avx2(s, work_vec); 
                sha256_step_avx2(s, work_vec); 

                __m256i cmp = _mm256_cmpeq_epi32(s, V_SET(0x00000000));
                if (!_mm256_testz_si256(cmp, cmp)) {
                    #pragma omp critical
                    std::cout << "[!!!] SOLUTION FOUND" << std::endl;
                }
                total_hashes += 8;
            }
        }
        nonce_offset += (BATCH * 8);
        
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - window_start;
        if (elapsed.count() >= 5.0) {
            std::cout << "Performance: " << (total_hashes / elapsed.count()) / 1000000.0 << " MH/s" << std::endl;
            total_hashes = 0;
            window_start = std::chrono::high_resolution_clock::now();
        }
    }
    return 0;
}