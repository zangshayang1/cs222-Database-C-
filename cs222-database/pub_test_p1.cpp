#include "../FileManager/test_util.h"

using namespace std;

int RBFTest_1(PagedFileManager *pfm)
{
    // Functions Tested:
    // 1. Create File
    cout << endl << "***** In RBF Test Case 1 *****" << endl;
    
    RC rc;
    string fileName = "test1";
    
    // Create a file named "test"
    rc = pfm->createFile(fileName);
    assert(rc == success && "Creating the file failed.");
    
    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating the file failed.");
    
    // Create "test" again, should fail
    rc = pfm->createFile(fileName);
    assert(rc != success && "Creating the same file should fail.");
    
    cout << "RBF Test Case 1 Finished! The result will be examined." << endl << endl;
    return 0;
}

int RBFTest_2(PagedFileManager *pfm)
{
    // Functions Tested:
    // 1. Destroy File
    cout << endl << "***** In RBF Test Case 2 *****" << endl;
    
    RC rc;
    string fileName = "test1";
    
    rc = pfm->destroyFile(fileName);
    assert(rc == success  && "Destroying the file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName);
    assert(rc == success  && "Destroying the file should not fail.");
    
    // Destroy "test1" again, should fail
    rc = pfm->destroyFile(fileName);
    assert(rc != success && "Destroy the same file should fail.");
    
    cout << "RBF Test Case 2 Finished! The result will be examined." << endl << endl;
    return 0;
}

int RBFTest_3(PagedFileManager *pfm)
{
    // Functions Tested:
    // 1. Create File
    // 2. Open File
    // 3. Get Number Of Pages
    // 4. Close File
    cout << endl << "***** In RBF Test Case 3 *****" << endl;
    
    RC rc;
    string fileName = "test3";
    
    // Create a file named "test3"
    rc = pfm->createFile(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    // Open the file
    FileHandle fileHandle;
    rc = pfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    // Get the number of pages in the test file. In this case, it should be zero.
    unsigned count = fileHandle.getNumberOfPages();
    assert(count == (unsigned)0 && "The page count should be zero at this moment.");
    
    // Close the file
    rc = pfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    cout << "RBF Test Case 3 Finished! The result will be examined." << endl << endl;
    
    return 0;
}

int RBFTest_4(PagedFileManager *pfm)
{
    // Functions Tested:
    // 1. Open File
    // 2. Append Page
    // 3. Get Number Of Pages
    // 4. Get Counter Values
    // 5. Close File
    cout << endl << "***** In RBF Test Case 4 *****" << endl;
    
    RC rc;
    string fileName = "test3";
    
    unsigned readPageCount = 0;
    unsigned writePageCount = 0;
    unsigned appendPageCount = 0;
    unsigned readPageCount1 = 0;
    unsigned writePageCount1 = 0;
    unsigned appendPageCount1 = 0;
    
    // Open the file "test3"
    FileHandle fileHandle;
    rc = pfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    // Collect before counters
    rc = fileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case 4 failed." << endl;
        rc = pfm->closeFile(fileHandle);
        return -1;
    }
    
    // Append the first page
    void *data = malloc(PAGE_SIZE);
    for(unsigned i = 0; i < PAGE_SIZE; i++)
    {
        *((char *)data+i) = i % 96 + 30;
    }
    rc = fileHandle.appendPage(data);
    assert(rc == success && "Appending a page should not fail.");
    
    // collect after counters
    rc = fileHandle.collectCounterValues(readPageCount1, writePageCount1, appendPageCount1);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case 4 failed." << endl;
        rc = pfm->closeFile(fileHandle);
        return -1;
    }
    cout << "before:R W A - " << readPageCount << " " << writePageCount << " " << appendPageCount << " after:R W A - " << readPageCount1 << " " << writePageCount1 << " " << appendPageCount1 << endl;
    assert(appendPageCount1 > appendPageCount && "The appendPageCount should have been increased.");
    
    // Get the number of pages
    unsigned count = fileHandle.getNumberOfPages();
    assert(count == (unsigned)1 && "The count should be one at this moment.");
    
    // Close the file "test3"
    rc = pfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    free(data);
    
    cout << "RBF Test Case 4 Finished! The result will be examined." << endl << endl;
    
    return 0;
}

int RBFTest_5(PagedFileManager *pfm)
{
    // Functions Tested:
    // 1. Open File
    // 2. Read Page
    // 3. Close File
    cout << endl << "***** In RBF Test Case 5 *****" << endl;
    
    RC rc;
    string fileName = "test3";
    
    unsigned readPageCount = 0;
    unsigned writePageCount = 0;
    unsigned appendPageCount = 0;
    unsigned readPageCount1 = 0;
    unsigned writePageCount1 = 0;
    unsigned appendPageCount1 = 0;
    
    // Open the file "test3"
    FileHandle fileHandle;
    rc = pfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    // collect before counters
    rc = fileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case 5 failed." << endl;
        rc = pfm->closeFile(fileHandle);
        return -1;
    }
    
    // Read the first page
    void *buffer = malloc(PAGE_SIZE);
    rc = fileHandle.readPage(0, buffer);
    assert(rc == success && "Reading a page should not fail.");
    
    // collect after counters
    rc = fileHandle.collectCounterValues(readPageCount1, writePageCount1, appendPageCount1);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case 5 failed." << endl;
        rc = pfm->closeFile(fileHandle);
        return -1;
    }
    cout << "before:R W A - " << readPageCount << " " << writePageCount << " " << appendPageCount << " after:R W A - " << readPageCount1 << " " << writePageCount1 << " " << appendPageCount1 << endl;
    assert(readPageCount1 > readPageCount && "The readPageCount should have been increased.");
    
    // Check the integrity of the page
    void *data = malloc(PAGE_SIZE);
    for(unsigned i = 0; i < PAGE_SIZE; i++)
    {
        *((char *)data+i) = i % 96 + 30;
    }
    rc = memcmp(data, buffer, PAGE_SIZE);
    assert(rc == success && "Checking the integrity of the page should not fail.");
    
    // Read a non-existing page
    rc = fileHandle.readPage(1, buffer);
    assert(rc != success && "Reading a non-existing page should fail.");
    
    // Close the file "test3"
    rc = pfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    free(data);
    free(buffer);
    
    cout << "RBF Test Case 5 Finished! The result will be examined." << endl << endl;
    
    return 0;
}

int RBFTest_6(PagedFileManager *pfm)
{
    // Functions Tested:
    // 1. Open File
    // 2. Write Page
    // 3. Read Page
    // 4. Close File
    // 5. Destroy File
    cout << endl << "***** In RBF Test Case 6 *****" << endl;
    
    RC rc;
    string fileName = "test3";
    
    unsigned readPageCount = 0;
    unsigned writePageCount = 0;
    unsigned appendPageCount = 0;
    unsigned readPageCount1 = 0;
    unsigned writePageCount1 = 0;
    unsigned appendPageCount1 = 0;
    
    // Open the file "test3"
    FileHandle fileHandle;
    rc = pfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    // collect before counters
    rc = fileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case 6 failed." << endl;
        rc = pfm->closeFile(fileHandle);
        return -1;
    }
    
    // Update the first page
    void *data = malloc(PAGE_SIZE);
    for(unsigned i = 0; i < PAGE_SIZE; i++)
    {
        *((char *)data+i) = i % 10 + 30;
    }
    rc = fileHandle.writePage(0, data);
    assert(rc == success && "Writing a page should not fail.");
    
    // Read the page
    void *buffer = malloc(PAGE_SIZE);
    rc = fileHandle.readPage(0, buffer);
    assert(rc == success && "Reading a page should not fail.");
    
    // collect after counters
    rc = fileHandle.collectCounterValues(readPageCount1, writePageCount1, appendPageCount1);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case 6 failed." << endl;
        rc = pfm->closeFile(fileHandle);
        return -1;
    }
    cout << "before:R W A - " << readPageCount << " " << writePageCount << " " << appendPageCount << " after:R W A - " << readPageCount1 << " " << writePageCount1 << " " << appendPageCount1 << endl;
    assert(writePageCount1 > writePageCount && "The writePageCount should have been increased.");
    assert(readPageCount1 > readPageCount && "The readPageCount should have been increased.");
    
    // Check the integrity
    rc = memcmp(data, buffer, PAGE_SIZE);
    assert(rc == success && "Checking the integrity of a page should not fail.");
    
    // Write a non-existing page
    rc = fileHandle.writePage(1, buffer);
    assert(rc != success && "Writing a non-existing page should fail.");
    
    // Close the file "test3"
    rc = pfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    free(data);
    free(buffer);
    
    // Destroy the file
    rc = pfm->destroyFile(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName);
    assert(rc == success  && "Destroying the file should not fail.");
    
    cout << "RBF Test Case 6 Finished! The result will be examined." << endl << endl;
    
    return 0;
    
}

int RBFTest_7(PagedFileManager *pfm)
{
    // Functions Tested:
    // 1. Create File
    // 2. Open File
    // 3. Append Page
    // 4. Get Number Of Pages
    // 5. Read Page
    // 6. Write Page
    // 7. Close File
    // 8. Destroy File
    cout << endl << "***** In RBF Test Case 7 *****" << endl;
    
    RC rc;
    string fileName = "test7";
    
    unsigned readPageCount = 0;
    unsigned writePageCount = 0;
    unsigned appendPageCount = 0;
    unsigned readPageCount1 = 0;
    unsigned writePageCount1 = 0;
    unsigned appendPageCount1 = 0;
    
    // Create the file named "test7"
    rc = pfm->createFile(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating the file failed.");
    
    // Open the file "test7"
    FileHandle fileHandle;
    rc = pfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    // collect before counters
    rc = fileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case 7 failed." << endl;
        rc = pfm->closeFile(fileHandle);
        return -1;
    }
    
    // Append 100 pages
    void *data = malloc(PAGE_SIZE);
    for(unsigned j = 0; j < 100; j++)
    {
        for(unsigned i = 0; i < PAGE_SIZE; i++)
        {
            *((char *)data+i) = i % (j+1) + 30;
        }
        rc = fileHandle.appendPage(data);
        assert(rc == success && "Appending a page should not fail.");
    }
    cout << "100 Pages have been successfully appended!" << endl;
    
    // collect after counters
    rc = fileHandle.collectCounterValues(readPageCount1, writePageCount1, appendPageCount1);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case 7 failed." << endl;
        rc = pfm->closeFile(fileHandle);
        return -1;
    }
    cout << "before:R W A - " << readPageCount << " " << writePageCount << " " << appendPageCount << " after:R W A - " << readPageCount1 << " " << writePageCount1 << " "  << appendPageCount1 << endl;
    assert(appendPageCount1 > appendPageCount && "The appendPageCount should have been increased.");
    
    // Get the number of pages
    unsigned count = fileHandle.getNumberOfPages();
    assert(count == (unsigned)100 && "The count should be 100 at this moment.");
    
    // Read the 87th page and check integrity
    void *buffer = malloc(PAGE_SIZE);
    rc = fileHandle.readPage(86, buffer);
    assert(rc == success && "Reading a page should not fail.");
    
    for(unsigned i = 0; i < PAGE_SIZE; i++)
    {
        *((char *)data + i) = i % 87 + 30;
    }
    rc = memcmp(buffer, data, PAGE_SIZE);
    assert(rc == success && "Checking the integrity of a page should not fail.");
    cout << "The data in 87th page is correct!" << endl;
    
    // Update the 87th page
    for(unsigned i = 0; i < PAGE_SIZE; i++)
    {
        *((char *)data+i) = i % 60 + 30;
    }
    rc = fileHandle.writePage(86, data);
    assert(rc == success && "Writing a page should not fail.");
    
    // Read the 87th page and check integrity
    rc = fileHandle.readPage(86, buffer);
    assert(rc == success && "Reading a page should not fail.");
    
    rc = memcmp(buffer, data, PAGE_SIZE);
    assert(rc == success && "Checking the integrity of a page should not fail.");
    
    // Close the file "test7"
    rc = pfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    // Destroy the file
    rc = pfm->destroyFile(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName);
    assert(rc == success  && "Destroying the file should not fail.");
    
    free(data);
    free(buffer);
    
    cout << "RBF Test Case 7 Finished! The result will be examined." << endl << endl;
    
    return 0;
}

int RBFTest_8(RecordBasedFileManager *rbfm) {
    // Functions tested
    // 1. Create Record-Based File
    // 2. Open Record-Based File
    // 3. Insert Record
    // 4. Read Record
    // 5. Close Record-Based File
    // 6. Destroy Record-Based File
    cout << endl << "***** In RBF Test Case 8 *****" << endl;
    
    RC rc;
    string fileName = "test8";
    
    // Create a file named "test8"
    rc = rbfm->createFile(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    // Open the file "test8"
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    RID rid;
    int recordSize = 0;
    void *record = malloc(100);
    void *returnedData = malloc(100);
    
    vector<Attribute> recordDescriptor;
    createRecordDescriptor(recordDescriptor);
    
    // Initialize a NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptor.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    // Insert a record into a file and print the record
    prepareRecord(recordDescriptor.size(), nullsIndicator, 8, "Anteater", 25, 177.8, 6200, record, &recordSize);
    cout << endl << "Inserting Data:" << endl;
    rbfm->printRecord(recordDescriptor, record);
    
    rc = rbfm->insertRecord(fileHandle, recordDescriptor, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    
    // Given the rid, read the record from file
    rc = rbfm->readRecord(fileHandle, recordDescriptor, rid, returnedData);
    assert(rc == success && "Reading a record should not fail.");
    
    cout << endl << "Returned Data:" << endl;
    rbfm->printRecord(recordDescriptor, returnedData);
    
    // Compare whether the two memory blocks are the same
    if(memcmp(record, returnedData, recordSize) != 0)
    {
        cout << "[FAIL] Test Case 8 Failed!" << endl << endl;
        free(record);
        free(returnedData);
        return -1;
    }
    
    cout << endl;
    
    // Close the file "test8"
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    // Destroy the file
    rc = rbfm->destroyFile(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName);
    assert(rc == success  && "Destroying the file should not fail.");
    
    free(record);
    free(returnedData);
    
    cout << "RBF Test Case 8 Finished! The result will be examined." << endl << endl;
    
    return 0;
}

int RBFTest_8b(RecordBasedFileManager *rbfm) {
    // Functions tested
    // 1. Create Record-Based File
    // 2. Open Record-Based File
    // 3. Insert Record - NULL
    // 4. Read Record
    // 5. Close Record-Based File
    // 6. Destroy Record-Based File
    cout << endl << "***** In RBF Test Case 8b *****" << endl;
    
    RC rc;
    string fileName = "test8b";
    
    // Create a file named "test8b"
    rc = rbfm->createFile(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating the file failed.");
    
    // Open the file "test8b"
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    RID rid;
    int recordSize = 0;
    void *record = malloc(100);
    void *returnedData = malloc(100);
    
    vector<Attribute> recordDescriptor;
    createRecordDescriptor(recordDescriptor);
    
    // NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptor.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    // Setting the age & salary fields value as null
    nullsIndicator[0] = 80; // 01010000
    
    // Insert a record into a file
    prepareRecord(recordDescriptor.size(), nullsIndicator, 8, "Anteater", NULL, 177.8, NULL, record, &recordSize);
    cout << endl << "Inserting Data:" << endl;
    rbfm->printRecord(recordDescriptor, record);
    
    rc = rbfm->insertRecord(fileHandle, recordDescriptor, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    
    // Given the rid, read the record from file
    rc = rbfm->readRecord(fileHandle, recordDescriptor, rid, returnedData);
    assert(rc == success && "Reading a record should not fail.");
    
    // The salary field should not be printed
    cout << endl << "Returned Data:" << endl;
    rbfm->printRecord(recordDescriptor, returnedData);
    
    // Compare whether the two memory blocks are the same
    if(memcmp(record, returnedData, recordSize) != 0)
    {
        cout << "[FAIL] Test Case 8b Failed!" << endl << endl;
        free(record);
        free(returnedData);
        return -1;
    }
    
    cout << endl;
    
    // Close the file "test8b"
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    // Destroy File
    rc = rbfm->destroyFile(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName);
    assert(rc == success  && "Destroying the file should not fail.");
    
    free(record);
    free(returnedData);
    
    cout << "RBF Test Case 8b Finished! The result will be examined." << endl << endl;
    
    return 0;
}

int RBFTest_9(RecordBasedFileManager *rbfm, vector<RID> &rids, vector<int> &sizes) {
    // Functions tested
    // 1. Create Record-Based File
    // 2. Open Record-Based File
    // 3. Insert Multiple Records
    // 4. Close Record-Based File
    cout << endl << "***** In RBF Test Case 9 *****" << endl;
    
    RC rc;
    string fileName = "test9";
    
    // Create a file named "test9"
    rc = rbfm->createFile(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating the file failed.");
    
    // Open the file "test9"
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    RID rid;
    void *record = malloc(1000);
    int numRecords = 2000;
    
    vector<Attribute> recordDescriptor;
    createLargeRecordDescriptor(recordDescriptor);
    
    for(unsigned i = 0; i < recordDescriptor.size(); i++)
    {
        cout << "Attr Name: " << recordDescriptor[i].name << " Attr Type: " << (AttrType)recordDescriptor[i].type << " Attr Len: " << recordDescriptor[i].length << endl;
    }
    cout << endl;
    
    // NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptor.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    // Insert 2000 records into file
    for(int i = 0; i < numRecords; i++)
    {
        // Test insert Record
        int size = 0;
        memset(record, 0, 1000);
        prepareLargeRecord(recordDescriptor.size(), nullsIndicator, i, record, &size);
        
        rc = rbfm->insertRecord(fileHandle, recordDescriptor, record, rid);
        assert(rc == success && "Inserting a record should not fail.");
        
        rids.push_back(rid);
        sizes.push_back(size);
    }
    // Close the file "test9"
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    free(record);
    
    
    // Write RIDs to the disk. Do not use this code in your codebase. This is not a PAGE-BASED operation - for the test purpose only.
    ofstream ridsFile("test9rids", ios::out | ios::trunc | ios::binary);
    
    if (ridsFile.is_open()) {
        ridsFile.seekp(0, ios::beg);
        for (int i = 0; i < numRecords; i++) {
            ridsFile.write(reinterpret_cast<const char*>(&rids[i].pageNum),
                           sizeof(unsigned));
            ridsFile.write(reinterpret_cast<const char*>(&rids[i].slotNum),
                           sizeof(unsigned));
            if (i % 1000 == 0) {
                cout << "RID #" << i << ": " << rids[i].pageNum << ", "
                << rids[i].slotNum << endl;
            }
        }
        ridsFile.close();
    }
    
    // Write sizes vector to the disk. Do not use this code in your codebase. This is not a PAGE-BASED operation - for the test purpose only.
    ofstream sizesFile("test9sizes", ios::out | ios::trunc | ios::binary);
    
    if (sizesFile.is_open()) {
        sizesFile.seekp(0, ios::beg);
        for (int i = 0; i < numRecords; i++) {
            sizesFile.write(reinterpret_cast<const char*>(&sizes[i]),sizeof(int));
            if (i % 1000 == 0) {
                cout << "Sizes #" << i << ": " << sizes[i] << endl;
            }
        }
        sizesFile.close();
    }
    
    cout << "RBF Test Case 9 Finished! The result will be examined." << endl << endl;
    
    return 0;
}

int RBFTest_10(RecordBasedFileManager *rbfm) {
    // Functions tested
    // 1. Open Record-Based File
    // 2. Read Multiple Records
    // 3. Close Record-Based File
    // 4. Destroy Record-Based File
    cout << endl << "***** In RBF Test Case 10 *****" << endl;
    
    RC rc;
    string fileName = "test9";
    
    // Open the file "test9"
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    int numRecords = 2000;
    void *record = malloc(1000);
    void *returnedData = malloc(1000);
    
    vector<Attribute> recordDescriptor;
    createLargeRecordDescriptor(recordDescriptor);
    
    vector<RID> rids;
    vector<int> sizes;
    RID tempRID;
    
    // Read rids from the disk - do not use this code in your codebase. This is not a PAGE-BASED operation - for the test purpose only.
    ifstream ridsFileRead("test9rids", ios::in | ios::binary);
    
    unsigned pageNum;
    unsigned slotNum;
    
    if (ridsFileRead.is_open()) {
        ridsFileRead.seekg(0,ios::beg);
        for (int i = 0; i < numRecords; i++) {
            ridsFileRead.read(reinterpret_cast<char*>(&pageNum), sizeof(unsigned));
            ridsFileRead.read(reinterpret_cast<char*>(&slotNum), sizeof(unsigned));
            if (i % 1000 == 0) {
                cout << "loaded RID #" << i << ": " << pageNum << ", " << slotNum << endl;
            }
            tempRID.pageNum = pageNum;
            tempRID.slotNum = slotNum;
            rids.push_back(tempRID);
        }
        ridsFileRead.close();
    }
    
    assert(rids.size() == (unsigned) numRecords && "Reading records should not fail.");
    
    // Read sizes vector from the disk - do not use this code in your codebase. This is not a PAGE-BASED operation - for the test purpose only.
    ifstream sizesFileRead("test9sizes", ios::in | ios::binary);
    
    int tempSize;
    
    if (sizesFileRead.is_open()) {
        sizesFileRead.seekg(0,ios::beg);
        for (int i = 0; i < numRecords; i++) {
            sizesFileRead.read(reinterpret_cast<char*>(&tempSize), sizeof(int));
            if (i % 1000 == 0) {
                cout << "loaded Sizes #" << i << ": " << tempSize << endl;
            }
            sizes.push_back(tempSize);
        }
        sizesFileRead.close();
    }
    
    assert(sizes.size() == (unsigned) numRecords && "Reading records should not fail.");
    
    // NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptor.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    for(int i = 0; i < numRecords; i++)
    {
        memset(record, 0, 1000);
        memset(returnedData, 0, 1000);
        rc = rbfm->readRecord(fileHandle, recordDescriptor, rids[i], returnedData);
        assert(rc == success && "Reading a record should not fail.");
        
        if (i % 1000 == 0) {
            cout << endl << "Returned Data:" << endl;
            rbfm->printRecord(recordDescriptor, returnedData);
        }
        
        
        int size = 0;
        prepareLargeRecord(recordDescriptor.size(), nullsIndicator, i, record, &size);
        
        if (i % 1000 == 0) {
            cout << endl << "Prepared record:" << endl;
            rbfm->printRecord(recordDescriptor, record);
        }
        /*
         cout << sizes[i] << endl;
         cout << "--------------------------------------------" << endl;
         print_bytes(record, sizes[i]);
         cout << "--------------------------------------------" << endl;
         print_bytes(returnedData, sizes[i]);
         */
        if(memcmp(returnedData, record, sizes[i]) != 0)
        {
            cout << "[FAIL] Test Case 10 Failed!" << endl << endl;
            free(record);
            free(returnedData);
            return -1;
        }
    }
    
    cout << endl;
    
    // Close the file "test9"
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    rc = rbfm->destroyFile(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName);
    assert(rc == success  && "Destroying the file should not fail.");
    
    free(record);
    free(returnedData);
    
    cout << "RBF Test Case 10 Finished! The result will be examined." << endl << endl;
    
    remove("test9sizes");
    remove("test9rids");
    
    return 0;
}

int RBFTest_11(RecordBasedFileManager *rbfm) {
    // Functions tested
    // 1. Create Record-Based File
    // 2. Open Record-Based File
    // 3. Insert Multiple Records
    // 4. Close Record-Based File
    cout << endl << "***** In RBF Test Case 11 *****" << endl;
    
    RC rc;
    string fileName = "test11";
    
    // Create a file named "test11"
    rc = rbfm->createFile(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    // Open the file "test11"
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    RID rid;
    void *record = malloc(1000);
    void *returnedData = malloc(1000);
    int numRecords = 10000;
    
    vector<Attribute> recordDescriptor;
    createLargeRecordDescriptor2(recordDescriptor);
    
    for (unsigned i = 0; i < recordDescriptor.size(); i++) {
        cout << "Attr Name: " << recordDescriptor[i].name << " Attr Type: " << (AttrType)recordDescriptor[i].type << " Attr Len: " << recordDescriptor[i].length << endl;
    }
    
    // NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptor.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    vector<RID> rids;
    // Insert 10000 records into file
    for (int i = 0; i < numRecords; i++) {
        // Test insert Record
        memset(record, 0, 1000);
        int size = 0;
        prepareLargeRecord2(recordDescriptor.size(), nullsIndicator, i, record, &size);
        
        rc = rbfm->insertRecord(fileHandle, recordDescriptor, record, rid);
        assert(rc == success && "Inserting a record should not fail.");
        
        rids.push_back(rid);
    }
    
    // Close the file
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    free(record);
    free(returnedData);
    
    assert(rids.size() == (unsigned) numRecords && "Inserting records should not fail.");
    
    // Write RIDs to the disk. Do not use this code in your codebase. This is not a page-based operation - for the test purpose only.
    ofstream ridsFile("test11rids", ios::out | ios::trunc | ios::binary);
    
    if (ridsFile.is_open()) {
        ridsFile.seekp(0, ios::beg);
        for (int i = 0; i < numRecords; i++) {
            ridsFile.write(reinterpret_cast<const char*>(&rids[i].pageNum),
                           sizeof(unsigned));
            ridsFile.write(reinterpret_cast<const char*>(&rids[i].slotNum),
                           sizeof(unsigned));
            if (i % 1000 == 0) {
                cout << "RID #" << i << ": " << rids[i].pageNum << ", "
                << rids[i].slotNum << endl;
            }
        }
        ridsFile.close();
    }
    
    cout << "RBF Test Case 11 Finished! The result will be examined." << endl << endl;
    
    return 0;
}

int RBFTest_12(RecordBasedFileManager *rbfm) {
    // Functions tested
    // 1. Open Record-Based File
    // 2. Read Multiple Records
    // 3. Close Record-Based File
    cout << endl << "***** In RBF Test Case 12 *****" << endl;
    
    RC rc;
    string fileName = "test11";
    
    // Open the file "test11"
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    void *record = malloc(1000);
    void *returnedData = malloc(1000);
    int numRecords = 10000;
    
    vector<Attribute> recordDescriptor;
    createLargeRecordDescriptor2(recordDescriptor);
    
    for (unsigned i = 0; i < recordDescriptor.size(); i++) {
        cout << "Attr Name: " << recordDescriptor[i].name << " Attr Type: " << (AttrType)recordDescriptor[i].type << " Attr Len: " << recordDescriptor[i].length << endl;
    }
    
    vector<RID> rids;
    RID tempRID;
    
    // Read rids from the disk - do not use this code. This is not a page-based operation. For test purpose only.
    ifstream ridsFileRead("test11rids", ios::in | ios::binary);
    
    unsigned pageNum;
    unsigned slotNum;
    
    if (ridsFileRead.is_open()) {
        ridsFileRead.seekg(0,ios::beg);
        for (int i = 0; i < numRecords; i++) {
            ridsFileRead.read(reinterpret_cast<char*>(&pageNum), sizeof(unsigned));
            ridsFileRead.read(reinterpret_cast<char*>(&slotNum), sizeof(unsigned));
            if (i % 1000 == 0) {
                cout << "loaded RID #" << i << ": " << pageNum << ", " << slotNum << endl;
            }
            tempRID.pageNum = pageNum;
            tempRID.slotNum = slotNum;
            rids.push_back(tempRID);
        }
        ridsFileRead.close();
    }
    
    assert(rids.size() == (unsigned) numRecords && "Reading records should not fail.");
    
    // NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptor.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    // Compare records from the disk read with the record created from the method
    for (int i = 0; i < numRecords; i++) {
        memset(record, 0, 1000);
        memset(returnedData, 0, 1000);
        rc = rbfm->readRecord(fileHandle, recordDescriptor, rids[i],
                              returnedData);
        if (rc != success) {
            return -1;
        }
        assert(rc == success);
        
        int size = 0;
        prepareLargeRecord2(recordDescriptor.size(), nullsIndicator, i, record, &size);
        if (memcmp(returnedData, record, size) != 0) {
            cout << "Test Case 12 Failed!" << endl << endl;
            free(record);
            free(returnedData);
            return -1;
        }
    }
    
    
    // Close the file
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    free(record);
    free(returnedData);
    
    cout << "RBF Test Case 12 Finished! The result will be examined." << endl << endl;
    
    return 0;
}

int RBFTest_Delete(RecordBasedFileManager *rbfm)
{
    // Functions tested
    // 1. Create Record-Based File
    // 2. Open Record-Based File
    // 3. Insert Record (3)
    // 4. Delete Record (1)
    // 5. Read Record
    // 6. Close Record-Based File
    // 7. Destroy Record-Based File
    cout << endl << "***** In RBF Test Case Delete *****" << endl;
    
    RC rc;
    string fileName = "test_delete";
    
    rc = rbfm->createFile(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    // Open the file
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    RID rid;
    int recordSize = 0;
    void *record = malloc(100);
    void *returnedData = malloc(100);
    
    vector<Attribute> recordDescriptor;
    createRecordDescriptor(recordDescriptor);
    
    // Initialize a NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptor.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    // Insert a record into a file and print the record
    nullsIndicator[0] = 128;
    prepareRecord(recordDescriptor.size(), nullsIndicator, 8, "", 25, 177.8, 6200, record,
                  &recordSize);
    cout << endl << "Inserting Data:" << endl;
    rbfm->printRecord(recordDescriptor, record);
    
    rc = rbfm->insertRecord(fileHandle, recordDescriptor, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    
    cout << endl;
    
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    // Insert a record into a file and print the record
    
    nullsIndicator[0] = 128;
    prepareRecord(recordDescriptor.size(), nullsIndicator, 8, "", 25, 177.8, 6200, record,
                  &recordSize);
    cout << endl << "Inserting Data:" << endl;
    rbfm->printRecord(recordDescriptor, record);
    
    rc = rbfm->insertRecord(fileHandle, recordDescriptor, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    
    cout << endl << "Inserting Data:" << endl;
    rbfm->printRecord(recordDescriptor, record);
    
    rc = rbfm->insertRecord(fileHandle, recordDescriptor, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    
    cout << endl << "Inserting Data:" << endl;
    rbfm->printRecord(recordDescriptor, record);
    
    rc = rbfm->insertRecord(fileHandle, recordDescriptor, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    
    unsigned slotNum = rid.slotNum;
    rid.slotNum = 0;
    
    rc = rbfm->deleteRecord(fileHandle, recordDescriptor, rid);
    assert(rc == success && "Deleting a record should not fail.");
    
    rc = rbfm->readRecord(fileHandle, recordDescriptor, rid, returnedData);
    assert(rc != success && "Reading a deleted record should fail.");
    
    rid.slotNum = slotNum - 1;
    
    // Given the rid, read the record from file
    rc = rbfm->readRecord(fileHandle, recordDescriptor, rid, returnedData);
    assert(rc == success && "Reading a record should not fail.");
    
    cout << endl << "Returned Data:" << endl;
    rbfm->printRecord(recordDescriptor, returnedData);
    
    // Compare whether the two memory blocks are the same
    if (memcmp(record, returnedData, recordSize) != 0)
    {
        cout << "[FAIL] Test Case Delete Failed!" << endl << endl;
        free(record);
        free(returnedData);
        return -1;
    }
    
    rc = rbfm->insertRecord(fileHandle, recordDescriptor, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    assert(rid.slotNum == 0 && "Inserted record should use previous deleted slot.");
    
    // Given the rid, read the record from file
    rc = rbfm->readRecord(fileHandle, recordDescriptor, rid, returnedData);
    assert(rc == success && "Reading a record should not fail.");
    
    cout << endl << "Returned Data:" << endl;
    rbfm->printRecord(recordDescriptor, returnedData);
    
    // Compare whether the two memory blocks are the same
    if (memcmp(record, returnedData, recordSize) != 0)
    {
        cout << "[FAIL] Test Case Delete Failed!" << endl << endl;
        free(record);
        free(returnedData);
        return -1;
    }
    
    cout << endl;
    
    // Close the file
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    // Destroy the file
    rc = rbfm->destroyFile(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    free(record);
    free(returnedData);
    
    cout << "RBF Test Case Delete Finished! The result will be examined." << endl << endl;
    
    return 0;
}

// ------------------ the following update might be a bit ugly ---------------------------------

void *record = malloc(2000);
void *returnedData = malloc(2000);
vector<Attribute> recordDescriptor;
unsigned char *nullsIndicator = NULL;
FileHandle fileHandle;

void readRecord(RecordBasedFileManager *rbfm, const RID& rid, string str)
{
    int recordSize;
    prepareRecord(recordDescriptor.size(), nullsIndicator, str.length(), str, 25, 177.8, 6200,
                  record, &recordSize);
    
    RC rc = rbfm->readRecord(fileHandle, recordDescriptor, rid, returnedData);
    assert(rc == success && "Reading a record should not fail.");
    
    // Compare whether the two memory blocks are the same
    assert(memcmp(record, returnedData, recordSize) == 0 && "Returned Data should be the same");
}

void insertRecord(RecordBasedFileManager *rbfm, RID& rid, string str)
{
    int recordSize;
    prepareRecord(recordDescriptor.size(), nullsIndicator, str.length(), str, 25, 177.8, 6200,
                  record, &recordSize);
    
    RC rc = rbfm->insertRecord(fileHandle, recordDescriptor, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    
}

void updateRecord(RecordBasedFileManager *rbfm, RID& rid, string str)
{
    int recordSize;
    prepareRecord(recordDescriptor.size(), nullsIndicator, str.length(), str, 25, 177.8, 6200,
                  record, &recordSize);
    
    RC rc = rbfm->updateRecord(fileHandle, recordDescriptor, record, rid);
    assert(rc == success && "Updating a record should not fail.");
    
}

int RBFTest_Update(RecordBasedFileManager *rbfm)
{
    // Functions tested
    // 1. Create Record-Based File
    // 2. Open Record-Based File
    // 3. Insert Record
    // 4. Read Record
    // 5. Close Record-Based File
    // 6. Destroy Record-Based File
    cout << endl << "***** In RBF Test Case Update *****" << endl;
    
    RC rc;
    string fileName = "test_update";
    
    // Create a file
    rc = rbfm->createFile(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    // Open the file
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    RID rid;
    createRecordDescriptor(recordDescriptor);
    
    string longstr;
    for (int i = 0; i < 1000; i++)
    {
        longstr.push_back('a');
    }
    
    string shortstr;
    for (int i = 0; i < 10; i++)
    {
        shortstr.push_back('s');
    }
    
    string midstr;
    for (int i = 0; i < 100; i++)
    {
        midstr.push_back('m');
    }
    
    // Initialize a NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptor.size());
    nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    // Insert short record
    insertRecord(rbfm, rid, shortstr);
    RID shortRID = rid;
    
    // Insert mid record
    insertRecord(rbfm, rid, midstr);
    RID midRID = rid;
    
    // Insert long record
    insertRecord(rbfm, rid, longstr);
    RID longRID = rid;
    
    // update short record
    updateRecord(rbfm, shortRID, midstr);
    
    //read updated short record and verify its content
    readRecord(rbfm, shortRID, midstr);
    
    // insert two more records
    insertRecord(rbfm, rid, longstr);
    insertRecord(rbfm, rid, longstr);
    
    // read mid record and verify its content
    readRecord(rbfm, midRID, midstr);
    
    // update short record
    updateRecord(rbfm, shortRID, longstr);
    
    // read the short record and verify its content
    readRecord(rbfm, shortRID, longstr);
    
    // delete the short record
    rbfm->deleteRecord(fileHandle, recordDescriptor, shortRID);
    
    // verify the short record has been deleted
    rc = rbfm->readRecord(fileHandle, recordDescriptor, shortRID, returnedData);
    
    assert(rc != success && "Read a deleted record should not success.");
    
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    // Destroy the file
    rc = rbfm->destroyFile(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    free(record);
    free(returnedData);
    
    cout << "RBF Test Case Update Finished! The result will be examined." << endl << endl;
    
    return 0;
}

// ------------------------------------------------------------------------------------------

int main() {
  
    PagedFileManager *pfm = PagedFileManager::instance();
    
    RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();
    
    RBFTest_1(pfm);
    
    RBFTest_2(pfm);
    
    RBFTest_3(pfm);
    
    RBFTest_4(pfm);
    
    RBFTest_5(pfm);
    
    RBFTest_6(pfm);
    
    RBFTest_7(pfm);
    
    RBFTest_8(rbfm);
    
    RBFTest_8b(rbfm);
    
    vector<RID> rids;
    vector<int> sizes;
    RBFTest_9(rbfm, rids, sizes);
    
    RBFTest_10(rbfm);
    
    RBFTest_11(rbfm);
    
    RBFTest_12(rbfm);
    
    RBFTest_Delete(rbfm);
    
    RBFTest_Update(rbfm);
    
    return 0;
}
