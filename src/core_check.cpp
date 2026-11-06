#include <iostream>
#include <iomanip>
#include <windows.h>
#include <cpuid.h>

int main() {

    DWORD_PTR processMask = 0;
    DWORD_PTR systemMask = 0;

    HANDLE thread = GetCurrentThread();

    if (!GetProcessAffinityMask(
            GetCurrentProcess(),
            &processMask,
            &systemMask)) {

        std::cerr << "Unable to get processor information.\n";
        return 1;
    }

    DWORD_PTR originalMask = 0;
    bool savedMask = false;

    std::cout << "CPU Core Identification\n";
    std::cout << "-----------------------\n";

    for (unsigned int i = 0;
         i < sizeof(DWORD_PTR) * 8;
         i++) {

        DWORD_PTR mask = (DWORD_PTR(1) << i);

        if (!(processMask & mask))
            continue;

        // Run the thread on one logical processor at a time
        DWORD_PTR previousMask =
            SetThreadAffinityMask(thread, mask);

        if (previousMask == 0) {
            std::cout << "Could not access CPU " << i << "\n";
            continue;
        }

        // Save the original affinity so it can be restored later
        if (!savedMask) {
            originalMask = previousMask;
            savedMask = true;
        }

        Sleep(5);

        unsigned int eax, ebx, ecx, edx;

        std::cout << "Logical Processor "
                  << std::setw(2) << i << " : ";

        if (__get_cpuid_count(
                0x1A, 0, &eax, &ebx, &ecx, &edx)) {

            unsigned int type = (eax >> 24) & 0xFF;

            if (type == 0x40)
                std::cout << "P-core\n";

            else if (type == 0x20)
                std::cout << "E-core\n";

            else
                std::cout << "Unknown core type: 0x"
                          << std::hex << type << std::dec << "\n";

        } else {
            std::cout << "CPUID information unavailable\n";
        }
    }

    // Restore the original processor affinity
    if (savedMask)
        SetThreadAffinityMask(thread, originalMask);

    return 0;
}
