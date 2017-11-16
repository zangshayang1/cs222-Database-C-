#include "pfm.h"
#include <fstream>
#include <sys/stat.h>
#include <cstdio>
#include <iostream>

const string FILE_STAT_SUFFIX = ".stat";
const string FILE_STAT_PREFIX = ".";
const unsigned int STAT_NUM = 3;
const unsigned int READ_PAGE_COUNTER_OFFSET = 0;
const unsigned int WRITE_PAGE_COUNTER_OFFSET = READ_PAGE_COUNTER_OFFSET + sizeof(unsigned int);
const unsigned int APPEND_PAGE_COUNTER_OFFSET = WRITE_PAGE_COUNTER_OFFSET + sizeof(unsigned int);

string statFileNameOf(const string fileName) {
    return FILE_STAT_PREFIX + fileName + FILE_STAT_SUFFIX;
}

PagedFileManager* PagedFileManager::_pf_manager = nullptr;

PagedFileManager* PagedFileManager::instance()
{
    if(!_pf_manager)
        _pf_manager = new PagedFileManager();
    
    return _pf_manager;
}


PagedFileManager::PagedFileManager()
{
}

// ~myfunc() : does the opposite as the myfunc() does
// keyword delete goes with new; function free() goes with malloc()
PagedFileManager::~PagedFileManager()
{
    delete _pf_manager;
}


bool PagedFileManager::fileExists(const string &filename) {
    struct stat file_stat;
    if (stat(filename.c_str(), &file_stat) == 0) {
        return true;
    }
    return false;
}


RC PagedFileManager::createFile(const string &fileName)
{
    if (fileExists(fileName) || fileExists(statFileNameOf(fileName))) {
        cout << "PagedFileManager::createFile(const string &fileName) -> the file already exists." << endl;
        return -1;
    }
    ofstream ofs; // <fstream>
    ofs.open(fileName);
    ofs.close();
    ofs.open(statFileNameOf(fileName));
    ofs.close();
    // make readPageCounter/writePageCounter/appendPageCounter persistent -> create a ".fileName.stat" for each "fileName".
    FILE * fptr = fopen(statFileNameOf(fileName).c_str(), "rb+");
    void * buffer = malloc(sizeof(unsigned int) * STAT_NUM);
    unsigned int counter = 0;
    memcpy((char*)buffer + READ_PAGE_COUNTER_OFFSET, & counter, sizeof(unsigned int));
    memcpy((char*)buffer + WRITE_PAGE_COUNTER_OFFSET, & counter, sizeof(unsigned int));
    memcpy((char*)buffer + APPEND_PAGE_COUNTER_OFFSET, & counter, sizeof(unsigned int));
    fwrite(buffer, sizeof(char), sizeof(unsigned int) * STAT_NUM, fptr);
    fflush(fptr);
    free(buffer);
    fclose(fptr);
    return 0;
}


RC PagedFileManager::destroyFile(const string &fileName)
{
    if (!fileExists(fileName) || !fileExists(statFileNameOf(fileName))) {
        return -1;
    }
    if (remove(fileName.c_str()) != 0 || remove(statFileNameOf(fileName).c_str()) != 0) {
        return -1;
    }
    return 0;
}


/*
 FILE * fopen ( const char * filename, const char * mode );
 if not success, a null pointer is returned.
 */
RC PagedFileManager::openFile(const string &fileName, FileHandle &fileHandle)
{
    if (!fileExists(fileName)) {
        return -1;
    }
    // read binary and update
    fileHandle.pFile = fopen(fileName.c_str(), "rb+");
    fileHandle.fileName = fileName;

    if (fileHandle.pFile == NULL) {
        return -1;
    }
    
    // read persistent counters from disk
    FILE * fptr = fopen(statFileNameOf(fileName).c_str(), "rb+");
    void * buffer = malloc(sizeof(unsigned int) * STAT_NUM);
    fread(buffer, sizeof(char), sizeof(unsigned int) * STAT_NUM, fptr);
    unsigned int readPageCounter;
    unsigned int writePageCounter;
    unsigned int appendPageCounter;
    memcpy(&readPageCounter, (char*)buffer + READ_PAGE_COUNTER_OFFSET, sizeof(unsigned int));
    memcpy(&writePageCounter, (char*)buffer + WRITE_PAGE_COUNTER_OFFSET, sizeof(unsigned int));
    memcpy(&appendPageCounter, (char*)buffer + APPEND_PAGE_COUNTER_OFFSET, sizeof(unsigned int));
    fileHandle.readPageCounter = readPageCounter;
    fileHandle.writePageCounter = writePageCounter;
    fileHandle.appendPageCounter = appendPageCounter;
    free(buffer);
    fclose(fptr);
    return 0;
}


RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
    if (fileHandle.pFile == NULL) {
        return -1;
    }
    
    FILE * fptr = fopen(statFileNameOf(fileHandle.fileName).c_str(), "rb+");
    void * buffer = malloc(sizeof(unsigned int) * STAT_NUM);
    memcpy((char*)buffer + READ_PAGE_COUNTER_OFFSET, & fileHandle.readPageCounter, sizeof(unsigned int));
    memcpy((char*)buffer + WRITE_PAGE_COUNTER_OFFSET, & fileHandle.writePageCounter, sizeof(unsigned int));
    memcpy((char*)buffer + APPEND_PAGE_COUNTER_OFFSET, & fileHandle.appendPageCounter, sizeof(unsigned int));
    fwrite(buffer, sizeof(char), sizeof(unsigned int) * STAT_NUM, fptr);
    fflush(fptr);
    free(buffer);
    fclose(fptr);
    
    fclose(fileHandle.pFile);
    
    return 0;
}




// constructor
FileHandle::FileHandle()
{
//    readPageCounter = 0;
//    writePageCounter = 0;
//    appendPageCounter = 0;
//    fileName = "";
//    pFile = NULL;
}

// deconstructor
FileHandle::~FileHandle()
{
}

/*
 fseek(FILE * fp, long offset, int position) where position can be commons such as: SEEK_SET/SEEK_CUR/SEEK_END
 fread(void * [ptr to a chunk of memory],
 size_t [size in bytes of each element to be read],
 int [number of elements to be read],
 FILE * fp)
 */
RC FileHandle::readPage(PageNum pageNum, void *data)
{
    // check eligibility
    if (pageNum >= getNumberOfPages()) {
        return -1;
    }
    // place pFile at the beginning of the corresponding page
    if (fseek(pFile, PAGE_SIZE * pageNum, SEEK_SET) != 0) {
        return -1;
    }
    // read 4096 bytes into *data
    // fread() returns the number of bytes read.
    if (fread(data, sizeof(char), PAGE_SIZE, pFile) == 0) {
        return -1;
    }
    //    fflush(pFile);
    readPageCounter++;
    return 0;
}


RC FileHandle::writePage(PageNum pageNum, const void *data)
{
    // check eligibility
    if (pageNum >= getNumberOfPages()) {
        return -1;
    }
    // find end
    if (fseek(pFile, PAGE_SIZE * pageNum, SEEK_SET) != 0) {
        return -1;
    }
    // fwrite() returns the number of bytes written.
    if (fwrite(data, sizeof(char), PAGE_SIZE, pFile) == 0) {
        return -1;
    }
    fflush(pFile);
    writePageCounter++;
    return 0;
}


RC FileHandle::appendPage(const void *data)
{
    if (fseek(pFile, 0, SEEK_END) != 0) {
        return -1;
    }
    // fwrite() returns the number of bytes written successfully.
    if (fwrite((char*)data, sizeof(char), PAGE_SIZE, pFile) == 0) {
        return -1;
    }
    fflush(pFile);
    appendPageCounter++;
    return 0;
}

/*
 
 ftell(FILE * fp) returns the position of file pointer in the file with respect to starting of the file
 */
unsigned FileHandle::getNumberOfPages()
{
    fseek(pFile, 0, SEEK_END);
    long pos = ftell(pFile);
    // if no page has ever been initialized, return 0;
    // if 1 page has been initialized, pos should equal to PAGE_SIZE, and it returns 1.
    return (unsigned) (pos / PAGE_SIZE);
    
}


RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
    readPageCount = readPageCounter;
    writePageCount = writePageCounter;
    appendPageCount = appendPageCounter;
    return 0;
}
