#ifndef _pfm_h_
#define _pfm_h_

typedef unsigned PageNum;
typedef unsigned SlotNum;
typedef int RC;
typedef char byte;
typedef unsigned char CompressedPageNum[3];
typedef unsigned char CompressedSlotNum[2];

#define PAGE_SIZE 4096

#include <stdio.h>
#include <string>
#include <climits>
#include <stdio.h>

using namespace std;

class FileHandle;

class PagedFileManager
{
public:
    static PagedFileManager* instance();                                  // Access to the _pf_manager instance
    
    RC createFile    (const string &fileName);                            // Create a new file
    RC destroyFile   (const string &fileName);                            // Destroy a file
    RC openFile      (const string &fileName, FileHandle &fileHandle);    // Open a file
    RC closeFile     (FileHandle &fileHandle);                            // Close a file
    
    bool fileExists(const string &filename);
    
protected:
    PagedFileManager();                                                   // Constructor
    ~PagedFileManager();                                                  // Destructor
    
private:
    static PagedFileManager *_pf_manager;
    
};


class FileHandle
{
public:
    // variables to keep the counter for each operation
    unsigned readPageCounter;
    unsigned writePageCounter;
    unsigned appendPageCounter;
    FILE * pFile; // <stdio.h>
    
    FileHandle();                                                         // Default constructor
    ~FileHandle();                                                        // Destructor
    
    RC readPage(PageNum pageNum, void *data);
    // Get a specific page
    RC writePage(PageNum pageNum, const void *data);
    // Write a specific page
    RC appendPage(const void *data);
    // Append a specific page
    
    unsigned getNumberOfPages();                                          // Get the number of pages in the file
    RC collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount);  // Put the current counter values into variables
    
};

#endif /* pfm_hpp */
