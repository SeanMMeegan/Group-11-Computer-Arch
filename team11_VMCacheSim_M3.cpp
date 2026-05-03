/** @file team11_VMCacheSim_M3.cpp
 * @author Group 11 - Meegan, Sean M. | Guerra, Sage | Hipolito, Kristian | Teschan, Addison K.
 * @brief CS 3853 Virtual Memory and Cache Simulator - Milestone #3
 *
 * @details
 * This program reads command-line arguments for a configurable virtual memory
 * and cache simulator. It computes the required Milestone #1 calculated values,
 * performs the Milestone #2 virtual memory simulation, and performs the
 * Milestone #3 cache simulation.
 *
 * The simulator processes instruction fetches and valid source/destination
 * memory references from up to three trace files. Each virtual address is first
 * translated through the process page table, then the resulting physical address
 * is used to simulate cache behavior.
 *
 * The cache simulation tracks cache accesses, instruction bytes, source/
 * destination bytes, hits, misses, compulsory misses, conflict misses, hit rate,
 * miss rate, CPI, unused cache space, and unused cache blocks.
 *
 * @section functionality Milestone #3 Functionality
 * - Parse command-line arguments
 * - Compute cache and physical memory values (Milestone #1)
 * - Initialize per-process page tables (512K entries each)
 * - Simulate virtual-to-physical page mapping (Milestone #2)
 * - Simulate cache accesses using physical addresses (Milestone #3)
 * - Support round-robin or random replacement policy
 * - Track:
 *   * Total cache accesses
 *   * Instruction bytes
 *   * Source/destination bytes
 *   * Cache hits and misses
 *   * Compulsory and conflict misses
 *   * Total cycle count
 *   * CPI
 *   * Unused cache blocks and unused cache space
 *
 * @section assumptions Project Assumptions
 * - 32-bit data bus
 * - 32-bit virtual address space (4 GB)
 * - Only lower 31 bits are used by the trace files
 * - Page size = 4 KB
 * - Page table size = 512K entries per process
 * - Physical memory is configurable from 128 MB to 4096 MB
 * - Cache size is configurable from 8 KB to 8192 KB
 * - Block size may be 8, 16, 32, or 64 bytes
 * - Associativity may be 1, 2, 4, 8, or 16
 * - Replacement policy may be RR or RND
 * - Up to 3 trace files may be provided
 * - Each trace file represents a separate process
 * - EIP instruction fetches use the length listed in the trace
 * - Valid dstM/srcM references are treated as 4-byte accesses
 *
 * @section parameters Command-Line Parameters
 * - `-s <cache size KB>` : cache size in KB
 * - `-b <block size>` : block size in bytes
 * - `-a <associativity>` : associativity value
 * - `-r <replacement policy>` : RR or RND
 * - `-p <physical memory MB>` : physical memory size in MB
 * - `-u <percent physical mem used by OS>` : 0 to 100
 * - `-n <instructions / time slice>` : positive integer, or -1 for max
 * - `-f <trace file name>` : trace file input (1 to 3 allowed)
 *
 * @section example Example Run
 * @code
 * ./team11_VMCacheSim_M3 -s 512 -b 16 -a 4 -r rr -p 1024 -u 75 -n 100 \
 * -f Trace1.trc -f Trace2_4Evaluation.trc -f Corruption1.trc
 * @endcode
 *
 * @section notes Notes
 * - Trace files are processed line by line.
 * - Each EIP access is counted as an instruction fetch.
 * - Each valid dstM/srcM access is counted as a data access.
 * - A memory reference may touch multiple cache blocks.
 * - Cache hits cost 1 cycle per cache block accessed.
 * - Cache misses cost 4 * ceil(block size / 4) cycles per cache block accessed.
 * - Instruction fetches add 2 execution cycles.
 * - Data accesses add 1 effective-address calculation cycle.
 * - Page faults add 100 cycles.
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
 *-------------------------------------------------------------------------------
 * Constants
 *-------------------------------------------------------------------------------
 */

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
 *--------------------------------------------------------------------------------
 * Structs
 *--------------------------------------------------------------------------------
 */

struct PageTableEntry {
    bool valid = false;
    unsigned int phyPageNumber = 0;
};

struct ProcessInfo {
    string traceFileName;
    PageTableEntry pageTable[PAGE_TABLE_ENTRIES];
    unsigned int usedPageTableEntries = 0;
};

// Global process array
// ***This is here because it grows fast and will OVERFLOW the stack***
ProcessInfo processes[3];

/**
 *--------------------------------------------------------------------------------
 * M3 Cache Structss
 *--------------------------------------------------------------------------------
 */

// One block in the cache
struct CacheBlock {
    bool isValid = false;          // does this block have valid data?
    unsigned int tag = 0;          // tag used to identify the block
};

// One set in the cache (a group of blocks)
struct CacheSet {
    vector<CacheBlock> blocks;     // blocks inside this set

    // used for round-robin replacement
    int nextBlockToReplace = 0;
};

// Keeps track of all cache stats
struct CacheStats {
    int totalAddresses = 0;        // number of addresses from trace

    int totalCacheAccesses = 0;    // number of cache block accesses

    int instructionBytes = 0;      // total instruction bytes
    int dataBytes = 0;             // total src/dst bytes

    int hits = 0;
    int misses = 0;

    int compulsoryMisses = 0;      // first time loading block
    int conflictMisses = 0;        // replaced block in same set

    int totalInstructions = 0;     // number of instucions
    long long totalCycles = 0;     // total cycles used, long long incase of overflow, seems to be getting pretty big

    double hitRate = 0.0;

};


/*--------------------------------------------------------------------------------
 * Functions
 *--------------------------------------------------------------------------------
 */

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
 *@brief Convert hex to unsigned int
 */
unsigned int hexToUInt(const string &hexStr) {
    unsigned int value = 0;
    sscanf(hexStr.c_str(), "%x", &value);
    return value;
}

/**
 * helper for address processing
 */
bool processAddress(unsigned int addr,
                    ProcessInfo &process,
                    vector<unsigned int> &freePages,
                    int &virtualPagesMapped,
                    int &totalPageTableHits,
                    int &totalPagesFromFree,
                    int &totalPageFaults) {
    unsigned int virtualPage = addr / PAGE_SIZE;

    // count every valid address access
    virtualPagesMapped++;

    // already mapped
    if (process.pageTable[virtualPage].valid) {
        totalPageTableHits++;
        return false; // mapped free page, no page fault
    }

    // not mapped yet
    if (!freePages.empty()) {
        unsigned int physicalPage = freePages.back();
        freePages.pop_back();

        process.pageTable[virtualPage].valid = true;
        process.pageTable[virtualPage].phyPageNumber = physicalPage;
        process.usedPageTableEntries++;

        totalPagesFromFree++;
        return false; // mapped free page still no page fault
    } else {
        // otherwise nothing was free so pagefault should occur
        totalPageFaults++;

        // TODO (M3 - Page Replacement):
        // No free physical pages available.
        // Need to implement page replacement:
        //   1. Select a physical page to evict (could be from any process)
        //   2. Find which virtual page maps to that physical page
        //   3. Invalidate that page table entry
        //   4. (Optional but correct) Invalidate any cache blocks using that physical page
        //   5. Reassign the freed physical page to this virtual page

        // For now, we only count the page fault and do not remap.

        return true; // page fault has happened
    }
}

/**
 * @brief free process pages after reading a tradce file
 * @param process
 * @param freePages
 */
void freeProcessPages(ProcessInfo &process, vector<unsigned int> &freePages) {
    for (size_t v = 0; v < PAGE_TABLE_ENTRIES; v++) {
        if (process.pageTable[v].valid) {
            freePages.push_back(process.pageTable[v].phyPageNumber);
            process.pageTable[v].valid = false;
            process.pageTable[v].phyPageNumber = 0;
        }
    }
}

/*----------------------
 * M3 Functions
 *----------------------*/

//1. create sets
//2. create blocks in each set
//3. mark blocks invalid
//4. start round-robin pointer at 0

// Create and set up the cache
void initCache(vector<CacheSet> &cache, int totalRows, int associativity) {

    // Create the correct number of sets
    // Right now this just creates empty sets no blocks inside yet.
    // totalRows = totalBlocks / associativity
    // totalBlocks = cache size / block size
    // this holds a n number of CacheSets
    cache.resize(totalRows);

    // For EACH set, create the correct number of blocks.
    // - Each set should have 'associativity' number of CacheBlock objects
    // - associativity = 4: each set has 4 blocks
    // if wee skip this, the cache exists but can't store anything
    for (int i = 0; i < totalRows; i++) {
        // inside one set: cache[i]
        // give it some blocks based off associativity
        cache[i].blocks.resize(associativity);

        // then go through each block in this set: cache[i]
        for (int j = 0; j < associativity; j++) {
            // now inside one blocj
            // initialize the block
            // need to set each block isvalid and tag
            cache[i].blocks[j].isValid = false;
            cache[i].blocks[j].tag = 0;
       }
        // Reset replacement pointer for Round Robin.
        // without replacment then it wil probaly blow up
        cache[i].nextBlockToReplace = 0;
   }
 }





// Get the cache set index from a physical address
unsigned int getCacheIndex(unsigned physicalAddress, int offsetBits, int totalRows) {


    // Remove the block offset bits from the address.
    // The offset only tells us the byte inside the block, not which set to use.
    unsigned blockNumber = physicalAddress >> offsetBits;

    // map the number to one cache set
    // Multiple blocks will share the same set so it needs to wrap
    unsigned index = blockNumber % totalRows;



      // Use totalRows to keep the index inside the cache.
      // If this math is wrong, the cache will quietly lie to us.

    return index;
}

//Sean changes:
// Get the cache tag from a physical address
unsigned int getCacheTag(unsigned int physicalAddress, int offsetBits, int indexBits) {

    // Remove both the offset bits and index bits.
    // Whatever remains is the tag.
    unsigned int tag = physicalAddress >> (offsetBits + indexBits);

    // Return the tag so we can compare it against blocks in the selected set.
    // The tag is how we know whether the block is actually the one we wanted.
    
    return tag;
}

//Sean changes:
void accessCache(unsigned int physicalAddress,
                 vector<CacheSet> &cache,
                 CacheStats &stats,
                 int offsetBits,
                 int indexBits,
                 int totalRows,
                 int blockSize,
                 string replacementPolicy) {

    // find index + tag
    unsigned int index = getCacheIndex(physicalAddress, offsetBits, totalRows);
    unsigned int tag = getCacheTag(physicalAddress, offsetBits, indexBits);

    // find set
    CacheSet &set = cache[index];

    stats.totalCacheAccesses++;

    // check hit/miss
    for (auto &block : set.blocks) {
        if (block.isValid && block.tag == tag) {
            stats.hits++;

            // Cache hit cost 1 cycle epr cach eblock access.
            stats.totalCycles += 1;

            return; // exit if hit
        }
    }

    // miss if not returned
    stats.misses++;

    //Cache miss cost 4 cycles per 4byte memory read

    // check for empty blocks
    for (auto &block : set.blocks) {
        if (!block.isValid) {
            block.isValid = true;
            block.tag = tag;

            stats.compulsoryMisses++;
            return; //exit if empty block found
        }
    }

    // Conflict miss if not returned
    stats.conflictMisses++;

    int replaceIndex;
    
    //this is part of what im not sure about, hoping i interpreted these right
    if (replacementPolicy == "rr") {
        replaceIndex = set.nextBlockToReplace;
        set.nextBlockToReplace =
            (set.nextBlockToReplace + 1) % set.blocks.size();
    } else {
        replaceIndex = rand() % set.blocks.size();
    }

    set.blocks[replaceIndex].isValid = true;
    set.blocks[replaceIndex].tag = tag;
}

//Sean changes:
void accessCacheRange(unsigned int physicalAddress,
                      int length,
                      int blockSize,
                      vector<CacheSet> &cache,
                      CacheStats &cacheStats,
                      int offsetBits,
                      int indexBits,
                      int totalRows,
                      string replacementPolicyRaw)
{
    // had some issues with the first method being innaccurate, this worked as a solution, 
    // not gonna mess with it too much for now
    unsigned int startBlock = physicalAddress / blockSize;
    unsigned int endBlock;
    endBlock = (physicalAddress + length - 1) / blockSize;

    for (unsigned int block = startBlock; block <= endBlock; block++) {
        unsigned int blockAddress = block * blockSize;

        accessCache(blockAddress, cache, cacheStats, offsetBits, indexBits, totalRows, blockSize, replacementPolicyRaw);
    }
}

//----------------------------------------------------------------------------------------------------------------------
//  START OF MAIN
//----------------------------------------------------------------------------------------------------------------------
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

    // vector for free pages
    vector<unsigned int> freePages;

    /**
     * ---------------------------------------------------------
     * M3 cache varibles
     * ---------------------------------------------------------
     */

    // Cache data structure
    vector<CacheSet> cache;

    // Cache statistics
    CacheStats cacheStats;


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
        } else if (option == "-b") {
            if (i + 1 >= argc) {
                printUsageAndExit();
            }
            blockSize = atoi(argv[++i]);
        } else if (option == "-a") {
            if (i + 1 >= argc) {
                printUsageAndExit();
            }
            associativity = atoi(argv[++i]);
        } else if (option == "-r") {
            if (i + 1 >= argc) {
                printUsageAndExit();
            }

            replacementPolicyRaw = argv[++i];

            if (replacementPolicyRaw == "rr" || replacementPolicyRaw == "RR") {
                replacementPolicyPretty = "Round Robin";
            } else if (replacementPolicyRaw == "rnd" || replacementPolicyRaw == "RND") {
                replacementPolicyPretty = "Random";
            } else {
                cerr << "Error: replacement policy must be rr or rnd.\n";
                return 1;
            }
        } else if (option == "-p") {
            if (i + 1 >= argc) {
                printUsageAndExit();
            }
            physicalMemoryMB = atoi(argv[++i]);
        } else if (option == "-u") {
            if (i + 1 >= argc) {
                printUsageAndExit();
            }
            percentUsedByOS = atoi(argv[++i]);
        } else if (option == "-n") {
            if (i + 1 >= argc) {
                printUsageAndExit();
            }
            instructionsPerTimeSlice = atoi(argv[++i]);
        } else if (option == "-f") {
            if (i + 1 >= argc) {
                printUsageAndExit();
            }

            if (traceCount >= 3) {
                cerr << "Error: at most 3 trace files are allowed.\n";
                return 1;
            }

            traceFiles[traceCount] = argv[++i];
            traceCount++;
        } else {
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
     * 
     */
    //Sean changes:
    initCache(cache, totalRows, associativity);

    /**
     *fill the free pages
     */
    for (unsigned int p = numberOfPagesForSystem; p < (unsigned int) numberOfPhysicalPages; p++) {
        freePages.push_back(p);
    }

    /*--------------------------------------------------------------
     *
     * Setup proccess & page tables
     *-------------------------------------------------------------
     */

    // process array moved to global

    // Loop though each trace file
    for (int i = 0; i < traceCount; ++i) {
        // store the tracefile name
        processes[i].traceFileName = traceFiles[i];

        // no pages mapped yet
        processes[i].usedPageTableEntries = 0;

        //initialize page table so all entres start as invalid
        // this means no virtual pages are mapped to phy pages yet
        for (int j = 0; j < PAGE_TABLE_ENTRIES; ++j) {
            // mark as not mapped
            processes[i].pageTable[j].valid = false;

            // Initialize phy page number to 0
            processes[i].pageTable[j].phyPageNumber = 0;
        }
    }


    /**
     * -----------------------------------------------------------------------------------------------------------------
     * Read Trace Files
     * -----------------------------------------------------------------------------------------------------------------ra
     */

    int virtualPagesMapped = 0;
    string traceLine;

    // start the loop
    for (int i = 0; i < traceCount; i++) {
        ifstream traceFile(traceFiles[i]);

        if (!traceFile) {
            cout << "Error opening file: " << traceFiles[i] << endl;
            continue;
        }

        while (getline(traceFile, traceLine)) {

            // EIP line
            if (traceLine.find("EIP") == 0) {
                // Example: EIP (04): 7c809767 ...
                string hexAddr = traceLine.substr(10, 8);
                unsigned int addr = hexToUInt(hexAddr);
                int instructionLength = atoi(traceLine.substr(5, 2).c_str());
                bool pagefault = processAddress(addr,
                                processes[i],
                               freePages,
                               virtualPagesMapped,
                               totalPageTableHits,
                               totalPagesFromFree,
                               totalPageFaults);

                if (pagefault) {
                    cacheStats.totalCycles += 100; // page fault cost 100 cycles
                }
                //Over all flow
                // Virtual address -> page table -> physical address -> case

                // Convert virtual address to physical address before accessing the cache

                //,.1 Find which virtual page address belongs to
                // PAGE_SIZE = 4096, so this gives us the page number
                unsigned int virtualPage = addr / PAGE_SIZE;

                // Find the offset within the page, should give the byte location
                unsigned int offset = addr % PAGE_SIZE;

                //3. Look up the physical page number from the pge table
                // will tell if virtual page is in physical memory
                unsigned int physicalPage = processes[i].pageTable[virtualPage].phyPageNumber;

                //4 reconstruct the full address
                // (physical page base address + offset inside the page)
                unsigned int physicalAddress = (physicalPage * PAGE_SIZE) + offset;

                //5. nnow we access the cache using the physical address
                // this could overlap block depending on the size of the instruction
                accessCacheRange(physicalAddress, instructionLength, blockSize, cache,
                                cacheStats, offsetBits, indexBits, totalRows, replacementPolicyRaw);

                // track how many instructions have been processed
                cacheStats.instructionBytes += instructionLength;

                // should count as one instruction.
                cacheStats.totalInstructions++;
            }
            // dstM / srcM line
            else if (traceLine.find("dstM:") == 0) {
                // fixed-format parsing
                string dstAddrStr = traceLine.substr(6, 8);
                string dstDataStr = traceLine.substr(15, 8);
                //changed this to 33 from 32 because it was cutting off a number in my tinyTrace testing
                string srcAddrStr = traceLine.substr(33, 8);
                string srcDataStr = traceLine.substr(41, 8);

                // process dstM only if valid
                if (dstDataStr != "--------" && dstAddrStr != "00000000") {
                    unsigned int dstAddr = hexToUInt(dstAddrStr);

                    bool pageFault = processAddress(dstAddr,
                                   processes[i],
                                   freePages,
                                   virtualPagesMapped,
                                   totalPageTableHits,
                                   totalPagesFromFree,
                                   totalPageFaults);
                    if (pageFault) {
                        cacheStats.totalCycles += 100; // page fault cost 100 cycles
                    }

                    //Sean changes:
                    unsigned int virtualPage = dstAddr / PAGE_SIZE;
                    unsigned int offset = dstAddr % PAGE_SIZE;

                    unsigned int physicalPage = processes[i].pageTable[virtualPage].phyPageNumber;

                    unsigned int physicalAddress = (physicalPage * PAGE_SIZE) + offset;

                    accessCacheRange(physicalAddress, 4, blockSize, cache,
                                cacheStats, offsetBits, indexBits, totalRows, replacementPolicyRaw);


                    // Date access is +1 cycle for effective address calculation
                    cacheStats.totalCycles +=1;

                    cacheStats.dataBytes += 4;
                }

                // process srcM only if valid
                if (srcDataStr != "--------" && srcAddrStr != "00000000") {
                    unsigned int srcAddr = hexToUInt(srcAddrStr);

                   bool pageFault =  processAddress(srcAddr,
                                   processes[i],
                                   freePages,
                                   virtualPagesMapped,
                                   totalPageTableHits,
                                   totalPagesFromFree,
                                   totalPageFaults);

                    if (pageFault) {
                        cacheStats.totalCycles += 100; // page fault cost 100 cycles
                    }

                    //Sean changes:
                    unsigned int virtualPage = srcAddr / PAGE_SIZE;
                    unsigned int offset = srcAddr % PAGE_SIZE;

                    unsigned int physicalPage = processes[i].pageTable[virtualPage].phyPageNumber;

                    unsigned int physicalAddress = (physicalPage * PAGE_SIZE) + offset;

                    accessCacheRange(physicalAddress, 4, blockSize,
                                    cache, cacheStats, offsetBits, indexBits,
                                    totalRows, replacementPolicyRaw);

                    //Data access requiers +1 cycle for effective address
                    cacheStats.totalCycles += 1;

                    cacheStats.dataBytes += 4;
                }
            }
        }

        traceFile.close();
        if (cacheStats.totalCacheAccesses > 0) {
            cacheStats.hitRate = (cacheStats.hits * 100.0)/cacheStats.totalCacheAccesses;
        }
        // free the pages for current process in loop
        freeProcessPages(processes[i], freePages);
    }


    /**
    * ----------------------------------------------------------------------------------------------------------------------
    * Print required output
    * ----------------------------------------------------------------------------------------------------------------------
    */

    /*-------------------------------------------------------------------------------------
    * M1 Outputs
    * -------------------------------------------------------------------------------------
    */


    cout << "Cache Simulator - CS 3853 - Team #11\n\n";

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
    } else {
        cout << instructionsPerTimeSlice << '\n';
    }

    cout << "\n***** Cache Calculated Values *****\n\n";
    cout << left << setw(32) << "Total # Blocks:" << totalBlocks << '\n';

    /* Tag size is based on actual physical memory */
    cout << left << setw(32) << "Tag Size:" << tagBits << " bits\n";

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

    /* numberOfPagesForSystem = (percentUsedByOS / 100.0) * numberOfPhysicalPages */
    cout << left << setw(32) << "Number of Pages for System:"
            << numberOfPagesForSystem << '\n';

    /* pageTableEntryBits = 1 valid bit + physicalPageBits */
    cout << left << setw(32) << "Size of Page Table Entry:"
            << pageTableEntryBits << " bits\n";

    /* totalRAMForPageTables = 512K entries * traceCount * pageTableEntryBits / 8 */
    cout << left << setw(32) << "Total RAM for Page Table(s):"
            << totalRAMForPageTables << " bytes\n";

    /*------------------------------------------------------------------------------------------
     * M2 Outputs
     * -----------------------------------------------------------------------------------------
     */

    cout << left << setw(32) << "\n***** VIRTUAL MEMORY SIMULATION RESULTS *****\n" << endl;
    cout << left << setw(32) << "Physical Pages Used By SYSTEM: " << numberOfPagesForSystem << endl;
    cout << left << setw(32) << "Pages Available to User: " << totalPagesForUser << endl;
    cout << endl;
    cout << left << setw(32) << "Virtual Pages Mapped:" << virtualPagesMapped << endl;
    cout << "        ------------------------------" << endl;
    cout << left << setw(32) << "        Page Table Hits:" << totalPageTableHits << endl;
    cout << endl;
    cout << left << setw(32) << "        Pages from Free:" << totalPagesFromFree << endl;
    cout << endl;
    cout << left << setw(32) << "        Total Page Faults:" << totalPageFaults << endl;
    cout << endl;
    cout << endl;
    cout << "Page Table Usage Per Process:" << endl;
    cout << "------------------------------" << endl;
    for (int i = 0; i < traceCount; i++) {
        if (!traceFiles[i].empty()) {
            cout << "[" << i << "] " << traceFiles[i] << ":" << endl;
            cout << "\tUsed Page Table Entries: "
                    << processes[i].usedPageTableEntries;
            double percent = (double) processes[i].usedPageTableEntries / PAGE_TABLE_ENTRIES * 100;
            cout << " (" << fixed << setprecision(2) << percent << "%)" << endl;
            int unusedEntries = PAGE_TABLE_ENTRIES - processes[i].usedPageTableEntries;
            int wastedBytes = (unusedEntries * pageTableEntryBits) / 8;
            cout << "\tPage Table Wasted: " << wastedBytes << " bytes" << endl;
        }
    }

    /*-----------
        M3 OUTPUTS
     ----------
     */
    //double total = cacheStats.totalCycles/cacheStats.totalInstructions;

    // CPI = total cycles / total instructions
    // double to get the decimal division instead of int division
    double cpi = 0.0;

    // if there are instructions, then calculate
    if (cacheStats.totalInstructions  > 0) {
        cpi = (double) cacheStats.totalCycles / cacheStats.totalInstructions;
    }

    cout << left << setw(32) << "\n***** CACHE SIMULATION RESULTS*****\n" << endl;
    cout << left << setw(32) << "Total Cache Accesses: " << cacheStats.totalCacheAccesses << endl; // times cache row hit
    cout << left << setw(32) << "--- Instruction Bytes:  " << cacheStats.instructionBytes <<endl;
    cout << left << setw(32) << "--- SrcDst Bytes: " << cacheStats.dataBytes << endl;
    cout << left << setw(32) << "Cache Hits:" << cacheStats.hits << endl; // it was valid and tag matched
    cout << left << setw(32) << "Cache misses: " << cacheStats.misses << endl; // it was either not valid or tag didnt match
    cout << left << setw(32) << "--- Compulsory Misses:  " << cacheStats.compulsoryMisses << endl; // it was not valid
    cout << left << setw(32) << "--- Conflict Misses: " << cacheStats.conflictMisses << endl; // it was valid, tag did not match
    cout << endl;
    cout << endl;
    cout << left << setw(32) << "\n***** ***** CACCHE HIT & MISS RATE: ***** *****" << endl;
    cout << endl;
    cout << left << setw(32) << "Hit Rate: " << fixed << setprecision(4) << cacheStats.hitRate << "%\n"; // (Hits * 100) / total accesses
    cout << left << setw(32) << setprecision(4) << "Miss Rate: " << 100.0 - cacheStats.hitRate << "%" << endl; // 1 - Hit rate
    cout << left << setw(32) << "CPI:" << fixed << setprecision(2) << cpi << " Cycles/Instruction" << endl; // # Cycles/ # instructions

    // Unused blocks are block that were never filled
    // Compulsory misses is the number of blocks that were used at least once
    int unusedBlocks = totalBlocks - cacheStats.compulsoryMisses;

    // Each cache block uses data bytes plus overhead bits.
    // Convert overhead bits to bytes, avoid int division
    double overheadBytesPerBlock = overheadBitsPerBlock / 8.0;

    // Unused cache space = unused blocks * storage used per block
    double unusedCacheKB = (unusedBlocks * (blockSize + overheadBytesPerBlock)) / 1024.0;
    double unusedPercent = (unusedCacheKB / implementationMemorySizeKB) * 100.0;
    double wasteCost = unusedCacheKB * COST_PER_KB;
    cout << left << setw(32) << "Unused Cache Space: "
    << fixed << setprecision(2)
    << unusedCacheKB << "KB / "
    << implementationMemorySizeKB << " KB = "
    << unusedPercent << "% Waste: $"
    << wasteCost << "/chip" << endl;
    cout << left << setw(32) << "Unused Cache Blocks: " << unusedBlocks << " / " << totalBlocks << endl;

    // NOTE: A cache access is any time an address maps to a row.
    //       reading 7 bytes and hitting two rows is counted as two accesses, not 7.
    // Unused KB = ( (TotalBlocks-Compulsory Misses) * (BlockSize+OverheadSize) ) / 1024
    // The 1024 KB below is the total cache size for this example
    // Waste = COST/KB * Unused KB

 /*------------------------------------------------------------------------------------------
 * END
 * -----------------------------------------------------------------------------------------
 */

    return 0;
}
