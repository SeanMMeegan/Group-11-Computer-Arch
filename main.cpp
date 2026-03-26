#include <string>
#include <string.h>
#include <iostream>
#include <bits/stdc++.h>

using namespace std;

int main(int argc, char* argv[]) {
    int cacheSize = 0;
    int blockSize = 0;
    int associativity = 0;
    string replacementPolicy = "";
    int physicalMemory = 0;
    int instruction = 0;
    int physicalMemoryUsedByOS = 0;

    if (argc < 16) {
	    cout << "Not enough arguments. Now exiting." << endl;
	    exit(1);
    }
    string arr[3] = {" ", " ", " "};
    for (int i = 1; i < argc ; i++) {
	    if (strcmp(argv[i], "-f") == 0) {
                arr[0] = argv[i + 1];
                arr[2] = arr[1];
                arr[1] = arr[0];
            }
            if (strcmp(argv[i], "-s") == 0) {
                cacheSize = atoi(argv[i + 1]) * 1024;
            }
            if (strcmp(argv[i], "-b") == 0) {
                blockSize = atoi(argv[i + 1]);
            }
            if (strcmp(argv[i], "-a") == 0) {
                associativity = atoi(argv[i + 1]);                
            }
            if (strcmp(argv[i], "-r") == 0) {
                replacementPolicy = argv[i + 1];
            }
            if (strcmp(argv[i], "-p") == 0) {
                physicalMemory = atoi(argv[i + 1]) * 1024 * 1024;
            }
            if (strcmp(argv[i], "-n") == 0) {
                instruction = atoi(argv[i + 1]);
            }
            if (strcmp(argv[i], "-u") == 0) {
                physicalMemoryUsedByOS = atoi(argv[i + 1]);
            }
        }
	cout << "Trace File(s):" << endl;
	cout << "\t" + arr[0] + "\n\t" + arr[1] + "\n\t" + arr[2] << endl;
        int numberOfBlocks = (cacheSize) / blockSize;
        int physicalAddressBits = (int)(log(physicalMemory) / log(2));
        int numberOfRows = numberOfBlocks / associativity;
        int offsetBits = (int)(log(blockSize) / log(2));
        int indexBits = (int)(log(numberOfRows) / log(2));
        int tagBits = physicalAddressBits - indexBits - offsetBits;
        int overheadPerLine = tagBits + 1;
        int totalOverheadBits = (numberOfBlocks * overheadPerLine) / 8;
        int impMemorySize = cacheSize + totalOverheadBits;

        double cost = (float) impMemorySize;
        cost = cost * 0.07;

        //Physical Memory
	cout << "\n***** Cache Input Parameters *****\n" << endl;
	cout << left << setw(34) << "Cache Size: "<< cacheSize << endl;
	cout << left << setw(34) << "Block Size: "<< blockSize << endl;
	cout << left << setw(34) << "Associativity: "<< associativity << endl;
	cout << left << setw(34) << "Replacement Policy: "<< replacementPolicy << endl;
	cout << left << setw(34) << "Physical Memory: "<< physicalMemory << endl;
	cout << left << setw(34) << "Percent Memory Used by System: "<< physicalMemoryUsedByOS << endl;
	cout << left << setw(34) << "Instructions / Time Slice: "<< instruction << endl;
	cout << "\n***** Cache Calculated Values *****\n" << endl;
        cout << left << setw(34) <<    "Total # Blocks: " << numberOfBlocks << endl;
        cout << left << setw(34) <<    "Tag Size: " << tagBits << endl;
        cout << left << setw(34) <<    "Index Size: " << indexBits << endl;
        cout << left << setw(34) <<    "Total # Rows: " << numberOfRows << endl;
        cout << left << setw(34) <<    "Overhead Size: " << totalOverheadBits << endl;
        cout << left << setw(34) <<    "Implementation Memory Size: " <<  impMemorySize/1024 <<  " " << impMemorySize << endl;
        cout << left << setw(34) <<    "Cost: " <<  cost <<  " @ $0.07 per KB" << endl;
	cout << "\n***** Physical Memory Calculated Values *****\n" << endl;
        int pageSize = 4096;
        int numOfPages = physicalMemory / pageSize;
        int sizeOfPageTable = 1+ (int) (log(numOfPages) / (log(2)));
        double pagesForSystem = numOfPages;
        int totalRam = cacheSize * 3 * sizeOfPageTable / 8;
        pagesForSystem = pagesForSystem * 0.75;
        cout << left << setw(34) <<    "Number of Physical Pages: " << numOfPages << endl;
        cout << left << setw(34) <<    "Number of Pages for System: " <<  pagesForSystem << endl;
        cout << left << setw(34) <<    "Size of Page Table Entry: " << sizeOfPageTable << endl;
        cout << left << setw(34) <<    "Total RAM for Page Table(s): " <<  totalRam << endl;

}
