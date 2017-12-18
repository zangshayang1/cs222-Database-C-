#include <iostream>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#include "../IndexManager/ix.h"
#include "../IndexManager/ix_test_util.h"

IndexManager *indexManager;

RC testCase_1(const string &indexFileName)
{
    // Functions tested
    // 1. Create Index File **
    // 2. Open Index File **
    // 3. Create Index File -- when index file is already created **
    // 4. Open Index File ** -- when a file handle is already opened **
    // 5. Close Index File **
    // NOTE: "**" signifies the new functions being tested in this test case.
    cerr << endl << "***** In IX Test Case 1 *****" << endl;
    
    // create index file
    RC rc = indexManager->createFile(indexFileName);
    assert(rc == success && "indexManager::createFile() should not fail.");
    
    // open index file
    IXFileHandle ixfileHandle;
    rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");
    
    // create duplicate index file
    rc = indexManager->createFile(indexFileName);
    assert(rc != success && "Calling indexManager::createFile() on an existing file should fail.");
    
    // open index file again using the file handle that is already opened.
    rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc != success && "Calling indexManager::openFile() using an already opened file handle should fail.");
    
    // close index file
    rc = indexManager->closeFile(ixfileHandle);
    assert(rc == success && "indexManager::closeFile() should not fail.");
    
    return success;
}

int testCase_2(const string &indexFileName, const Attribute &attribute)
{
    // Functions tested
    // 1. Open Index file
    // 2. Insert entry **
    // 3. Disk I/O check of Insertion - CollectCounterValues **
    // 4. print B+ Tree **
    // 5. Close Index file
    // NOTE: "**" signifies the new functions being tested in this test case.
    cerr << endl << "***** In IX Test Case 2 *****" << endl;
    
    RID rid;
    int key = 200;
    rid.pageNum = 500;
    rid.slotNum = 20;
    
    unsigned readPageCount = 0;
    unsigned writePageCount = 0;
    unsigned appendPageCount = 0;
    unsigned readPageCountAfter = 0;
    unsigned writePageCountAfter = 0;
    unsigned appendPageCountAfter = 0;
    unsigned readDiff = 0;
    unsigned writeDiff = 0;
    unsigned appendDiff = 0;
    
    // open index file
    IXFileHandle ixfileHandle;
    RC rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");
    
    // collect counters
    rc = ixfileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
    assert(rc == success && "indexManager::collectCounterValues() should not fail.");
    
    cerr << endl << "Before Insert - R W A: " << readPageCount << " " <<  writePageCount << " " << appendPageCount << endl;
    
    // insert entry
    rc = indexManager->insertEntry(ixfileHandle, attribute, &key, rid);
    assert(rc == success && "indexManager::insertEntry() should not fail.");
    
    // collect counters
    rc = ixfileHandle.collectCounterValues(readPageCountAfter, writePageCountAfter, appendPageCountAfter);
    assert(rc == success && "indexManager::collectCounterValues() should not fail.");
    
    cerr << "After Insert - R W A: " << readPageCountAfter << " " <<  writePageCountAfter << " " << appendPageCountAfter << endl;
    
    readDiff = readPageCountAfter - readPageCount;
    writeDiff = writePageCountAfter - writePageCount;
    appendDiff = appendPageCountAfter - appendPageCount;
    
    cerr << "Page I/O count of single insertion - R W A: " << readDiff << " " << writeDiff << " " << appendDiff << endl;
    
    if (readDiff == 0 && writeDiff == 0 && appendDiff == 0) {
        cerr << "Insertion should generate some page I/O. The implementation is not correct." << endl;
        rc = indexManager->closeFile(ixfileHandle);
        return fail;
    }
    
    // print BTree, by this time the BTree should have only one node
    cerr << endl;
    indexManager->printBtree(ixfileHandle, attribute);
    
    // close index file
    rc = indexManager->closeFile(ixfileHandle);
    assert(rc == success && "indexManager::closeFile() should not fail.");
    
    return success;
}

int testCase_3(const string &indexFileName, const Attribute &attribute)
{
    // Functions tested
    // 1. Open Index file
    // 2. Disk I/O check of Scan and getNextEntry - CollectCounterValues **
    // 3. Close Index file
    // NOTE: "**" signifies the new functions being tested in this test case.
    cerr << endl << "***** In IX Test Case 3 *****" << endl;
    
    RID rid;
    int key = 200;
    rid.pageNum = 500;
    rid.slotNum = 20;
    
    unsigned readPageCount = 0;
    unsigned writePageCount = 0;
    unsigned appendPageCount = 0;
    unsigned readPageCountAfter = 0;
    unsigned writePageCountAfter = 0;
    unsigned appendPageCountAfter = 0;
    unsigned readDiff = 0;
    unsigned writeDiff = 0;
    unsigned appendDiff = 0;
    
    IX_ScanIterator ix_ScanIterator;
    
    // open index file
    IXFileHandle ixfileHandle;
    RC rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");
    
    // collect counters
    rc = ixfileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
    assert(rc == success && "indexManager::collectCounterValues() should not fail.");
    
    cerr << "Before scan - R W A: " << readPageCount << " " << writePageCount << " " << appendPageCount << endl;
    
    // Conduct a scan
    rc = indexManager->scan(ixfileHandle, attribute, NULL, NULL, true, true, ix_ScanIterator);
    assert(rc == success && "indexManager::scan() should not fail.");
    
    // There should be one record
    int count = 0;
    while(ix_ScanIterator.getNextEntry(rid, &key) == success)
    {
        cerr << "Returned rid from a scan: " << rid.pageNum << " " << rid.slotNum << endl;
        assert(rid.pageNum == 500 && "rid.pageNum is not correct.");
        assert(rid.slotNum == 20 && "rid.slotNum is not correct.");
        count++;
    }
    assert(count == 1 && "scan count is not correct.");
    
    // collect counters
    rc = ixfileHandle.collectCounterValues(readPageCountAfter, writePageCountAfter, appendPageCountAfter);
    assert(rc == success && "indexManager::collectCounterValues() should not fail.");
    
    cerr << "After scan - R W A: " << readPageCountAfter << " " << writePageCountAfter << " " << appendPageCountAfter << endl;
    
    readDiff = readPageCountAfter - readPageCount;
    writeDiff = writePageCountAfter - writePageCount;
    appendDiff = appendPageCountAfter - appendPageCount;
    
    cerr << "Page I/O count of scan - R W A: " << readDiff << " " << writeDiff << " " << appendDiff << endl;
    
    if (readDiff == 0 && writeDiff == 0 && appendDiff == 0) {
        cerr << "Scan should generate some page I/O. The implementation is not correct." << endl;
        rc = ix_ScanIterator.close();
        rc = indexManager->closeFile(ixfileHandle);
        return fail;
    }
    
    // Close Scan
    rc = ix_ScanIterator.close();
    assert(rc == success && "IX_ScanIterator::close() should not fail.");
    
    // Close index file
    rc = indexManager->closeFile(ixfileHandle);
    assert(rc == success && "indexManager::closeFile() should not fail.");
    
    return success;
    
}

int testCase_4(const string &indexFileName, const Attribute &attribute)
{
    // Functions tested
    // 1. Open Index file
    // 2. Disk I/O check of deleteEntry - CollectCounterValues **
    // 3. Close Index file
    // NOTE: "**" signifies the new functions being tested in this test case.
    cerr << endl << "***** In IX Test Case 4 *****" << endl;
    
    RID rid;
    int key = 200;
    rid.pageNum = 500;
    rid.slotNum = 20;
    
    unsigned readPageCount = 0;
    unsigned writePageCount = 0;
    unsigned appendPageCount = 0;
    unsigned readPageCountAfter = 0;
    unsigned writePageCountAfter = 0;
    unsigned appendPageCountAfter = 0;
    unsigned readDiff = 0;
    unsigned writeDiff = 0;
    unsigned appendDiff = 0;
    
    // open index file
    IXFileHandle ixfileHandle;
    RC rc = indexManager->openFile(indexFileName, ixfileHandle);
    assert(rc == success && "indexManager::openFile() should not fail.");
    
    // collect counters
    rc = ixfileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
    assert(rc == success && "indexManager::collectCounterValues() should not fail.");
    
    cerr << "Before DeleteEntry - R W A: " << readPageCount << " " << writePageCount << " " << appendPageCount << endl;
    
    // delete entry
    rc = indexManager->deleteEntry(ixfileHandle, attribute, &key, rid);
    assert(rc == success && "indexManager::deleteEntry() should not fail.");
    
    rc = ixfileHandle.collectCounterValues(readPageCountAfter, writePageCountAfter, appendPageCountAfter);
    assert(rc == success && "indexManager::collectCounterValues() should not fail.");
    
    cerr << "After DeleteEntry - R W A: " << readPageCountAfter << " " << writePageCountAfter << " " << appendPageCountAfter << endl;
    
    // collect counters
    readDiff = readPageCountAfter - readPageCount;
    writeDiff = writePageCountAfter - writePageCount;
    appendDiff = appendPageCountAfter - appendPageCount;
    
    cerr << "Page I/O count of single deletion - R W A: " << readDiff << " " << writeDiff << " " << appendDiff << endl;
    
    if (readDiff == 0 && writeDiff == 0 && appendDiff == 0) {
        cerr << "Deletion should generate some page I/O. The implementation is not correct." << endl;
        rc = indexManager->closeFile(ixfileHandle);
        return fail;
    }
    
    // delete entry again - should fail
    rc = indexManager->deleteEntry(ixfileHandle, attribute, &key, rid);
    assert(rc != success && "indexManager::deleteEntry() should fail.");
    
    // close index file
    rc = indexManager->closeFile(ixfileHandle);
    assert(rc == success && "indexManager::closeFile() should not fail.");
    
    return success;
    
}

int main()
{
    // Global Initialization
    indexManager = IndexManager::instance();
    RC rc;
    
    // test 1
    const string indexFileName = "age_idx";
    remove("age_idx");
    
    rc = testCase_1(indexFileName);
    if (rc == success) {
        cerr << "***** IX Test Case 1 finished. The result will be examined. *****" << endl;
    } else {
        cerr << "***** [FAIL] IX Test Case 1 failed. *****" << endl;
    }
    
    // test 2
    Attribute attrAge;
    attrAge.length = 4;
    attrAge.name = "age";
    attrAge.type = TypeInt;
    
    rc = testCase_2(indexFileName, attrAge);
    if (rc == success) {
        cerr << "***** IX Test Case 2 finished. The result will be examined. *****" << endl;
    } else {
        cerr << "***** [FAIL] IX Test Case 2 failed. *****" << endl;
    }
    
    // test 3
    rc = testCase_3(indexFileName, attrAge);
    if (rc == success) {
        cerr << "***** IX Test Case 3 finished. The result will be examined. *****" << endl;
    } else {
        cerr << "***** [FAIL] IX Test Case 3 failed. *****" << endl;
    }
    
    // test 4
    rc = testCase_4(indexFileName, attrAge);
    if (rc == success) {
        cerr << "***** IX Test Case 4 finished. The result will be examined. *****" << endl;
    } else {
        cerr << "***** [FAIL] IX Test Case 4 failed. *****" << endl;
    }
}


