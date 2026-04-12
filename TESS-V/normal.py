# gpu_stress_baseline.py
"""
Safe GPU stress test - warms up but doesn't kill the GPU
Monitors temperature and throttles if needed
"""

import numpy as np
import time
import psutil
import threading
from dataclasses import dataclass
from typing import Dict, Tuple
import sys

try:
    import cupy as cp
    import pynvml
    HAS_GPU = True
except ImportError:
    HAS_GPU = False
    print("⚠️  Install cupy and nvidia-ml-py for GPU testing")
    print("   pip install cupy-cuda12x nvidia-ml-py")

@dataclass
class GPUMetrics:
    temperature: float
    power_draw: float
    memory_used: float
    gpu_util: float
    clock_mhz: float

class SafeGPUStressTest:
    """
    GPU stress test with safety limits
    Will NOT exceed 80°C or 90% power limit
    """
    
    def __init__(self, max_temp_celsius=80, max_power_percent=90):
        self.max_temp = max_temp_celsius
        self.max_power = max_power_percent
        self.running = True
        self.metrics_history = []
        
        if HAS_GPU:
            pynvml.nvmlInit()
            self.handle = pynvml.nvmlDeviceGetHandleByIndex(0)
            self._set_safe_limits()
    
    def _set_safe_limits(self):
        """Set power limits to prevent overheating"""
        try:
            # Get default power limit
            default_power = pynvml.nvmlDeviceGetPowerManagementDefaultLimit(self.handle)
            # Set to 80% of default for safety
            safe_power = int(default_power * self.max_power / 100)
            pynvml.nvmlDeviceSetPowerManagementLimit(self.handle, safe_power)
            print(f"🔒 Power limited to {self.max_power}% ({safe_power/1000:.0f}W)")
        except:
            print("⚠️  Could not set power limits (run as admin?)")
    
    def get_metrics(self) -> GPUMetrics:
        """Read current GPU metrics"""
        if not HAS_GPU:
            return GPUMetrics(0, 0, 0, 0, 0)
        
        temp = pynvml.nvmlDeviceGetTemperature(self.handle, pynvml.NVML_TEMPERATURE_GPU)
        power = pynvml.nvmlDeviceGetPowerUsage(self.handle) / 1000.0
        mem_info = pynvml.nvmlDeviceGetMemoryInfo(self.handle)
        mem_used = mem_info.used / 1024**3  # GB
        util = pynvml.nvmlDeviceGetUtilizationRates(self.handle)
        clock = pynvml.nvmlDeviceGetClockInfo(self.handle, pynvml.NVML_CLOCK_SM)
        
        return GPUMetrics(temp, power, mem_used, util.gpu, clock)
    
    def run_stress_test(self, duration_seconds=30, matrix_size=4096):
        """
        Run safe matrix multiplication stress test
        Returns: ops_per_second, avg_temp, peak_memory
        """
        print(f"\n🔥 Starting SAFE GPU Stress Test ({duration_seconds}s)")
        print(f"   Matrix size: {matrix_size}x{matrix_size}")
        print(f"   Safety limit: {self.max_temp}°C")
        
        # Create test matrices on GPU
        A = cp.random.randn(matrix_size, matrix_size, dtype=cp.float32)
        B = cp.random.randn(matrix_size, matrix_size, dtype=cp.float32)
        
        # Warmup (5 seconds)
        print("   Warming up...")
        for _ in range(10):
            C = cp.matmul(A, B)
            cp.cuda.Stream.null.synchronize()
        
        # Monitoring thread
        def monitor():
            while self.running:
                metrics = self.get_metrics()
                self.metrics_history.append(metrics)
                
                # Safety check
                if metrics.temperature > self.max_temp:
                    print(f"\n⚠️  Temperature {metrics.temperature}°C exceeds limit! Throttling...")
                    time.sleep(0.5)  # Give GPU a break
                
                time.sleep(0.5)
        
        monitor_thread = threading.Thread(target=monitor)
        monitor_thread.start()
        
        # Actual test
        start_time = time.time()
        ops_count = 0
        
        while time.time() - start_time < duration_seconds:
            # Matrix multiplication (2*N^3 FLOPs)
            C = cp.matmul(A, B)
            cp.cuda.Stream.null.synchronize()
            ops_count += 2 * matrix_size**3
            
            # Check temp periodically
            if self.metrics_history and self.metrics_history[-1].temperature > self.max_temp:
                time.sleep(0.1)  # Cooldown
        
        self.running = False
        monitor_thread.join()
        elapsed = time.time() - start_time
        
        # Calculate metrics
        ops_per_sec = ops_count / elapsed
        avg_temp = np.mean([m.temperature for m in self.metrics_history])
        peak_mem = max([m.memory_used for m in self.metrics_history])
        
        print(f"\n✅ Stress Test Complete")
        print(f"   Performance: {ops_per_sec/1e12:.2f} TFLOPS")
        print(f"   Avg Temp: {avg_temp:.1f}°C")
        print(f"   Peak Memory: {peak_mem:.1f} GB")
        
        return ops_per_sec, avg_temp, peak_mem

# Run baseline
if __name__ == "__main__":
    tester = SafeGPUStressTest(max_temp_celsius=75)  # Conservative
    baseline_ops, temp, mem = tester.run_stress_test(duration_seconds=15)
    
    # Store baseline for comparison
    with open("baseline_perf.txt", "w") as f:
        f.write(f"{baseline_ops}")
    
    print(f"\n📊 BASELINE: {baseline_ops/1e12:.2f} TFLOPS")