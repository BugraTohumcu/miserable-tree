#include "../src/persistence/BookRepository.h"
#include <iostream>
#include <chrono>

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

    // 1. Capture baseline memory before index construction
    double memStart = getMemoryUsageMB();
    auto start = std::chrono::high_resolution_clock::now();

    // 2. Boot repository (Triggers full file parse & B+ Tree constructions)
    mislib::BookRepo repo(dataPath);

    // 3. Capture benchmarks immediately after build completes
    auto end = std::chrono::high_resolution_clock::now();
    auto buildMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double memEnd = getMemoryUsageMB();

    // 4. Output results
    std::cout << "[BOOT] Build Time       : " << buildMs << " ms\n";
    std::cout << "[RAM]  Net Memory Cost  : " << (memEnd - memStart) << " MB\n";
    std::cout << "==================================================\n";

    return 0;
}