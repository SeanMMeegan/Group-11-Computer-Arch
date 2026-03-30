/**
* @file team11_VMCacheSim.cpp
* @author Group 11 - Meegan, Sean M. | Guerra, Sage | Hipolito, Kristian | Teschan, Addison K.
 * @brief CS 3853 Cache Simulator - Milestone #1
 *
 * @details
 * This program reads command-line arguments for a configurable cache and
 * virtual memory simulator, computes the required Milestone #1 values,
 * and prints them in the required format.
 *
 * Milestone #1 only requires:
 * - input parameters
 * - calculated cache values
 * - calculated physical memory values
 *
 * This program does NOT yet simulate actual trace execution.
 *
 * @section assumptions Project Assumptions
 * - 32-bit data bus
 * - 32-bit virtual address space
 * - Virtual address space = 4 GB
 * - Physical memory is configurable from 128 MB to 4096 MB in powers of 2
 * - Cache size is configurable from 8 KB to 8192 KB in powers of 2
 * - Block size may be 8, 16, 32, or 64 bytes
 * - Associativity may be 1, 2, 4, 8, or 16
 * - Replacement policy may be RR (Round Robin) or RND (Random)
 * - Up to 3 trace files may be provided
 * - Each trace file represents a different process
 *
 * @section parameters Command-Line Parameters
 * - `-s <cache size KB>` : cache size in KB
 * - `-b <block size>` : block size in bytes
 * - `-a <associativity>` : associativity value
 * - `-r <replacement policy>` : RR or RND
 * - `-p <physical memory MB>` : physical memory size in MB
 * - `-u <percent physical mem used by OS>` : 0 to 100
 * - `-n <instructions / time slice>` : positive integer, or -1 for max
 * - `-f <trace file name>` : trace file input
 *
 * @section example Example Run
 * @code
 * ./VMCacheSim -s 512 -b 16 -a 4 -r rr -p 1024 -u 75 -n 100 \
 * -f Trace1.trc -f Trace2_4Evaluation.trc -f Corruption1.trc
 * @endcode
 *
 * @section Notes
 * - For Milestone #1, the program focuses on parsing input and computing
 *   derived values only.
 * - Cache write policy is assumed irrelevant for this milestone.
 * - The program should accept 1, 2, or 3 trace files. guess we use these later
 */

#include <cstdlib>
#include <iomanip> //for easier output formatting
#include <iostream>
#include <string>

using namespace std;

/**
 * @brief Cost per KB for implementation memory.
 */
const double COST_PER_KB = 0.07;

/**
 * @brief Page size in bytes.
 *
 * The assignment implies 4 KB pages.
 */
const int PAGE_SIZE = 4096;

/**
 * @brief Number of page table entries per process.
 *
 * The assignment states page tables can be 512K entries.
 * 512K = 524288
 */
const int PAGE_TABLE_ENTRIES = 524288;

/**
 * @brief Checks whether a number is a power of 2.
 *
 * @param value The integer to test.
 * @return true if value is a power of 2, false otherwise.
 */
bool isPowerOfTwo(int value) {
    if (value <= 0) {
        return false;
    }

    return (value & (value - 1)) == 0;
}

/**
 * @brief Computes integer log base 2 for a power-of-two value.
 *
 * Example:
 * - intLog2(8) = 3
 * - intLog2(16) = 4
 * - intLog2(1024) = 10
 *
 * @param value A positive power-of-two integer.
 * @return log base 2 of value.
 */
int intLog2(int value) {
    int count = 0;

    while (value > 1) {
        value = value / 2;
        count++;
    }

    return count;
}

/**
 * @brief Prints correct command-line usage and exits.
 */
void printUsageAndExit() {
    cerr << "Usage:\n";
    cerr << "  ./VMCacheSim -s <cache size KB> -b <block size> -a <associativity>\n";
    cerr << "               -r <rr|rnd> -p <physical memory MB> -u <percent used by OS>\n";
    cerr << "               -n <instructions per time slice> -f <trace1> [-f <trace2>] [-f <trace3>]\n";
    exit(1);
}

/**
 * @brief Main driver for Milestone #1.
 *
 * This program:
 * 1. Reads command-line arguments
 * 2. Validates the arguments
 * 3. Computes the required cache values
 * 4. Computes the required physical memory values
 * 5. Prints all results in the required format
 */
int main(int argc, char *argv[]) {
    /**
     * ---------------------------------------------------------
     * Input variables
     * ---------------------------------------------------------
     */

    /** @brief Cache size in KB from -s */
    int cacheSizeKB = 0;

    /** @brief Block size in bytes from -b */
    int blockSize = 0;

    /** @brief Associativity from -a */
    int associativity = 0;

    /** @brief Raw replacement policy input, such as "rr" or "rnd" */
    string replacementPolicyRaw = "";

    /** @brief Pretty replacement policy string for output */
    string replacementPolicyPretty = "";

    /** @brief Physical memory size in MB from -p */
    int physicalMemoryMB = 0;

    /** @brief Percent of physical memory used by the OS from -u */
    int percentUsedByOS = 0;

    /** @brief Instructions per time slice from -n */
    int instructionsPerTimeSlice = 0;

    /**
     * @brief Stores up to 3 trace file names.
     *
     * Needs 1, 2, or 3 trace files.
     */
    string traceFiles[3];

    /** @brief Number of trace files actually given */
    int traceCount = 0;

    /**
     * ---------------------------------------------------------
     * Variables for calculations
     * ---------------------------------------------------------
     */

    /** @brief Cache size converted to bytes */
    int cacheSizeBytes = 0;

    /** @brief Physical memory converted to bytes */
    int physicalMemoryBytes = 0;

    /** @brief Total number of cache blocks */
    int totalBlocks = 0;

    /** @brief Total number of rows/sets in the cache */
    int totalRows = 0;

    /** @brief Number of block offset bits */
    int offsetBits = 0;

    /** @brief Number of index bits */
    int indexBits = 0;

    /** @brief Number of physical address bits */
    int physicalAddressBits = 0;

    /** @brief Number of tag bits */
    int tagBits = 0;

    /** @brief Number of overhead bits per cache block */
    int overheadBitsPerBlock = 0;

    /** @brief Total overhead size in bytes */
    int overheadSizeBytes = 0;

    /** @brief Total implementation memory size in bytes */
    int implementationMemorySizeBytes = 0;

    /** @brief Total implementation memory size in KB */
    double implementationMemorySizeKB = 0.0;

    /** @brief Cost of implementation memory */
    double cost = 0.0;

    /** @brief Number of physical pages */
    int numberOfPhysicalPages = 0;

    /** @brief Number of physical pages reserved for the OS */
    int numberOfPagesForSystem = 0;

    /** @brief Number of bits needed for the physical page number */
    int physicalPageBits = 0;

    /** @brief Number of bits in one page table entry */
    int pageTableEntryBits = 0;

    /** @brief Total RAM used by all page tables in bytes */
    int totalRAMForPageTables = 0;

    /** @brief Loop variable for parsing arguments */
    int i = 0;

    /**
     * ---------------------------------------------------------
     * Basic argument count check
     * ---------------------------------------------------------
     */

    if (argc < 3) {
        printUsageAndExit();
    }

    /**
     * ---------------------------------------------------------
     * Parse command-line arguments
     * ---------------------------------------------------------
     */

    for (i = 1; i < argc; i++) {
        string option = argv[i];

        if (option == "-s") {
            if (i + 1 >= argc) {
                printUsageAndExit();
            }
            cacheSizeKB = atoi(argv[++i]);
        }
        else if (option == "-b") {
            if (i + 1 >= argc) {
                printUsageAndExit();
            }
            blockSize = atoi(argv[++i]);
        }
        else if (option == "-a") {
            if (i + 1 >= argc) {
                printUsageAndExit();
            }
            associativity = atoi(argv[++i]);
        }
        else if (option == "-r") {
            if (i + 1 >= argc) {
                printUsageAndExit();
            }

            replacementPolicyRaw = argv[++i];

            if (replacementPolicyRaw == "rr" || replacementPolicyRaw == "RR") {
                replacementPolicyPretty = "Round Robin";
            }
            else if (replacementPolicyRaw == "rnd" || replacementPolicyRaw == "RND") {
                replacementPolicyPretty = "Random";
            }
            else {
                cerr << "Error: replacement policy must be rr or rnd.\n";
                return 1;
            }
        }
        else if (option == "-p") {
            if (i + 1 >= argc) {
                printUsageAndExit();
            }
            physicalMemoryMB = atoi(argv[++i]);
        }
        else if (option == "-u") {
            if (i + 1 >= argc) {
                printUsageAndExit();
            }
            percentUsedByOS = atoi(argv[++i]);
        }
        else if (option == "-n") {
            if (i + 1 >= argc) {
                printUsageAndExit();
            }
            instructionsPerTimeSlice = atoi(argv[++i]);
        }
        else if (option == "-f") {
            if (i + 1 >= argc) {
                printUsageAndExit();
            }

            if (traceCount >= 3) {
                cerr << "Error: at most 3 trace files are allowed.\n";
                return 1;
            }

            traceFiles[traceCount] = argv[++i];
            traceCount++;
        }
        else {
            cerr << "Error: unknown option: " << option << endl;
            printUsageAndExit();
        }
    }

    /**
     * ---------------------------------------------------------
     * Validate all input values
     * ---------------------------------------------------------
     */

    if (!(cacheSizeKB >= 8 && cacheSizeKB <= 8192 && isPowerOfTwo(cacheSizeKB))) {
        cerr << "Error: cache size must be 8 to 8192 KB in powers of 2.\n";
        return 1;
    }

    if (!(blockSize == 8 || blockSize == 16 || blockSize == 32 || blockSize == 64)) {
        cerr << "Error: block size must be 8, 16, 32, or 64 bytes.\n";
        return 1;
    }

    if (!(associativity == 1 || associativity == 2 || associativity == 4 ||
          associativity == 8 || associativity == 16)) {
        cerr << "Error: associativity must be 1, 2, 4, 8, or 16.\n";
        return 1;
    }

    if (!(physicalMemoryMB >= 128 && physicalMemoryMB <= 4096 &&
          isPowerOfTwo(physicalMemoryMB))) {
        cerr << "Error: physical memory must be 128 to 4096 MB in powers of 2.\n";
        return 1;
    }

    if (percentUsedByOS < 0 || percentUsedByOS > 100) {
        cerr << "Error: percent memory used by system must be between 0 and 100.\n";
        return 1;
    }

    if (instructionsPerTimeSlice == 0 || instructionsPerTimeSlice < -1) {
        cerr << "Error: instructions per time slice must be -1 or a positive integer.\n";
        return 1;
    }

    if (traceCount < 1 || traceCount > 3) {
        cerr << "Error: you must provide 1 to 3 trace files.\n";
        return 1;
    }

    /**
     * ---------------------------------------------------------
     * Convert KB and MB inputs into bytes
     * ---------------------------------------------------------
     */

    cacheSizeBytes = cacheSizeKB * 1024;
    physicalMemoryBytes = physicalMemoryMB * 1024 * 1024;

    /**
     * ---------------------------------------------------------
     * Cache calculations
     * ---------------------------------------------------------
     */

    /**
     * Total blocks = cache size in bytes / block size
     */
    totalBlocks = cacheSizeBytes / blockSize;

    /**
     * Total rows = total blocks / associativity
     */
    totalRows = totalBlocks / associativity;

    /**
     * Offset bits = log2(block size)
     */
    offsetBits = intLog2(blockSize);

    /**
     * Index bits = log2(total rows)
     */
    indexBits = intLog2(totalRows);

    /**
     * Physical address bits = log2(physical memory in bytes)
     */
    physicalAddressBits = intLog2(physicalMemoryBytes);

    /**
     * Tag bits = physical address bits - index bits - offset bits
     */
    tagBits = physicalAddressBits - indexBits - offsetBits;

    /**
     * Overhead per block = 1 valid bit + tag bits
     */
    overheadBitsPerBlock = tagBits + 1;

    /**
     * Convert total overhead from bits to bytes
     */
    overheadSizeBytes = (totalBlocks * overheadBitsPerBlock) / 8;

    /**
     * Implementation memory size = cache data + overhead
     */
    implementationMemorySizeBytes = cacheSizeBytes + overheadSizeBytes;
    implementationMemorySizeKB = implementationMemorySizeBytes / 1024.0;

    /**
     * Cost is based on KB
     */
    cost = implementationMemorySizeKB * COST_PER_KB;

    /**
     * ---------------------------------------------------------
     * Physical memory calculations
     * ---------------------------------------------------------
     */

    /**
     * Number of physical pages = physical memory bytes / page size
     */
    numberOfPhysicalPages = physicalMemoryBytes / PAGE_SIZE;

    /**
     * Number of pages used by the system
     */
    numberOfPagesForSystem =
        static_cast<int>(numberOfPhysicalPages * (percentUsedByOS / 100.0));

    /**
     * Physical page bits = log2(number of physical pages)
     */
    physicalPageBits = intLog2(numberOfPhysicalPages);

    /**
     * Page table entry bits = 1 valid bit + physical page bits
     */
    pageTableEntryBits = 1 + physicalPageBits;

    /**
     * Total RAM for all page tables:
     * 512K entries * number of trace files * bits per entry / 8
     */
    totalRAMForPageTables =
        (PAGE_TABLE_ENTRIES * traceCount * pageTableEntryBits) / 8;

    /**
     * ---------------------------------------------------------
     * Print required output
     * ---------------------------------------------------------
     */

    cout << "MILESTONE #1:  Input Parameters and Calculated Values\n";
    cout << "Cache Simulator - CS 3853 - Team 11\n\n";

    cout << "Trace File(s):\n";
    for (i = 0; i < traceCount; i++) {
        cout << "        " << traceFiles[i] << '\n';
    }
    cout << '\n';

    cout << "***** Cache Input Parameters *****\n\n";
    cout << left << setw(32) << "Cache Size:" << cacheSizeKB << " KB\n";
    cout << left << setw(32) << "Block Size:" << blockSize << " bytes\n";
    cout << left << setw(32) << "Associativity:" << associativity << '\n';
    cout << left << setw(32) << "Replacement Policy:" << replacementPolicyPretty << '\n';
    cout << left << setw(32) << "Physical Memory:" << physicalMemoryMB << " MB\n";
    cout << left << setw(32) << "Percent Memory Used by System:"
         << fixed << setprecision(1) << static_cast<double>(percentUsedByOS) << "%\n";

    cout << left << setw(32) << "Instructions / Time Slice:";
    if (instructionsPerTimeSlice == -1) {
        cout << "max\n";
    }
    else {
        cout << instructionsPerTimeSlice << '\n';
    }

    cout << "\n***** Cache Calculated Values *****\n\n";
    cout << left << setw(32) << "Total # Blocks:" << totalBlocks << '\n';
    cout << left << setw(32) << "Tag Size:" << tagBits
         << " bits        (based on actual physical memory)\n";
    cout << left << setw(32) << "Index Size:" << indexBits << " bits\n";
    cout << left << setw(32) << "Total # Rows:" << totalRows << '\n';
    cout << left << setw(32) << "Overhead Size:" << overheadSizeBytes << " bytes\n";
    cout << left << setw(32) << "Implementation Memory Size:"
         << fixed << setprecision(2) << implementationMemorySizeKB
         << " KB  (" << implementationMemorySizeBytes << " bytes)\n";
    cout << left << setw(32) << "Cost:"
         << "$" << fixed << setprecision(2) << cost << " @ $0.07 per KB\n";

    cout << "\n***** Physical Memory Calculated Values *****\n\n";
    cout << left << setw(32) << "Number of Physical Pages:" << numberOfPhysicalPages << '\n';
    cout << left << setw(32) << "Number of Pages for System:"
         << numberOfPagesForSystem
         << "         ( " << fixed << setprecision(2)
         << (percentUsedByOS / 100.0)
         << " * " << numberOfPhysicalPages
         << " = " << numberOfPagesForSystem << " )\n";
    cout << left << setw(32) << "Size of Page Table Entry:"
         << pageTableEntryBits
         << " bits        (1 valid bit, " << physicalPageBits << " for PhysPage)\n";
    cout << left << setw(32) << "Total RAM for Page Table(s):"
         << totalRAMForPageTables
         << " bytes  (512K entries * " << traceCount
         << " .trc files * " << pageTableEntryBits << " / 8)\n";

    return 0;
}