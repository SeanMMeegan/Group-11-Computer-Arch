public class Deliverable1 {
    public static int cacheSize;
    public static int blockSize;
    public static int associativity;
    public static String replacementPolicy;
    public static int physicalMemory;
    public static int instruction;
    public static int physicalMemoryUsedByOS;
    public static void main(String[] args) {
        String[] arr = {" ", " ", " "};
        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("-f")) {
                arr[0] = String.valueOf(args[i + 1]);
                arr[2] = arr[1];
                arr[1] = arr[0];
            }
             if (args[i].equals("-s")) {
                cacheSize = Integer.parseInt(args[i + 1]) * 1024;
            }
            if (args[i].equals("-b")) {
                blockSize = Integer.parseInt(args[i + 1]);
            }
            if (args[i].equals("-a")) {
                associativity = Integer.parseInt(args[i + 1]);                
            }
            if (args[i].equals("-r")) {
                replacementPolicy = args[i + 1];
                
            }
            if (args[i].equals("-p")) {
                physicalMemory = Integer.parseInt(args[i + 1]) * 1024 * 1024;
            }
            if (args[i].equals("-n")) {
                instruction = Integer.parseInt(args[i + 1]);
            }
            if (args[i].equals("-u")) {
                physicalMemoryUsedByOS = Integer.parseInt(args[i + 1]);
            }
        }
        System.out.println(arr[0] + " " + arr[1] + " " + arr[2]);
        int numberOfBlocks = (cacheSize) / blockSize;
        int physicalAddressBits = (int)(Math.log(physicalMemory) / Math.log(2));
        int numberOfRows = numberOfBlocks / associativity;
        int offsetBits = (int)(Math.log(blockSize) / Math.log(2));
        int indexBits = (int)(Math.log(numberOfRows) / Math.log(2));
        int tagBits = physicalAddressBits - indexBits - offsetBits;
        
        int overheadPerLine = tagBits + 1;
        int totalOverheadBits = (numberOfBlocks * overheadPerLine) / 8;
        
        int impMemorySize = cacheSize + totalOverheadBits;

        double cost = (float) impMemorySize;
        cost = cost * 0.07;

        //Physical Memory
        
        
        

        
        System.out.println("Total # Blocks: " + numberOfBlocks);
        System.out.println("Tag Size: " + tagBits);
        System.out.println("Index Size: " + indexBits);
        System.out.println("Total # Rows: " + numberOfRows);
        System.out.println("Overhead Size: " + totalOverheadBits);
        System.out.println("Implementation Memory Size: " + impMemorySize/1024 + " " + impMemorySize);
        System.out.println("Cost: " + cost + " @ $0.07 per KB");
        System.out.println("Physical Memory");
        int pageSize = 4096;
        int numOfPages = physicalMemory / pageSize;
        System.out.println("Number of Physical Pages: " + numOfPages);
        int sizeOfPageTable = 1+ (int) (Math.log(numOfPages) / (Math.log(2)));
        double pagesForSystem = numOfPages;
        pagesForSystem = pagesForSystem * 0.75;
        System.out.println("Number of Pages for System: " + pagesForSystem);
        System.out.println("Size of Page Table Entry: " + sizeOfPageTable);
        int totalRam = cacheSize * 3 * sizeOfPageTable / 8;
        System.out.println("Total RAM for Page Table(s): " + totalRam);


    }
}
