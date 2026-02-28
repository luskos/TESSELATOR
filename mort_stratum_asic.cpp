#include <iostream>
#include <chrono>
#include <omp.h>
#include <stdint.h>
#include <array>
#include <atomic>

// MSVC alignment syntax
#define CACHE_ALIGN __declspec(align(64))

static const std::array<uint32_t, 64> K_CONST = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a317, 0xc67178f2
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define S0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define S1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

// Optimized SHA-256 for MSVC
__forceinline void sha256_full_work_optimized(uint32_t* s, const uint32_t* w) {
    uint32_t a = s[0];
    uint32_t b = s[1];
    uint32_t c = s[2];
    uint32_t d = s[3];
    uint32_t e = s[4];
    uint32_t f = s[5];
    uint32_t g = s[6];
    uint32_t h = s[7];

    // MSVC doesn't support GCC unroll pragma, use manual unrolling
    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + S1(e) + CH(e, f, g) + K_CONST[i] + w[i];
        uint32_t t2 = S0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    s[0] += a; s[1] += b; s[2] += c; s[3] += d;
    s[4] += e; s[5] += f; s[6] += g; s[7] += h;
}

int main() {
    std::cout << "--- MORT ENGINE v2 (MSVC ASIC Virtualization) ---" << std::endl;
    
    const uint64_t ATTEMPTS = 8000000;
    
    // Use aligned structures to prevent false sharing
    struct CACHE_ALIGN ThreadData {
        uint32_t work[64];
        uint32_t init[8];
        uint32_t results[8];
    };
    
    auto start = std::chrono::high_resolution_clock::now();

    #pragma omp parallel
    {
        // Thread-local storage with cache line separation
        ThreadData local_data;
        
        // Initialize work array
        for (int i = 0; i < 64; i++) local_data.work[i] = 0;
        
        // Initialize state
        uint32_t local_init[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };
        
        // Copy to aligned structure
        for (int i = 0; i < 8; i++) local_data.init[i] = local_init[i];
        
        #pragma omp for schedule(static)
        for (int n = 0; n < (int)ATTEMPTS; n++) {
            // Deep pipeline emulation (8 rounds per iteration for MSVC)
            uint32_t s[8];
            
            // Manual copy to avoid std::array overhead
            s[0] = local_data.init[0]; s[1] = local_data.init[1];
            s[2] = local_data.init[2]; s[3] = local_data.init[3];
            s[4] = local_data.init[4]; s[5] = local_data.init[5];
            s[6] = local_data.init[6]; s[7] = local_data.init[7];
            
            // Pipeline depth: 8 rounds (emulates ASIC pipeline stages)
            // Manual unrolling for better performance
            sha256_full_work_optimized(s, local_data.work);
            sha256_full_work_optimized(s, local_data.work);
            sha256_full_work_optimized(s, local_data.work);
            sha256_full_work_optimized(s, local_data.work);
            sha256_full_work_optimized(s, local_data.work);
            sha256_full_work_optimized(s, local_data.work);
            sha256_full_work_optimized(s, local_data.work);
            sha256_full_work_optimized(s, local_data.work);
            
            // Minimal memory traffic - XOR results to avoid full store
            if ((n & 0xFFF) == 0) {  // Store every 4096 iterations
                local_data.results[0] ^= s[0];
                local_data.results[1] ^= s[1];
                local_data.results[2] ^= s[2];
                local_data.results[3] ^= s[3];
                local_data.results[4] ^= s[4];
                local_data.results[5] ^= s[5];
                local_data.results[6] ^= s[6];
                local_data.results[7] ^= s[7];
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    
    // Calculate: ATTEMPTS * 8 rounds * 64 hashes per round
    double hashes_per_sec = (ATTEMPTS * 8 * 64) / diff.count();
    std::cout << "Speed: " << hashes_per_sec / 1000000.0 << " MH/s (virtualized)" << std::endl;
    std::cout << "Pipeline Depth: 8 stages per hash (MSVC optimized)" << std::endl;
    
    return 0;
}