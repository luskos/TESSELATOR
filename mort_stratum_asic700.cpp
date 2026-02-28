#include <iostream>
#include <chrono>
#include <omp.h>
#include <stdint.h>
#include <array>

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

void sha256_full_work(std::array<uint32_t, 8>& s, const std::array<uint32_t, 64>& w) {
    uint32_t a = s[0]; uint32_t b = s[1]; uint32_t c = s[2]; uint32_t d = s[3];
    uint32_t e = s[4]; uint32_t f = s[5]; uint32_t g = s[6]; uint32_t h = s[7];

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
    std::cout << "--- MORT ENGINE (MSVC) ---" << std::endl;
    const uint64_t ATTEMPTS = 2000000; 
    std::array<uint32_t, 64> work;
    work.fill(0);
    std::array<uint32_t, 8> init = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    auto start = std::chrono::high_resolution_clock::now();

	#pragma omp parallel
{
    // Pre-calculate work inside the thread but outside the loop
    std::array<uint32_t, 64> local_work;
    local_work.fill(0);
    std::array<uint32_t, 8> local_init = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    #pragma omp for schedule(static)
    for (int n = 0; n < (int)ATTEMPTS; n++) {
        std::array<uint32_t, 8> s = local_init;
        // Manual unrolling: doing more work per loop iteration
        sha256_full_work(s, local_work); 
        sha256_full_work(s, local_work); 
        sha256_full_work(s, local_work); 
        sha256_full_work(s, local_work); 
    }
}

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "Speed: " << ((ATTEMPTS * 1024) / diff.count()) / 1000000.0 << " MH/s" << std::endl;
    return 0;
}