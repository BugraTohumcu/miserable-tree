#include "../src/persistence/BookRepository.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

/**
 * @brief Measures current physical RAM usage in MB.
 */
double getMemoryUsageMB() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0);
    }
#endif
    return 0.0;
}

int main() {
    // Path to the dataset in the root directory
    const std::string dataPath = "data/books_dataset.txt"; 
    
    std::cout << "==================================================\n";
    std::cout << "        MISLIB SCALE PERFORMANCE BENCHMARK        \n";
    std::cout << "==================================================\n";

    double memStart = getMemoryUsageMB();
    auto start = std::chrono::high_resolution_clock::now();
    
    // Booting repository (Triggers O(N log N) indexing)
    mislib::BookRepo repo(dataPath); 
    
    auto end = std::chrono::high_resolution_clock::now();
    auto buildMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double memEnd = getMemoryUsageMB();

    std::cout << "[BOOT] Build Time       : " << buildMs << " ms\n";
    std::cout << "[RAM]  Net Memory Cost  : " << (memEnd - memStart) << " MB\n";
    std::cout << "--------------------------------------------------\n";

    size_t lastId = repo.getLastId();
    std::mt19937 gen(1337); // Fixed seed for consistent results
    std::uniform_int_distribution<size_t> dis(1, lastId);
    mislib::Book dummy;
    size_t hits = 0;

    std::cout << "[SEARCH] Executing 1,000 random lookups...\n";
    auto sStart = std::chrono::high_resolution_clock::now();
    
    for(int i = 0; i < 1000; ++i) {
        if(repo.get(dis(gen), dummy)) {
            hits++;
        }
    }
    
    auto sEnd = std::chrono::high_resolution_clock::now();
    auto totalSearchUs = std::chrono::duration_cast<std::chrono::microseconds>(sEnd - sStart).count();

    std::cout << "[RESULT] Avg Latency    : " << (totalSearchUs / 1000.0) << " us\n";
    std::cout << "[RESULT] Hit Rate       : " << hits << " / 1000\n";
    std::cout << "==================================================\n";
    
    return 0;
}