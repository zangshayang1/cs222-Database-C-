#include "pfm.h"


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

RC PagedFileManager::createFile(const string &fileName)
{
    
    if (_utils->fileExists(fileName) || _utils->fileExists(_utils->statFileNameOf(fileName))) {
        cout << "PagedFileManager::createFile(const string &fileName) -> the file already exists." << endl;
        return -1;
    }
    ofstream file; // <fstream>
    file.open(fileName.c_str());
    file.close();
    
    ofstream fileStat;
    fileStat.open(_utils->statFileNameOf(fileName).c_str());
    fileStat.close();
    
    // make readPageCounter/writePageCounter/appendPageCounter persistent -> create a ".fileName.stat" for each "fileName".
    FILE * fptr = fopen(_utils->statFileNameOf(fileName).c_str(), "rb+");
    void * buffer = malloc(sizeof(unsigned) * STAT_NUM);
    unsigned counter = 0;
    memcpy((char*)buffer + READ_PAGE_COUNTER_OFFSET, & counter, sizeof(unsigned));
    memcpy((char*)buffer + WRITE_PAGE_COUNTER_OFFSET, & counter, sizeof(unsigned));
    memcpy((char*)buffer + APPEND_PAGE_COUNTER_OFFSET, & counter, sizeof(unsigned));
    
    fwrite((char*)buffer, sizeof(char), sizeof(unsigned) * STAT_NUM, fptr);
    fflush(fptr);
    free(buffer);
    fclose(fptr);
    return 0;
}


RC PagedFileManager::destroyFile(const string &fileName)
{
    if (!_utils->fileExists(fileName) || !_utils->fileExists(_utils->statFileNameOf(fileName))) {
        return -1;
    }
    if (remove(fileName.c_str()) != 0 || remove(_utils->statFileNameOf(fileName).c_str()) != 0) {
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
    if (!_utils->fileExists(fileName)) {
        return -1;
    }
    // read binary and update
    fileHandle.pFile = fopen(fileName.c_str(), "rb+");
    fileHandle.fileName = fileName;

    if (fileHandle.pFile == NULL) {
        return -1;
    }
    
    // read persistent counters from disk
    FILE * fptr = fopen(_utils->statFileNameOf(fileName).c_str(), "rb+");
    void * buffer = malloc(sizeof(unsigned) * STAT_NUM);
    fread(buffer, sizeof(char), sizeof(unsigned) * STAT_NUM, fptr);
    unsigned readPageCounter;
    unsigned writePageCounter;
    unsigned appendPageCounter;
    memcpy(&readPageCounter, (char*)buffer + READ_PAGE_COUNTER_OFFSET, sizeof(unsigned));
    memcpy(&writePageCounter, (char*)buffer + WRITE_PAGE_COUNTER_OFFSET, sizeof(unsigned));
    memcpy(&appendPageCounter, (char*)buffer + APPEND_PAGE_COUNTER_OFFSET, sizeof(unsigned));
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
    
    FILE * fptr = fopen(_utils->statFileNameOf(fileHandle.fileName).c_str(), "rb+");
    void * buffer = malloc(sizeof(unsigned) * STAT_NUM);
    memcpy((char*)buffer + READ_PAGE_COUNTER_OFFSET, &fileHandle.readPageCounter, sizeof(unsigned));
    memcpy((char*)buffer + WRITE_PAGE_COUNTER_OFFSET, &fileHandle.writePageCounter, sizeof(unsigned));
    memcpy((char*)buffer + APPEND_PAGE_COUNTER_OFFSET, &fileHandle.appendPageCounter, sizeof(unsigned));
    fwrite((char*)buffer, sizeof(char), sizeof(unsigned) * STAT_NUM, fptr);
    fflush(fptr);
    free(buffer);
    fclose(fptr);
    fclose(fileHandle.pFile);
    
    return 0;
}




// constructor
FileHandle::FileHandle()
{
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
    fileName = "";
    pFile = NULL;
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
    
    fflush(pFile);
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
    if (fwrite((char*)data, sizeof(char), PAGE_SIZE, pFile) == 0) {
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
    
    if (pos < 0)
    {
        cout << "getNumberOfPages() failed." << endl;
        return -1;
    }
    
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
