/** @file team11_VMCacheSim.cpp
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

#include <cstdio>
#include <cstdlib>
#include <iomanip> //for easier output formatting
#include <iostream>
#include <iterator>
#include <string>
#include <fstream>
#include <vector>

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
 * @brief an object to hold information about each page table
 */
struct PageTableEntry {
        bool valid;
        int physicalPage;
    };

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
 * @brief Universal method call to validate and check through the information in each page rather than have almost identical logic in each page check and the dist and src checks
 *  this function accesses a page, checks the bounds and if the page has been mapped or not
 *  then it increments the table hits or the page faults otherwise it allocates a new page
 * 
 */
void pageTableCheck (int proc, int virtualPage,
                vector<vector<PageTableEntry>> &pageTables,
                vector<int> &freePages,
                int &totalPageTableHits,
                int &totalPagesFromFree,
                int &totalPageFaults,
                int &virtualPagesMapped,
                int usedEntries[]){
    //prevents out of bounds to send it to the next page
    if(virtualPage >= PAGE_TABLE_ENTRIES)
        return;
    //checks if the page has already been mapped
    if(pageTables[proc][virtualPage].valid)
        totalPageTableHits++;
    //else it allocates a new page
    else{
        virtualPagesMapped++;
        //checks to see if there is space
        if(!freePages.empty()){
            //removes a freePage
            int physicalPage = freePages.back();
            freePages.pop_back();

            //updates the pageTable to add the new page
            pageTables[proc][virtualPage].valid = true;
            pageTables[proc][virtualPage].physicalPage = physicalPage;

            usedEntries[proc]++;

            totalPagesFromFree++;
        }
        //adds a page fault for when freepages is empty
        else{
            totalPageFaults++;
        }
    }
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

    ios::sync_with_stdio(false);
    cin.tie(nullptr);
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

    /** @brief Total pages available to the user */
    int totalPagesForUser = 0;

    /** @brief Loop variable for parsing arguments */
    int i = 0;

    /** @brief Total times address is already mapped*/
    int totalPageTableHits = 0;

    /** @brief Total times address mapped to a page not in use */
    int totalPagesFromFree = 0;

    /** @brief Total times where no physical page is available */
    int totalPageFaults = 0;

    /**
     * ---------------------------------------------------------
     * Virtual Memory Variables
     * ---------------------------------------------------------
     */

    

    vector<int> freePages;

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

    totalPagesForUser = numberOfPhysicalPages - numberOfPagesForSystem;
    /**
     * ---------------------------------------------------------
     * Virtual Memory Calculations
     * ---------------------------------------------------------
     */

     //creates a page table entry for each of PAGE_TABLE_ENTRIES
    vector<vector<PageTableEntry>> pageTables(traceCount,vector<PageTableEntry>(PAGE_TABLE_ENTRIES));
    //creates a list of pages available to each program
    for(i = numberOfPagesForSystem; i< numberOfPhysicalPages; i++){
        freePages.push_back(i);
    }


/**
 * ---------------------------------------------------------
 * Read Trace Files - WIP
 * ---------------------------------------------------------
 */
    int usedEntries[3] = {0};
    int virtualPagesMapped = 0;
    string traceLine;
    //loops for each trace file
    for (int i = 0; i < traceCount; i++) {
        std::cout<< traceFiles[i] << endl;
        //checks to see if the file was grabbed correctly
        ifstream traceFile(traceFiles[i]);
        if(!traceFile){
            cerr << "error" << ": " << traceFiles[i] << endl;
            return 1;
        }
        // loop to read each line from a trace file
        while (getline(traceFile, traceLine)) {
            //handles EIP  lines
            if(traceLine[0] == 'E'){
                int length;
                unsigned int addr;
                //reads a line from the tracefiles and allocatess tghe information to the length and addr variables
                //this if statement is also used to limit each instruction to 2 pages max per instruction to increase efficiency
                if(sscanf(traceLine.c_str(), "EIP (%d): %x", &length, &addr) == 2){

                    int firstPage = addr / PAGE_SIZE;
                    int lastPage  = (addr + length - 1) / PAGE_SIZE;
                    //default page check for every instruction
                    pageTableCheck(i, firstPage, pageTables, freePages,
                            totalPageTableHits, totalPagesFromFree,
                            totalPageFaults, virtualPagesMapped,
                            usedEntries);

                    // check second page if instruction crosses page boundary
                    if(firstPage != lastPage){
                        pageTableCheck(i, lastPage, pageTables, freePages,
                                totalPageTableHits, totalPagesFromFree,
                                totalPageFaults, virtualPagesMapped,
                                usedEntries);
                    }                        
                }
            }
            //handles dstM and secM lines
            else if(traceLine[0] == 'd'){
                unsigned int dstAddr, srcAddr;
                //grabs the information for dstM and srcM
                sscanf(traceLine.c_str(), "dstM: %x %*s srcM: %x %*s", &dstAddr, &srcAddr);
                //handles the first part of the secondary line (distM)
                if(dstAddr != 0){
                    int virtualPage = dstAddr / PAGE_SIZE;

                    pageTableCheck(i, virtualPage, pageTables, freePages,
                            totalPageTableHits, totalPagesFromFree,
                            totalPageFaults, virtualPagesMapped,
                            usedEntries);
                }
                //handles the second part of the secondary lines(srcM)
                if(srcAddr != 0){
                    int virtualPage = srcAddr / PAGE_SIZE;

                    pageTableCheck(i, virtualPage, pageTables, freePages,
                            totalPageTableHits, totalPagesFromFree,
                            totalPageFaults, virtualPagesMapped,
                            usedEntries);
                }
            }
        }
        traceFile.close();
    }

    /**
     * Track Pages wasted by looping once for each trace file and calculating unused entries 
     *   by subtracting usedentries from the total
     */

    int wastedBytes[3] = {0};

    for(int i = 0; i < traceCount; i++){
        int unusedEntries = PAGE_TABLE_ENTRIES - usedEntries[i];
        wastedBytes[i] = (unusedEntries * pageTableEntryBits)/8;
    }
    
/**
 * ---------------------------------------------------------
 * Print required output
 * ---------------------------------------------------------
 */

    std::cout<< "MILESTONE #1:  Input Parameters and Calculated Values\n";
    std::cout<< "Cache Simulator - CS 3853 - Team #11\n\n";

    std::cout<< "Trace File(s):\n";
    for (i = 0; i < traceCount; i++) {
        std::cout<< "        " << traceFiles[i] << '\n';
    }
    std::cout<< '\n';

    std::cout<< "***** Cache Input Parameters *****\n\n";
    std::cout<< left << setw(32) << "Cache Size:" << cacheSizeKB << " KB\n";
    std::cout<< left << setw(32) << "Block Size:" << blockSize << " bytes\n";
    std::cout<< left << setw(32) << "Associativity:" << associativity << '\n';
    std::cout<< left << setw(32) << "Replacement Policy:" << replacementPolicyPretty << '\n';
    std::cout<< left << setw(32) << "Physical Memory:" << physicalMemoryMB << " MB\n";
    std::cout<< left << setw(32) << "Percent Memory Used by System:"
        << fixed << setprecision(1) << static_cast<double>(percentUsedByOS) << "%\n";

    std::cout<< left << setw(32) << "Instructions / Time Slice:";
    if (instructionsPerTimeSlice == -1) {
        std::cout<< "max\n";
    }
    else {
        std::cout<< instructionsPerTimeSlice << '\n';
    }

    std::cout<< "\n***** Cache Calculated Values *****\n\n";
    std::cout<< left << setw(32) << "Total # Blocks:" << totalBlocks << '\n';

    /* Tag size is based on actual physical memory */
    std::cout<< left << setw(32) << "Tag Size:" << tagBits << " bits\n";

    std::cout<< left << setw(32) << "Index Size:" << indexBits << " bits\n";
    std::cout<< left << setw(32) << "Total # Rows:" << totalRows << '\n';
    std::cout<< left << setw(32) << "Overhead Size:" << overheadSizeBytes << " bytes\n";
    std::cout<< left << setw(32) << "Implementation Memory Size:"
        << fixed << setprecision(2) << implementationMemorySizeKB
        << " KB  (" << implementationMemorySizeBytes << " bytes)\n";
    std::cout<< left << setw(32) << "Cost:"
        << "$" << fixed << setprecision(2) << cost << " @ $0.07 per KB\n";

    std::cout<< "\n***** Physical Memory Calculated Values *****\n\n";
    std::cout<< left << setw(32) << "Number of Physical Pages:" << numberOfPhysicalPages << '\n';

    /* numberOfPagesForSystem = (percentUsedByOS / 100.0) * numberOfPhysicalPages */
    std::cout<< left << setw(32) << "Number of Pages for System:"
        << numberOfPagesForSystem << '\n';

    /* pageTableEntryBits = 1 valid bit + physicalPageBits */
    std::cout<< left << setw(32) << "Size of Page Table Entry:"
        << pageTableEntryBits << " bits\n";

    /* totalRAMForPageTables = 512K entries * traceCount * pageTableEntryBits / 8 */
    std::cout<< left << setw(32) << "Total RAM for Page Table(s):"
        << totalRAMForPageTables << " bytes\n";

    std::cout<< left << setw(32) << "\n***** VIRTUAL MEMORY SIMULATION RESULTS *****\n" << endl;
    std::cout<< left << setw(32) << "Physical Pages Used By SYSTEM: " << numberOfPagesForSystem << endl;
    std::cout<< left << setw(32) << "Pages Available to User: " << totalPagesForUser << endl ;
    std::cout<< left << setw(32) << "Virtual Pages Mapped: " << virtualPagesMapped << endl;
    std::cout<< "\t------------------------------" << endl;
    std::cout<< "\tPage Table Hits: " << totalPageTableHits << endl << endl;
    std::cout<< "\tPages from Free: " << totalPagesFromFree << endl << endl;
    std::cout<< "\tTotal Page Faults: " << totalPageFaults << endl << endl;
    std::cout<< "Page Table Usage Per Process:" << endl;
    std::cout<< "------------------------------" << endl;
    for (int i = 0; i < traceCount; i++) {
        if (!traceFiles[i].empty()) {
            std::cout<< "[" << i << "] " << traceFiles[i] << ": " << endl;
            std::cout<< "\tUsed Page Table Entries: " << usedEntries[i] << endl;
            std::cout<< "\tPage Table Wasted: " << wastedBytes[i] << endl;
        }
    }
    return 0;
}
