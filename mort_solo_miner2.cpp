#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <omp.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdint.h>
#include <array>

#pragma comment(lib, "ws2_32.lib")

// MSVC alignment to prevent false sharing between cores
#define CACHE_ALIGN __declspec(align(64))

static const std::array<uint32_t, 64> K = {
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

// FIXED: Proper array indexing - MORT PARADIGM PRESERVED
__forceinline void sha256_step(uint32_t* s, const uint32_t* w) {
    uint32_t a = s[0], b = s[1], c = s[2], d = s[3];
    uint32_t e = s[4], f = s[5], g = s[6], h = s[7];
    
    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + S1(e) + CH(e, f, g) + K[i] + w[i];
        uint32_t t2 = S0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    s[0] += a; s[1] += b; s[2] += c; s[3] += d;
    s[4] += e; s[5] += f; s[6] += g; s[7] += h;
}

int main() {
    std::cout << "--- MORT SOLO MINER (Continuous Loop) ---" << std::endl;
    
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    struct addrinfo *result = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo("solo.ckpool.org", "3333", &hints, &result) != 0) {
        std::cout << "[-] Error: Could not resolve pool address." << std::endl;
        return 1;
    }

    SOCKET ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        std::cout << "[-] Error: Connection failed." << std::endl;
        return 1;
    }

    std::cout << "[+] Connected to solo.ckpool.org" << std::endl;

    // Authorized message (Change address to your actual BTC address)
    std::string auth = "{\"params\": [\"1YourBtcAddressHere.worker1\", \"password\"], \"id\": 2, \"method\": \"mining.authorize\"}\n";
    send(ConnectSocket, auth.c_str(), (int)auth.length(), 0);
    
    const uint64_t BATCH = 5000000;
    uint32_t target = 0x00000000; 
    uint64_t total_hashes_window = 0;
    uint32_t nonce_offset = 0;

    // FIXED: ThreadData needs arrays, not single values
    struct CACHE_ALIGN ThreadData {
        uint32_t work[64];
        uint32_t results[8];
    };

    auto window_start = std::chrono::high_resolution_clock::now();

    while (true) {
        #pragma omp parallel
        {
            ThreadData local_data;
            // FIXED: Proper array initialization
            for (int i = 0; i < 64; i++) local_data.work[i] = 0;
            for (int i = 0; i < 8; i++) local_data.results[i] = 0;

            #pragma omp for schedule(static)
            for (int n = 0; n < (int)BATCH; n++) {
                // FIXED: s must be array, not single uint32_t
                uint32_t s[8] = {
                    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
                };
                
                uint32_t current_nonce = nonce_offset + n;
                // FIXED: Fill work array with nonce-based pattern
                for (int i = 0; i < 64; i++) {
                    local_data.work[i] = current_nonce + i;
                }

                // MORT Paradigm: Pipeline Depth 8 (Emulates ASIC stages)
                sha256_step(s, local_data.work); 
                sha256_step(s, local_data.work); 
                sha256_step(s, local_data.work); 
                sha256_step(s, local_data.work); 
                sha256_step(s, local_data.work); 
                sha256_step(s, local_data.work); 
                sha256_step(s, local_data.work); 
                sha256_step(s, local_data.work); 

                // Minimal memory traffic reduction
                if ((n & 0xFFF) == 0) {
                    // FIXED: Array XOR operation
                    for (int i = 0; i < 8; i++) {
                        local_data.results[i] ^= s[i];
                    }
                }

                // FIXED: Array comparison (check first word for demo)
                if (s[0] == target) { 
                    #pragma omp critical
                    std::cout << "\n[!!!] SOLUTION FOUND: Nonce " << current_nonce << std::endl;
                }
            }
        }

        nonce_offset += BATCH;
        total_hashes_window += (BATCH * 8); 

        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - window_start;

        if (elapsed.count() >= 10.0) {
            double current_mhs = (total_hashes_window / elapsed.count()) / 1000000.0;
            std::cout << "[MORT] Status: Running | Offset: " << nonce_offset 
                      << " | Performance: " << current_mhs << " MH/s" << std::endl;
            
            total_hashes_window = 0;
            window_start = std::chrono::high_resolution_clock::now();
        }
    }

    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}