
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>
#include <cstdint>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <string>
#include <windows.h>
#include <malloc.h>

using namespace std;

// Benchmark settings
constexpr size_t CACHE_LINE_SIZE = 64;
constexpr size_t STRIDE = CACHE_LINE_SIZE / sizeof(uint32_t);

constexpr size_t REGION_SIZE = 64 * 1024;
constexpr size_t NODES_PER_REGION =
    REGION_SIZE / CACHE_LINE_SIZE;

constexpr int TRIALS = 30;
constexpr uint64_t ITERATIONS = 600000;

// Logical processor 0 is a P-core
constexpr unsigned int TARGET_CPU = 0;

volatile uint32_t benchmark_sink = 0;

struct TestPoint {
    size_t bytes;
    string region;
};


// Add the working-set sizes to be tested
void add_range(
    vector<TestPoint>& points,
    size_t start,
    size_t end,
    size_t step,
    const string& region
) {
    for (size_t size = start; size <= end; size += step) {
        points.push_back({size, region});
    }
}


// Pin the benchmark to the selected P-core
bool pin_to_cpu(DWORD_PTR& previousMask) {

    DWORD_PTR processMask = 0;
    DWORD_PTR systemMask = 0;

    if (!GetProcessAffinityMask(
            GetCurrentProcess(),
            &processMask,
            &systemMask)) {
        return false;
    }

    if (TARGET_CPU >= sizeof(DWORD_PTR) * 8)
        return false;

    DWORD_PTR targetMask =
        (DWORD_PTR(1) << TARGET_CPU);

    if (!(processMask & targetMask))
        return false;

    previousMask = SetThreadAffinityMask(
        GetCurrentThread(),
        targetMask
    );

    return previousMask != 0;
}


// Calculate the average latency
double calculate_mean(const vector<double>& values) {

    return accumulate(
        values.begin(),
        values.end(),
        0.0
    ) / values.size();
}


// Find the middle value after sorting the results
double calculate_median(vector<double> values) {

    sort(values.begin(), values.end());

    size_t n = values.size();

    if (n % 2 == 0) {
        return (values[n / 2 - 1] + values[n / 2]) / 2.0;
    }

    return values[n / 2];
}


// Calculate the sample standard deviation
double calculate_stddev(
    const vector<double>& values,
    double mean
) {

    double sum = 0.0;

    for (double value : values) {
        sum += (value - mean) * (value - mean);
    }

    return sqrt(sum / (values.size() - 1));
}


// Run the benchmark for one working-set size
bool benchmark_size(
    size_t bytes,
    const string& region,
    LARGE_INTEGER frequency,
    ofstream& csv
) {

    size_t nodes = bytes / CACHE_LINE_SIZE;

    // Allocate memory on a 64 KB boundary
    uint32_t* memory = static_cast<uint32_t*>(
        _aligned_malloc(bytes, REGION_SIZE)
    );

    if (memory == nullptr) {
        cerr << "Memory allocation failed for "
             << bytes / 1024 << " KB\n";
        return false;
    }

    // Create and shuffle the node indices
    vector<uint32_t> order(nodes);

    iota(order.begin(), order.end(), 0);

    mt19937 generator(
        static_cast<unsigned int>(
            bytes ^ 0xA5A5A5A5
        )
    );

    // Shuffle the nodes within each 64 KB region
    for (size_t start = 0;
         start < nodes;
         start += NODES_PER_REGION) {

        size_t end = min(
            start + NODES_PER_REGION,
            nodes
        );

        shuffle(
            order.begin() + start,
            order.begin() + end,
            generator
        );
    }

    // Link each cache-line-sized node to the next one
    for (size_t i = 0; i < nodes; i++) {

        uint32_t current = order[i];

        uint32_t next =
            order[(i + 1) % nodes];

        memory[
            static_cast<size_t>(current) * STRIDE
        ] = next;
    }

    // Free the temporary array
    vector<uint32_t>().swap(order);

    // Run through the pointer chain before starting the timer
    uint32_t current = 0;

    for (size_t i = 0; i < nodes * 2; i++) {

        current = memory[
            static_cast<size_t>(current) * STRIDE
        ];
    }

    vector<double> times;
    times.reserve(TRIALS);

    // Perform the timing trials
    for (int trial = 0; trial < TRIALS; trial++) {

        current = 0;

        LARGE_INTEGER start, finish;

        QueryPerformanceCounter(&start);

        for (uint64_t i = 0; i < ITERATIONS; i++) {

            current = memory[
                static_cast<size_t>(current) * STRIDE
            ];
        }

        QueryPerformanceCounter(&finish);

        benchmark_sink = current;

        double elapsed_ns =
            (finish.QuadPart - start.QuadPart)
            * 1e9 / frequency.QuadPart;

        double latency =
            elapsed_ns / ITERATIONS;

        times.push_back(latency);
    }

    // Calculate the statistics for this working-set size
    double mean = calculate_mean(times);

    double median = calculate_median(times);

    double stddev = calculate_stddev(times, mean);

    // Save the individual trial results and statistics
    csv << region << ","
        << bytes << ","
        << fixed << setprecision(3)
        << bytes / 1024.0 << ","
        << nodes;

    for (double value : times) {
        csv << "," << value;
    }

    csv << "," << mean
        << "," << median
        << "," << stddev
        << "\n";

    // Display the results
    cout << left << setw(10) << region
         << right << setw(10) << fixed
         << setprecision(1)
         << bytes / 1024.0 << " KB"
         << " | Median: " << setw(8)
         << setprecision(3) << median << " ns"
         << " | Mean: " << setw(8) << mean << " ns"
         << " | StdDev: " << stddev << " ns\n";

    _aligned_free(memory);

    return true;
}


int main() {

    // Set the processor affinity
    DWORD_PTR previousMask = 0;

    if (!pin_to_cpu(previousMask)) {

        cerr << "Could not pin thread to selected P-core.\n";
        return 1;
    }

    LARGE_INTEGER frequency;

    if (!QueryPerformanceFrequency(&frequency)) {

        cerr << "High-resolution timer unavailable.\n";

        SetThreadAffinityMask(
            GetCurrentThread(),
            previousMask
        );

        return 1;
    }

    cout << "CACHE CAPACITY BENCHMARK\n";
    cout << "------------------------\n";

    cout << "Logical Processor: "
         << TARGET_CPU << "\n";

    cout << "Core Type: P-core\n";

    cout << "Cache Line Assumption: "
         << CACHE_LINE_SIZE << " bytes\n";

    cout << "Region Size: "
         << REGION_SIZE / 1024 << " KB\n";

    cout << "Trials: " << TRIALS << "\n";

    cout << "Iterations per trial: "
         << ITERATIONS << "\n\n";


    // Set up the working-set sizes for each cache range
    vector<TestPoint> points;

    // L1: 32 KB to 80 KB, in 2 KB steps
    add_range(
        points,
        32 * 1024,
        80 * 1024,
        2 * 1024,
        "L1-range"
    );

    // L2: 1 MB to 1.75 MB, in 32 KB steps
    add_range(
        points,
        1024 * 1024,
        1792 * 1024,
        32 * 1024,
        "L2-range"
    );

    // L3: 12 MB to 24 MB, in 512 KB steps
    add_range(
        points,
        12 * 1024 * 1024,
        24 * 1024 * 1024,
        512 * 1024,
        "L3-range"
    );


    // Create the CSV file
    ofstream csv(
        "cache_results.csv"
    );

    if (!csv.is_open()) {

        cerr << "Could not create CSV file.\n";

        SetThreadAffinityMask(
            GetCurrentThread(),
            previousMask
        );

        return 1;
    }

    csv << "Region,Working_Set_Bytes,"
        << "Working_Set_KB,Nodes";

    for (int i = 1; i <= TRIALS; i++) {
        csv << ",Trial_" << i << "_ns";
    }

    csv << ",Mean_ns,Median_ns,StdDev_ns\n";


    // Run the benchmark for all working-set sizes
    bool success = true;

    for (const auto& point : points) {

        if (!benchmark_size(
                point.bytes,
                point.region,
                frequency,
                csv)) {

            success = false;
            break;
        }
    }

    csv.close();

    // Restore the original processor affinity
    SetThreadAffinityMask(
        GetCurrentThread(),
        previousMask
    );

    cout << "\n------------------------\n";

    if (success) {

        cout << "Benchmark completed successfully.\n";

        cout << "Results saved to "
             << "cache_results.csv\n";

    } else {

        cout << "Benchmark ended with an error.\n";
    }

    return success ? 0 : 1;
}
