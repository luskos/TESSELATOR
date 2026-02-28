// ============================================================================
// DIGITAL_TESSELATOR.cpp - TUNED VERSION
// ============================================================================

#include <iostream>
#include <vector>
#include <complex>
#include <chrono>
#include <random>
#include <cmath>
#include <immintrin.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class DigitalTesselator {
private:
    static constexpr int GRID_SIZE = 32;
    static constexpr int TOTAL_CELLS = GRID_SIZE * GRID_SIZE;
    
    std::vector<std::complex<double>> shield;
    std::vector<double> detectors;
    double threshold;  // Make threshold adjustable
    
public:
    DigitalTesselator(double thresh = 0.5) : shield(TOTAL_CELLS), detectors(TOTAL_CELLS), threshold(thresh) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dist(0.0, 2.0 * M_PI);
        
        for (auto& phase : shield) {
            phase = std::polar(1.0, dist(gen));
        }
    }
    
    void tesselate(const std::vector<std::complex<double>>& inputWaves) {
        #pragma omp parallel for
        for (int d = 0; d < TOTAL_CELLS; d++) {
            std::complex<double> sum = 0;
            
            for (int s = 0; s < TOTAL_CELLS; s++) {
                double distance = sqrt(pow((s/GRID_SIZE) - (d/GRID_SIZE), 2) +
                                       pow((s%GRID_SIZE) - (d%GRID_SIZE), 2));
                
                sum += inputWaves[s] * shield[s] * 
                       std::polar(1.0, 2.0 * M_PI * distance / GRID_SIZE);
            }
            
            detectors[d] = std::norm(sum) / (TOTAL_CELLS * TOTAL_CELLS); // Normalize
        }
    }
    
    bool xorGate(bool a, bool b) {
        std::vector<std::complex<double>> inputs(TOTAL_CELLS, 1.0);
        
        double phaseA = a ? M_PI : 0.0;
        double phaseB = b ? M_PI : 0.0;
        
        // Interleave bits for better interference
        for (int i = 0; i < TOTAL_CELLS; i++) {
            if (i % 2 == 0) {
                inputs[i] = std::polar(1.0, phaseA);
            } else {
                inputs[i] = std::polar(1.0, phaseB);
            }
        }
        
        tesselate(inputs);
        
        double power = detectors[TOTAL_CELLS/2];
        return power > threshold;
    }
    
    void findOptimalThreshold() {
        std::cout << "\n🔍 CALIBRATING THRESHOLD:\n";
        
        for (double t = 0.1; t < 1.0; t += 0.1) {
            threshold = t;
            int correct = 0;
            
            correct += (xorGate(0,0) == 0) ? 1 : 0;
            correct += (xorGate(0,1) == 1) ? 1 : 0;
            correct += (xorGate(1,0) == 1) ? 1 : 0;
            correct += (xorGate(1,1) == 0) ? 1 : 0;
            
            std::cout << "  Threshold " << t << ": " << correct << "/4 correct\n";
        }
    }
    
    void benchmark() {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::complex<double>> inputs(TOTAL_CELLS, 1.0);
        const int ITERATIONS = 100;
        
        for (int i = 0; i < ITERATIONS; i++) {
            tesselate(inputs);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        double ops_per_tesselation = TOTAL_CELLS * TOTAL_CELLS;
        double total_ops = ITERATIONS * ops_per_tesselation;
        double gops = total_ops / ms / 1e6;
        
        std::cout << "\n⚡ BENCHMARK RESULTS:\n";
        std::cout << "  " << ITERATIONS << " iterations in " << ms << " ms\n";
        std::cout << "  " << gops << " GOPS/s\n";
        std::cout << "  " << ops_per_tesselation / 1e6 << "M ops per tesselation\n";
    }
};

int main() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════════╗
║                                                                  ║
║   MORT TESSELATOR - DIGITAL PROOF OF CONCEPT                    ║
║                                                                  ║
╚══════════════════════════════════════════════════════════════════╝
)" << std::endl;
    
    DigitalTesselator tesselator(0.5);
    
    // First find the right threshold
    tesselator.findOptimalThreshold();
    
    // Set to best threshold (you'll see from calibration)
    DigitalTesselator finalTesselator(0.4); // Adjust based on calibration
    
    std::cout << "\n🔬 FINAL XOR TEST:\n";
    std::cout << "0 XOR 0 = " << finalTesselator.xorGate(0, 0) << "\n";
    std::cout << "0 XOR 1 = " << finalTesselator.xorGate(0, 1) << "\n";
    std::cout << "1 XOR 0 = " << finalTesselator.xorGate(1, 0) << "\n";
    std::cout << "1 XOR 1 = " << finalTesselator.xorGate(1, 1) << "\n";
    
    finalTesselator.benchmark();
    
    return 0;
}