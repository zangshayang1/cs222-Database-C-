#include "../FileManager/test_util.h"

int RBFTest_private_0(RecordBasedFileManager *rbfm) {
    // Checks whether VarChar is implemented correctly or not.
    //
    // Functions tested
    // 1. Create Two Record-Based File
    // 2. Open Two Record-Based File
    // 3. Insert Multiple Records Into Two files
    // 4. Close Two Record-Based File
    // 5. Compare The File Sizes
    // 6. Destroy Two Record-Based File
    cout << endl << "***** In RBF Test Case Private 0 *****" << endl;
    
    RC rc;
    string fileName1 = "test_private_0a";
    string fileName2 = "test_private_0b";
    
    // Create a file named "test_private_0a"
    rc = rbfm->createFile(fileName1);
    assert(rc == success && "Creating a file should not fail.");
    
    rc = createFileShouldSucceed(fileName1);
    assert(rc == success && "Creating a file failed.");
    
    // Create a file named "test_private_0b"
    rc = rbfm->createFile(fileName2);
    assert(rc == success && "Creating a file should not fail.");
    
    rc = createFileShouldSucceed(fileName2);
    assert(rc == success && "Creating a file failed.");
    
    // Open the file "test_private_0a"
    FileHandle fileHandle1;
    rc = rbfm->openFile(fileName1, fileHandle1);
    assert(rc == success && "Opening a file should not fail.");
    
    // Open the file "test_private_0b"
    FileHandle fileHandle2;
    rc = rbfm->openFile(fileName2, fileHandle2);
    assert(rc == success && "Opening a file should not fail.");
    
    RID rid;
    void *record = malloc(3000);
    int numRecords = 5000;
    
    // Each varchar field length - 200
    vector<Attribute> recordDescriptor1;
    createRecordDescriptorForTwitterUser(recordDescriptor1);
    
    // NULL field indicator
    int nullFieldsIndicatorActualSize1 = getActualByteForNullsIndicator(recordDescriptor1.size());
    unsigned char *nullsIndicator1 = (unsigned char *) malloc(nullFieldsIndicatorActualSize1);
    memset(nullsIndicator1, 0, nullFieldsIndicatorActualSize1);
    
    // Each varchar field length - 800
    vector<Attribute> recordDescriptor2;
    createRecordDescriptorForTwitterUser2(recordDescriptor2);
    
    bool equalSizes = false;
    
    // Insert 5000 records into file
    for (int i = 0; i < numRecords; i++) {
        // Test insert Record
        int size = 0;
        memset(record, 0, 3000);
        prepareLargeRecordForTwitterUser(recordDescriptor1.size(), nullsIndicator1, i, record, &size);
        
        rc = rbfm->insertRecord(fileHandle1, recordDescriptor1, record, rid);
        assert(rc == success && "Inserting a record should not fail.");
        
        rc = rbfm->insertRecord(fileHandle2, recordDescriptor2, record, rid);
        assert(rc == success && "Inserting a record should not fail.");
        
        if (i%1000 == 0 && i != 0) {
            cout << i << " / " << numRecords << "records are inserted." << endl;
            compareFileSizes(fileName1, fileName2);
        }
    }
    // Close the file "test_private_0a"
    rc = rbfm->closeFile(fileHandle1);
    assert(rc == success && "Closing a file should not fail.");
    
    // Close the file "test_private_0b"
    rc = rbfm->closeFile(fileHandle2);
    assert(rc == success && "Closing a file should not fail.");
    
    free(record);
    
    cout << endl;
    equalSizes = compareFileSizes(fileName1, fileName2);
    
    rc = rbfm->destroyFile(fileName1);
    assert(rc == success && "Destroying a file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName1);
    assert(rc == success  && "Destroying the file should not fail.");
    
    rc = rbfm->destroyFile(fileName2);
    assert(rc == success && "Destroying a file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName1);
    assert(rc == success  && "Destroying the file should not fail.");
    
    if (!equalSizes) {
        cout << "Variable length Record is not properly implemented." << endl;
        cout << "**** [FAIL] RBF Test Private 0 failed. Two files are of different sizes. *****" << endl;
        return -1;
    }
    
    cout << "***** RBF Test Case Private 0 Finished. The result will be examined! *****" << endl << endl;
    
    return 0;
}

int RBFTest_private_1(RecordBasedFileManager *rbfm) {
    // Functions Tested:
    // 1. Create File - RBFM
    // 2. Open File
    // 3. insertRecord() - with an empty string field (not NULL)
    // 4. Close File
    // 5. Destroy File
    cout << "***** In RBF Test Case Private 1 *****" << endl;
    
    RC rc;
    string fileName = "test_private_1";
    
    // Create a file named "test_private_1"
    rc = rbfm->createFile(fileName);
    assert(rc == success && "Creating a file should not fail.");
    
    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating a file should not fail.");
    
    // Open the file "test_private_1"
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening a file should not fail.");
    
    RID rid;
    int recordSize = 0;
    void *record = malloc(2000);
    void *returnedData = malloc(2000);
    
    vector<Attribute> recordDescriptorForTweetMessage;
    createRecordDescriptorForTweetMessage(recordDescriptorForTweetMessage);
    
    // NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptorForTweetMessage.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    // Insert a record into a file - referred_topics is an empty string - "", not null value.
    prepareRecordForTweetMessage(recordDescriptorForTweetMessage.size(), nullsIndicator, 101, 1, 0, "", 31, "Finding shortcut_menu was easy.", 123.4, 1013.45, record, &recordSize);
    
    // An empty string should be printed for the referred_topics field.
    
    rc = rbfm->insertRecord(fileHandle, recordDescriptorForTweetMessage, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    
    // Given the rid, read the record from file
    rc = rbfm->readRecord(fileHandle, recordDescriptorForTweetMessage, rid, returnedData);
    assert(rc == success && "Reading a record should not fail.");
    
    // An empty string should be printed for the referred_topics field.
    
    // Compare whether the two memory blocks are the same
    if(memcmp(record, returnedData, recordSize) != 0)
    {
        cout << "***** [FAIL] Test Case Private 1 Failed! *****" << endl << endl;
        free(record);
        free(returnedData);
        return -1;
    }
    
    // Close the file "test_private_1"
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    int fsize = getFileSize(fileName);
    if (fsize == 0) {
        cout << "File Size should not be zero at this moment." << endl;
        rc = rbfm->destroyFile(fileName);
        free(record);
        free(returnedData);
        return -1;
    }
    
    // Destroy File
    rc = rbfm->destroyFile(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName);
    assert(rc == success  && "Destroying the file should not fail.");
    
    free(record);
    free(returnedData);
    
    cout << "***** RBF Test Case Private 1 Finished. The result will be examined! *****" << endl << endl;
    
    return 0;
}

int RBFtest_private_1b(RecordBasedFileManager *rbfm) {
    // Functions Tested:
    // 1. Create File - RBFM
    // 2. Open File
    // 3. insertRecord() - with two consecutive NULLs
    // 4. Close File
    // 5. Destroy File
    cout << "***** In RBF Test Case Private 1b *****" << endl;
    
    RC rc;
    string fileName = "test_private_1b";
    
    // Create a file named "test_private_1b"
    rc = rbfm->createFile(fileName);
    assert(rc == success && "Creating a file should not fail.");
    
    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating a file should not fail.");
    
    // Open the file "test_private_1b"
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening a file should not fail.");
    
    RID rid;
    int recordSize = 0;
    void *record = malloc(2000);
    void *returnedData = malloc(2000);
    
    vector<Attribute> recordDescriptorForTweetMessage;
    createRecordDescriptorForTweetMessage(recordDescriptorForTweetMessage);
    
    // NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptorForTweetMessage.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    // Setting the message_text(4th) and sender_location(5th) field values as null
    nullsIndicator[0] = 20; // 00011000
    
    // Insert a record into a file
    prepareRecordForTweetMessage(recordDescriptorForTweetMessage.size(), nullsIndicator, 101, 1, 8, "database", 31, "Finding shortcut_menu was easy.", 123.4, 1013.45, record, &recordSize);
    
    rc = rbfm->insertRecord(fileHandle, recordDescriptorForTweetMessage, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    
    // Given the rid, read the record from file
    rc = rbfm->readRecord(fileHandle, recordDescriptorForTweetMessage, rid, returnedData);
    assert(rc == success && "Reading a record should not fail.");
    
    // Compare whether the two memory blocks are the same
    if(memcmp(record, returnedData, recordSize) != 0)
    {
        cout << "***** [FAIL] Test Case Private 1b Failed! *****" << endl << endl;
        free(record);
        free(returnedData);
        return -1;
    }
    
    // Close the file "test_private_1b"
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    cout << endl;
    int fsize = getFileSize(fileName);
    if (fsize == 0) {
        cout << "File Size should not be zero at this moment." << endl;
        rc = rbfm->destroyFile(fileName);
        free(record);
        free(returnedData);
        return -1;
    }
    
    // Destroy File
    rc = rbfm->destroyFile(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName);
    assert(rc == success  && "Destroying the file should not fail.");
    
    free(record);
    free(returnedData);
    
    cout << "***** RBF Test Case Private 1b Finished. The result will be examined! *****" << endl << endl;
    
    return 0;
}


int RBFtest_private_1c(RecordBasedFileManager *rbfm) {
    // Functions Tested:
    // 1. Create File - RBFM
    // 2. Open File
    // 3. insertRecord() - with three NULLs
    // 4. Close File
    // 5. Destroy File
    cout << "***** In RBF Test Case Private 1c *****" << endl;
    
    RC rc;
    string fileName = "test_private_1c";
    
    // Create a file named "test_private_1c"
    rc = rbfm->createFile(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    // Open the file "test_private_1c"
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    RID rid;
    int recordSize = 0;
    void *record = malloc(2000);
    void *returnedData = malloc(2000);
    
    vector<Attribute> recordDescriptor;
    createLargeRecordDescriptor3(recordDescriptor);
    
    // NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptor.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    // Setting attr0, attr13, and attr20 as NULL.
    // Each of theses attributes is placed as the left-most bit in each byte.
    // The entire byte representation is: 100000000000010000001000
    //                                    123456789012345678901234
    nullsIndicator[0] = 128; // 10000000
    nullsIndicator[1] = 4; // 00000100
    nullsIndicator[2] = 8; // 00001000
    
    // Insert a record into a file
    prepareLargeRecord3(recordDescriptor.size(), nullsIndicator, 5, record, &recordSize);
    
    rc = rbfm->insertRecord(fileHandle, recordDescriptor, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    
    // Given the rid, read the record from file
    rc = rbfm->readRecord(fileHandle, recordDescriptor, rid, returnedData);
    assert(rc == success && "Reading a record should not fail.");
    
    // Compare whether the two memory blocks are the same
    if(memcmp(record, returnedData, recordSize) != 0)
    {
        cout << "***** [FAIL] Test Case Private 1c Failed! *****" << endl << endl;
        free(record);
        free(returnedData);
        rc = rbfm->closeFile(fileHandle);
        rc = rbfm->destroyFile(fileName);
        return -1;
    }
    
    // Close the file "test_private_1c"
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    int fsize = getFileSize(fileName);
    if (fsize == 0) {
        cout << "File Size should not be zero at this moment." << endl;
        return -1;
    }
    
    // Destroy File
    rc = rbfm->destroyFile(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName);
    assert(rc == success  && "Destroying the file should not fail.");
    
    free(record);
    free(returnedData);
    
    cout << "***** RBF Test Case Private 1c Finished. The result will be examined! *****" << endl << endl;
    
    return 0;
}

int RBFtest_private_2(RecordBasedFileManager *rbfm) {
    // Functions Tested:
    // 1. Create File - RBFM
    // 2. Open File
    // 3. insertRecord() - a big sized record so that two records cannot fit in a page.
    // 4. Close File
    // 5. Destroy File
    cout << endl << "***** In RBF Test Case Private 2 *****" << endl;
    
    RC rc;
    string fileName = "test_private_2";
    
    unsigned readPageCount = 0;
    unsigned writePageCount = 0;
    unsigned appendPageCount = 0;
    unsigned readPageCount1 = 0;
    unsigned writePageCount1 = 0;
    unsigned appendPageCount1 = 0;
    unsigned readPageCount2 = 0;
    unsigned writePageCount2 = 0;
    unsigned appendPageCount2 = 0;
    
    // Create a file named "test_private_2"
    rc = rbfm->createFile(fileName);
    assert(rc == success && "Creating a file should not fail.");
    
    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating a file failed.");
    
    // Open the file "test_private_2"
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    RID rid;
    int recordSize = 0;
    void *record = malloc(3000);
    void *returnedData = malloc(3000);
    
    vector<Attribute> recordDescriptor;
    createLargeRecordDescriptor4(recordDescriptor);
    
    // NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptor.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    // Insert a record into the file
    prepareLargeRecord4(recordDescriptor.size(), nullsIndicator, 2061, record, &recordSize);
    
    // collect before counters
    rc = fileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case Private 2 failed." << endl;
        rc = rbfm->closeFile(fileHandle);
        return -1;
    }
    
    rc = rbfm->insertRecord(fileHandle, recordDescriptor, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    
    // collect after counters - 1
    rc = fileHandle.collectCounterValues(readPageCount1, writePageCount1, appendPageCount1);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case Private 2 failed." << endl;
        rc = rbfm->closeFile(fileHandle);
        return -1;
    }
    
    // Insert the second record into the file
    memset(record, 0, 3000);
    prepareLargeRecord4(recordDescriptor.size(), nullsIndicator, 2060, record, &recordSize);
    
    rc = rbfm->insertRecord(fileHandle, recordDescriptor, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    
    // collect after counters - 2
    rc = fileHandle.collectCounterValues(readPageCount2, writePageCount2, appendPageCount2);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case Private 2 failed." << endl;
        rc = rbfm->closeFile(fileHandle);
        return -1;
    }
    
    // Check (appendPageCount2 > appendPageCount1 > appendPageCount)
    if (appendPageCount2 <= appendPageCount1 || appendPageCount1 <= appendPageCount) {
        cout << "The implementation regarding appendPage() is not correct." << endl;
        cout << "***** [FAIL] Test Case Private 2 Failed! *****" << endl;
    }
    
    // Given the rid, read the record from file
    rc = rbfm->readRecord(fileHandle, recordDescriptor, rid, returnedData);
    assert(rc == success && "Reading a record should not fail.");
    
    // Compare whether the two memory blocks are the same
    if(memcmp(record, returnedData, recordSize) != 0)
    {
        cout << "***** [FAIL] Test Case Private 2 Failed! *****" << endl << endl;
        free(record);
        free(returnedData);
        return -1;
    }
    
    // Close the file "test_private_2"
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    int fsize = getFileSize(fileName);
    if (fsize == 0) {
        cout << "File Size should not be zero at this moment." << endl;
        cout << "***** [FAIL] Test Case Private 2 Failed! *****" << endl << endl;
        return -1;
    }
    
    // Destroy File
    rc = rbfm->destroyFile(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName);
    assert(rc == success  && "Destroying the file should not fail.");
    
    free(record);
    free(returnedData);
    
    cout << "***** RBF Test Case Private 2 Finished. The result will be examined. *****" << endl << endl;
    
    return 0;
}

int RBFtest_private_2b(RecordBasedFileManager *rbfm) {
    // Functions Tested:
    // 1. Create File - RBFM
    // 2. Open File
    // 3. insertRecord() - checks if we can't find an enough space in the last page,
    //                     the system checks from the beginning of the file.
    // 4. Close File
    // 5. Destroy File
    cout << "***** In RBF Test Case Private 2b *****" << endl;
    
    RC rc;
    string fileName = "test_private_2b";
    
    unsigned readPageCount = 0;
    unsigned writePageCount = 0;
    unsigned appendPageCount = 0;
    unsigned readPageCount1 = 0;
    unsigned writePageCount1 = 0;
    unsigned appendPageCount1 = 0;
    unsigned readPageCountDiff = 0;
    unsigned writePageCountDiff = 0;
    unsigned appendPageCountDiff = 0;
    
    unsigned numberOfHeaderPages = 0;
    bool headerPageExists = false;
    
    // Create a file named "test_private_2b"
    rc = rbfm->createFile(fileName);
    assert(rc == success && "Creating a file should not fail.");
    
    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating a file failed.");
    
    // Open the file "test_private_2b"
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    // Get the initial number of pages in the file.
    // If it's greater than zero, we assume there is a directory.
    numberOfHeaderPages = fileHandle.getNumberOfPages();
    if (numberOfHeaderPages > 0) {
        headerPageExists = true;
    }
    
    if (headerPageExists) {
        cout << endl << "A header page exists." << endl;
    }
    
    RID rid;
    int recordSize = 0;
    void *record = malloc(3000);
    void *returnedData = malloc(3000);
    
    vector<Attribute> recordDescriptor;
    createLargeRecordDescriptor4(recordDescriptor);
    
    // NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptor.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    int numRecords = 50;
    
    // Insert 50 records into the file
    for (int i = 0; i < numRecords; i++) {
        memset(record, 0, 3000);
        prepareLargeRecord4(recordDescriptor.size(), nullsIndicator, 2060+i, record, &recordSize);
        
        rc = rbfm->insertRecord(fileHandle, recordDescriptor, record, rid);
        assert(rc == success && "Inserting a record should not fail.");
    }
    
    // Collect before counters before doing one more insert
    rc = fileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case Private 2b failed." << endl;
        rc = rbfm->closeFile(fileHandle);
        rc = rbfm->destroyFile(fileName);
        free(record);
        free(returnedData);
        return -1;
    }
    
    // One more insertion
    memset(record, 0, 3000);
    prepareLargeRecord4(recordDescriptor.size(), nullsIndicator, 2160, record, &recordSize);
    
    rc = rbfm->insertRecord(fileHandle, recordDescriptor, record, rid);
    assert(rc == success && "Inserting a record should not fail.");
    
    // Collect after counters
    rc = fileHandle.collectCounterValues(readPageCount1, writePageCount1, appendPageCount1);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case Private 2b failed." << endl;
        rc = rbfm->closeFile(fileHandle);
        rc = rbfm->destroyFile(fileName);
        free(record);
        free(returnedData);
        return -1;
    }
    
    // Calculate the counter differences
    readPageCountDiff = readPageCount1 - readPageCount;
    appendPageCountDiff = appendPageCount1 - appendPageCount;
    writePageCountDiff = writePageCount1 - writePageCount;
    
    // If a directory exists, then we need to read at least one page and append one page.
    // Also, we need to update the directory structure. So, we need to have one write.
    if (headerPageExists) {
        if (readPageCountDiff < 1 || appendPageCountDiff < 1 || writePageCount < 1) {
            cout << "The implementation regarding insertRecord() is not correct." << endl;
            cout << "***** [FAIL] Test Case Private 2b Failed! *****" << endl;
            rc = rbfm->closeFile(fileHandle);
            rc = rbfm->destroyFile(fileName);
            free(record);
            free(returnedData);
            return -1;
        }
    } else {
        // Each page can only contain one record. So, readPageCountDiff should be greater than 50
        // since the system needs to go through all pages from the beginning.
        if (readPageCountDiff < numRecords) {
            cout << "The implementation regarding insertRecord() is not correct." << endl;
            cout << "***** [FAIL] Test Case Private 2b Failed! *****" << endl;
            rc = rbfm->closeFile(fileHandle);
            rc = rbfm->destroyFile(fileName);
            free(record);
            free(returnedData);
            return -1;
        }
    }
    
    
    // Close the file "test_private_2b"
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing the file should not fail.");
    
    int fsize = getFileSize(fileName);
    if (fsize == 0) {
        cout << "File Size should not be zero at this moment." << endl;
        cout << "***** [FAIL] Test Case Private 2b Failed! *****" << endl << endl;
        rc = rbfm->destroyFile(fileName);
        free(record);
        free(returnedData);
        return -1;
    }
    
    // Destroy File
    rc = rbfm->destroyFile(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    rc = destroyFileShouldSucceed(fileName);
    assert(rc == success  && "Destroying the file should not fail.");
    
    free(record);
    free(returnedData);
    
    cout << "***** RBF Test Case Private 2b Finished. The result will be examined. *****" << endl << endl;
    
    return 0;
}

int RBFTest_private_3(RecordBasedFileManager *rbfm) {
    // Functions Tested:
    // 1. Create a File - test_private_3a
    // 2. Create a File - test_private_3b
    // 3. Open test_private_3a
    // 4. Open test_private_3b
    // 5. Insert 10000 records into test_private_3a
    // 6. Insert 10000 records into test_private_3b
    // 7. Close test_private_3a
    // 8. Close test_private_3b
    cout << endl << "***** In RBF Test Case Private 3 ****" << endl;
    
    RC rc;
    string fileName = "test_private_3a";
    string fileName2 = "test_private_3b";
    
    // Create a file named "test_private_3a"
    rc = rbfm->createFile(fileName);
    assert(rc == success && "Creating a file should not fail.");
    
    rc = createFileShouldSucceed(fileName);
    assert(rc == success && "Creating a file should not fail.");
    
    // Create a file named "test_private_3b"
    rc = rbfm->createFile(fileName2);
    assert(rc == success && "Creating a file should not fail.");
    
    rc = createFileShouldSucceed(fileName2);
    assert(rc == success && "Creating a file should not fail.");
    
    // Open the file "test_private_3a"
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    // Open the file "test_private_3b"
    FileHandle fileHandle2;
    rc = rbfm->openFile(fileName2, fileHandle2);
    assert(rc == success && "Opening the file should not fail.");
    
    RID rid, rid2;
    void *record = malloc(1000);
    void *record2 = malloc(1000);
    void *returnedData = malloc(1000);
    void *returnedData2 = malloc(1000);
    int numRecords = 10000;
    
    vector<Attribute> recordDescriptorForTwitterUser,
    recordDescriptorForTweetMessage;
    
    createRecordDescriptorForTwitterUser(recordDescriptorForTwitterUser);
    createRecordDescriptorForTweetMessage(recordDescriptorForTweetMessage);
    
    // NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptorForTwitterUser.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    int nullFieldsIndicatorActualSize2 = getActualByteForNullsIndicator(recordDescriptorForTweetMessage.size());
    unsigned char *nullsIndicator2 = (unsigned char *) malloc(nullFieldsIndicatorActualSize2);
    memset(nullsIndicator2, 0, nullFieldsIndicatorActualSize2);
    
    vector<RID> rids, rids2;
    // Insert 50,000 records into the file - test_private_3a and test_private_3b
    for (int i = 0; i < numRecords; i++) {
        memset(record, 0, 1000);
        int size = 0;
        
        memset(record2, 0, 1000);
        int size2 = 0;
        
        prepareLargeRecordForTwitterUser(recordDescriptorForTwitterUser.size(), nullsIndicator, i, record, &size);
        
        rc = rbfm->insertRecord(fileHandle, recordDescriptorForTwitterUser,
                                record, rid);
        assert(rc == success && "Inserting a record for the file #1 should not fail.");
        
        rids.push_back(rid);
        
        
        prepareLargeRecordForTweetMessage(recordDescriptorForTweetMessage.size(), nullsIndicator2, i, record2, &size2);
        
        rc = rbfm->insertRecord(fileHandle2, recordDescriptorForTweetMessage,
                                record2, rid2);
        assert(rc == success && "Inserting a record  for the file #2 should not fail.");
        
        rids2.push_back(rid2);
        
        if (i%5000 == 0 && i != 0) {
            cout << i << " / " << numRecords << " records inserted so far for both files." << endl;
        }
    }
    
    cout << "Inserting " << numRecords << " records done for the both files." << endl << endl;
    
    // Close the file - test_private_3a
    rc = rbfm->closeFile(fileHandle);
    if (rc != success) {
        return -1;
    }
    assert(rc == success);
    
    free(record);
    free(returnedData);
    
    if (rids.size() != numRecords) {
        return -1;
    }
    
    // Close the file - test_private_3b
    rc = rbfm->closeFile(fileHandle2);
    if (rc != success) {
        return -1;
    }
    assert(rc == success);
    
    free(record2);
    free(returnedData2);
    
    if (rids2.size() != numRecords) {
        return -1;
    }
    
    int fsize = getFileSize(fileName);
    if (fsize == 0) {
        cout << "File Size should not be zero at this moment." << endl;
        cout << "***** [FAIL] Test Case Private 3 Failed! *****" << endl << endl;
        return -1;
    }
    
    fsize = getFileSize(fileName2);
    if (fsize == 0) {
        cout << "File Size should not be zero at this moment." << endl;
        cout << "***** [FAIL] Test Case Private 3 Failed! *****" << endl << endl;
        return -1;
    }
    
    // Write RIDs of test_private_3a to a disk - do not use this code. This is not a page-based operation. For test purpose only.
    ofstream ridsFile("test_private_3a_rids", ios::out | ios::trunc | ios::binary);
    
    if (ridsFile.is_open()) {
        ridsFile.seekp(0, ios::beg);
        for (int i = 0; i < numRecords; i++) {
            ridsFile.write(reinterpret_cast<const char*>(&rids.at(i).pageNum),
                           sizeof(unsigned));
            ridsFile.write(reinterpret_cast<const char*>(&rids.at(i).slotNum),
                           sizeof(unsigned));
        }
        ridsFile.close();
        cout << endl << endl;
    }
    
    // Write RIDs of test_private_3b to a disk - do not use this code. This is not a page-based operation. For test purpose only.
    ofstream ridsFile2("test_private_3b_rids", ios::out | ios::trunc | ios::binary);
    
    if (ridsFile2.is_open()) {
        ridsFile2.seekp(0, ios::beg);
        for (int i = 0; i < numRecords; i++) {
            ridsFile2.write(reinterpret_cast<const char*>(&rids2.at(i).pageNum),
                            sizeof(unsigned));
            ridsFile2.write(reinterpret_cast<const char*>(&rids2.at(i).slotNum),
                            sizeof(unsigned));
        }
        ridsFile2.close();
    }
    
    cout << "***** RBF Test Case Private 3 Finished. The result will be examined. *****" << endl << endl;
    
    return 0;
}

int RBFTest_private_4(RecordBasedFileManager *rbfm) {
    // Functions Tested:
    // 1. Open the File created by test_private_3 - test_private_3a
    // 2. Read entire records - test_private_3a
    // 3. Check correctness
    // 4. Close the File - test_private_3a
    // 5. Destroy the File - test_private_3a
    // 6. Open the File created by test 14 - test_private_3b
    // 7. Read entire records - test_private_3b
    // 8. Check correctness
    // 9. Close the File - test_private_3b
    // 10. Destroy the File - test_private_3b
    
    cout << endl << "***** In RBF Test Case Private 4 *****" << endl;
    
    RC rc;
    string fileName = "test_private_3a";
    string fileName2 = "test_private_3b";
    
    // Open the file "test_private_3a"
    FileHandle fileHandle;
    rc = rbfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening a file should not fail.");
    
    void *record = malloc(1000);
    void *returnedData = malloc(1000);
    int numRecords = 5000;
    
    vector<Attribute> recordDescriptorForTwitterUser, recordDescriptorForTweetMessage;
    createRecordDescriptorForTwitterUser(recordDescriptorForTwitterUser);
    
    vector<RID> rids, rids2;
    RID tempRID, tempRID2;
    
    // Read rids from the disk - do not use this code. This is not a page-based operation. For test purpose only.
    ifstream ridsFileRead("test_private_3a_rids", ios::in | ios::binary);
    
    unsigned pageNum;
    unsigned slotNum;
    
    if (ridsFileRead.is_open()) {
        ridsFileRead.seekg(0,ios::beg);
        for (int i = 0; i < numRecords; i++) {
            ridsFileRead.read(reinterpret_cast<char*>(&pageNum), sizeof(unsigned));
            ridsFileRead.read(reinterpret_cast<char*>(&slotNum), sizeof(unsigned));
            tempRID.pageNum = pageNum;
            tempRID.slotNum = slotNum;
            rids.push_back(tempRID);
        }
        ridsFileRead.close();
    }
    
    if (rids.size() != numRecords) {
        return -1;
    }
    
    // NULL field indicator
    int nullFieldsIndicatorActualSize = getActualByteForNullsIndicator(recordDescriptorForTwitterUser.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
    memset(nullsIndicator, 0, nullFieldsIndicatorActualSize);
    
    // Compare records from the disk read with the record created from the method
    for (int i = 0; i < numRecords; i++) {
        memset(record, 0, 1000);
        memset(returnedData, 0, 1000);
        rc = rbfm->readRecord(fileHandle, recordDescriptorForTwitterUser, rids[i],
                              returnedData);
        if (rc != success) {
            return -1;
        }
        assert(rc == success);
        
        int size = 0;
        prepareLargeRecordForTwitterUser(recordDescriptorForTwitterUser.size(), nullsIndicator, i, record, &size);
        if (memcmp(returnedData, record, size) != 0) {
            cout << "***** [FAIL] Comparison failed - RBF Test Case Private 4 Failed! *****" << i << endl << endl;
            free(record);
            free(returnedData);
            return -1;
        }
    }
    
    // Close the file
    rc = rbfm->closeFile(fileHandle);
    assert(rc == success && "Closing a file should not fail.");
    
    int fsize = getFileSize(fileName);
    if (fsize == 0) {
        cout << "File Size should not be zero at this moment." << endl;
        cout << "***** [FAIL] RBF Test Case Private 4 Failed! *****" << endl << endl;
        return -1;
    }
    
    // Destroy File
    rc = rbfm->destroyFile(fileName);
    assert(rc == success && "Destroying a file should not fail.");
    
    
    // Open the file "test_private_3b"
    FileHandle fileHandle2;
    rc = rbfm->openFile(fileName2, fileHandle2);
    assert(rc == success && "Opening a file should not fail.");
    
    createRecordDescriptorForTweetMessage(recordDescriptorForTweetMessage);
    
    cout << endl;
    
    // NULL field indicator
    int nullFieldsIndicatorActualSize2 = getActualByteForNullsIndicator(recordDescriptorForTweetMessage.size());
    unsigned char *nullsIndicator2 = (unsigned char *) malloc(nullFieldsIndicatorActualSize2);
    memset(nullsIndicator2, 0, nullFieldsIndicatorActualSize2);
    
    // Read rids from the disk - do not use this code. This is not a page-based operation. For test purpose only.
    ifstream ridsFileRead2("test_private_3b_rids", ios::in | ios::binary);
    
    if (ridsFileRead2.is_open()) {
        ridsFileRead2.seekg(0,ios::beg);
        for (int i = 0; i < numRecords; i++) {
            ridsFileRead2.read(reinterpret_cast<char*>(&pageNum), sizeof(unsigned));
            ridsFileRead2.read(reinterpret_cast<char*>(&slotNum), sizeof(unsigned));
            tempRID2.pageNum = pageNum;
            tempRID2.slotNum = slotNum;
            rids2.push_back(tempRID2);
        }
        ridsFileRead2.close();
    }
    
    if (rids2.size() != numRecords) {
        return -1;
    }
    
    
    
    // Compare records from the disk read with the record created from the method
    for (int i = 0; i < numRecords; i++) {
        memset(record, 0, 1000);
        memset(returnedData, 0, 1000);
        rc = rbfm->readRecord(fileHandle2, recordDescriptorForTweetMessage, rids2[i],
                              returnedData);
        if (rc != success) {
            return -1;
        }
        assert(rc == success);
        
        int size = 0;
        prepareLargeRecordForTweetMessage(recordDescriptorForTweetMessage.size(), nullsIndicator2, i, record, &size);
        if (memcmp(returnedData, record, size) != 0) {
            cout << "***** [FAIL] Comparison failed - RBF Test Case Private 4 Failed! *****" << i << endl << endl;
            free(record);
            free(returnedData);
            return -1;
        }
    }
    
    // Close the file
    rc = rbfm->closeFile(fileHandle2);
    assert(rc == success && "Closing a file should not fail.");
    
    fsize = getFileSize(fileName2);
    if (fsize == 0) {
        cout << "File Size should not be zero at this moment." << endl;
        cout << "***** [FAIL] RBF Test Case Private 4 Failed! *****" << endl << endl;
        return -1;
    }
    
    // Destroy File
    rc = rbfm->destroyFile(fileName2);
    assert(rc == success && "Destroying a file should not fail.");
    
    free(record);
    free(returnedData);
    
    remove("test_private_3a_rids");
    remove("test_private_3b_rids");
    
    cout << "***** RBF Test Case Private 4 Finished. The result will be examined. *****" << endl << endl;
    
    return 0;
}

int RBFTest_p5(PagedFileManager *pfm)
{
    // Functions Tested:
    // 1. Open File
    // 2. Append Page
    // 3. Get Number Of Pages
    // 4. Get Counter Values
    // 5. Close File
    cout << "***** In RBF Test Case Private 5 *****" << endl;
    
    RC rc;
    string fileName = "testp5";
    
    unsigned readPageCount = 0;
    unsigned writePageCount = 0;
    unsigned appendPageCount = 0;
    unsigned readPageCount1 = 0;
    unsigned writePageCount1 = 0;
    unsigned appendPageCount1 = 0;
    
    rc = pfm->createFile(fileName);
    assert(rc == success && "Creating the file should not fail.");
    
    // Open the file "testp5"
    FileHandle fileHandle;
    rc = pfm->openFile(fileName, fileHandle);
    assert(rc == success && "Opening the file should not fail.");
    
    // Collect before counters
    rc = fileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case 13 failed." << endl;
        rc = pfm->closeFile(fileHandle);
        return -1;
    }
    
    // Append the first page read the first page write the first page append the second page
    void *data = malloc(PAGE_SIZE);
    void *read_buffer = malloc(PAGE_SIZE);
    for(unsigned i = 0; i < PAGE_SIZE; i++)
    {
        *((char *)data+i) = i % 96 + 30;
    }
    rc = fileHandle.appendPage(data);
    assert(rc == success && "Appending a page should not fail.");
    rc = fileHandle.readPage(0, read_buffer);
    assert(rc == success && "Reading a page should not fail.");
    for(unsigned i = 0; i < PAGE_SIZE; i++)
    {
        *((char *)data+i) = i % 96 + 30;
    }
    rc = fileHandle.writePage(0, data);
    assert(rc == success && "Writing a page should not fail.");
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
        cout << "[FAIL] collectCounterValues() failed. Test Case 13 failed." << endl;
        rc = pfm->closeFile(fileHandle);
        return -1;
    }
    assert(readPageCount1 - readPageCount == 1 && "Read counter should be correct.");
    assert(writePageCount1 - writePageCount == 1 && "Write counter should be correct.");
    assert(appendPageCount1 - appendPageCount == 2 && "Append counter should be correct.");
    assert(appendPageCount1 > appendPageCount && "The appendPageCount should have been increased.");
    
    // Get the number of pages
    unsigned count = fileHandle.getNumberOfPages();
    assert(count == (unsigned)2 && "The count should be one at this moment.");
    
    // Close the file "test3"
    rc = pfm->closeFile(fileHandle);
    
    assert(rc == success && "Closing the file should not fail.");
    
    // Open the file "test3"
    FileHandle fileHandle2;
    rc = pfm->openFile(fileName, fileHandle2);
    
    assert(rc == success && "Open the file should not fail.");
    
    // collect after counters
    rc = fileHandle2.collectCounterValues(readPageCount1, writePageCount1, appendPageCount1);
    if(rc != success)
    {
        cout << "[FAIL] collectCounterValues() failed. Test Case 13 failed." << endl;
        rc = pfm->closeFile(fileHandle);
        return -1;
    }
    assert(readPageCount1 - readPageCount == 1 && "Persistent read counter should be correct.");
    assert(writePageCount1 - writePageCount == 1 && "Persistent write counter should be correct.");
    assert(appendPageCount1 - appendPageCount == 2 && "Persistent append counter should be correct.");
    
    rc = pfm->closeFile(fileHandle2);
    
    assert(rc == success && "Closing the file should not fail.");
    
    rc = pfm->destroyFile(fileName);
    assert(rc == success && "Destroying the file should not fail.");
    
    free(data);
    free(read_buffer);
    
    cout << "RBF Test Case Private 5 Finished! The result will be examined." << endl << endl;
    
    return 0;
}

int main() {
    
    PagedFileManager *pfm = PagedFileManager::instance();
    RecordBasedFileManager *rbfm = RecordBasedFileManager::instance();
    
    RBFTest_private_0(rbfm);
    
    RBFTest_private_1(rbfm);
    
    RBFtest_private_1b(rbfm);
    
    RBFtest_private_1c(rbfm);
    
    RBFtest_private_2(rbfm);
    
    RBFtest_private_2b(rbfm);
    
    RBFTest_private_3(rbfm);
    
    RBFTest_private_4(rbfm);
    
    RBFTest_p5(pfm);
    
    return 0;
}
