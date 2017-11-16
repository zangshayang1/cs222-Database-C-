#include "../RelationManager/rm_test_util.h"

int deleteAll()
{
    // By executing this script, the following tables including the system tables will be removed.
    cout << endl << "***** RM TEST - Deleting the Catalog and User tables *****" << endl;
    
    RC rc = rm->deleteTable("tbl_employee");
    if (rc != 0) {
        cout << "Deleting tbl_employee failed." << endl;
    }
    
    rc = rm->deleteTable("tbl_employee2");
    if (rc != 0) {
        cout << "Deleting tbl_employee2 failed." << endl;
    }
    
    rc = rm->deleteTable("tbl_employee3");
    if (rc != 0) {
        cout << "Deleting tbl_employee3 failed." << endl;
    }
    
    rc = rm->deleteTable("tbl_employee4");
    if (rc != 0) {
        cout << "Deleting tbl_employee4 failed." << endl;
    }
    
    rc = rm->deleteCatalog();
    if (rc != 0) {
        cout << "Deleting the catalog failed." << endl;
        return rc;
    }
    
    return success;
}

int create()
{
    // By executing this script, the following tables including the system tables will be removed and constructed again.
    
    // Before executing rmtest_xx, you need to make sure that this script work properly.
    cout << endl << "***** RM TEST - Creating the Catalog and user tables *****" << endl;
    
    // Try to delete the System Catalog.
    // If this is the first time, it will generate an error. It's OK and we will ignore that.
    RC rc = rm->deleteCatalog();
    
    rc = rm->createCatalog();
    assert (rc == success && "Creating the Catalog should not fail.");
    
    // Delete the actual file and create Table tbl_employee
    remove("tbl_employee");
    
    rc = createTable("tbl_employee");
    assert (rc == success && "Creating a table should not fail.");
    
    // Delete the actual file and create Table tbl_employee
    remove("tbl_employee2");
    
    rc = createTable("tbl_employee2");
    assert (rc == success && "Creating a table should not fail.");
    
    // Delete the actual file and create Table tbl_employee
    remove("tbl_employee3");
    
    rc = createTable("tbl_employee3");
    assert (rc == success && "Creating a table should not fail.");
    
    // Delete the actual file and create Table tbl_employee
    remove("tbl_employee4");
    
    rc = createLargeTable("tbl_employee4");
    assert (rc == success && "Creating a table should not fail.");
    
    return success;
}

RC TEST_RM_0(const string &tableName)
{
    // Functions Tested
    // 1. getAttributes **
    cout << endl << "***** In RM Test Case 0 *****" << endl;
    
    // GetAttributes
    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    for(unsigned i = 0; i < attrs.size(); i++)
    {
        cout << (i+1) << ". Attr Name: " << attrs[i].name << " Type: " << (AttrType) attrs[i].type << " Len: " << attrs[i].length << endl;
    }
    
    cout << endl << "***** RM Test Case 0 finished. The result will be examined. *****" << endl;
    
    return success;
}

RC TEST_RM_1(const string &tableName, const int nameLength, const string &name, const int age, const float height, const int salary)
{
    // Functions tested
    // 1. Insert Tuple **
    // 2. Read Tuple **
    // NOTE: "**" signifies the new functions being tested in this test case.
    cout << endl << "***** In RM Test Case 1 *****" << endl;
    
    RID rid;
    int tupleSize = 0;
    void *tuple = malloc(200);
    void *returnedData = malloc(200);
    
    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    // Initialize a NULL field indicator
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    // Insert a tuple into a table
    prepareTuple(attrs.size(), nullsIndicator, nameLength, name, age, height, salary, tuple, &tupleSize);
    cout << "The tuple to be inserted:" << endl;
    rc = rm->printTuple(attrs, tuple);
    cout << endl;
    
    rc = rm->insertTuple(tableName, tuple, rid);
    assert(rc == success && "RelationManager::insertTuple() should not fail.");
    
    // Given the rid, read the tuple from table
    rc = rm->readTuple(tableName, rid, returnedData);
    assert(rc == success && "RelationManager::readTuple() should not fail.");
    
    cout << "The returned tuple:" << endl;
    rc = rm->printTuple(attrs, returnedData);
    cout << endl;
    
    // Compare whether the two memory blocks are the same
    if(memcmp(tuple, returnedData, tupleSize) == 0)
    {
        cout << "**** RM Test Case 1 finished. The result will be examined. *****" << endl << endl;
        free(tuple);
        free(returnedData);
        free(nullsIndicator);
        return success;
    }
    else
    {
        cout << "**** [FAIL] RM Test Case 1 failed *****" << endl << endl;
        free(tuple);
        free(returnedData);
        free(nullsIndicator);
        return -1;
    }
    
}

RC TEST_RM_2(const string &tableName, const int nameLength, const string &name, const int age, const float height, const int salary)
{
    // Functions Tested
    // 1. Insert tuple
    // 2. Delete Tuple **
    // 3. Read Tuple
    cout << endl << "***** In RM Test Case 2 *****" << endl;
    
    RID rid;
    int tupleSize = 0;
    void *tuple = malloc(200);
    void *returnedData = malloc(200);
    
    // Test Insert the Tuple
    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    prepareTuple(attrs.size(), nullsIndicator, nameLength, name, age, height, salary, tuple, &tupleSize);
    cout << "The tuple to be inserted:" << endl;
    rc = rm->printTuple(attrs, tuple);
    cout << endl;
    rc = rm->insertTuple(tableName, tuple, rid);
    assert(rc == success && "RelationManager::insertTuple() should not fail.");
    
    // Delete the tuple
    rc = rm->deleteTuple(tableName, rid);
    assert(rc == success && "RelationManager::deleteTuple() should not fail.");
    
    // Read Tuple after deleting it - should fail
    memset(returnedData, 0, 200);
    rc = rm->readTuple(tableName, rid, returnedData);
    assert(rc != success && "Reading a deleted tuple should fail.");
    
    // Compare the two memory blocks to see whether they are different
    if (memcmp(tuple, returnedData, tupleSize) != 0)
    {
        cout << "***** RM Test Case 2 finished. The result will be examined. *****" << endl << endl;
        free(tuple);
        free(returnedData);
        free(nullsIndicator);
        return success;
    }
    else
    {
        cout << "***** [FAIL] RM Test case 2 failed *****" << endl << endl;
        free(tuple);
        free(returnedData);
        free(nullsIndicator);
        return -1;
    }
    
}


RC TEST_RM_3(const string &tableName, const int nameLength, const string &name, const int age, const float height, const int salary)
{
    // Functions Tested
    // 1. Insert Tuple
    // 2. Update Tuple **
    // 3. Read Tuple
    cout << endl << "***** In RM Test Case 3****" << endl;
    
    RID rid;
    int tupleSize = 0;
    int updatedTupleSize = 0;
    void *tuple = malloc(200);
    void *updatedTuple = malloc(200);
    void *returnedData = malloc(200);
    
    // Test Insert the Tuple
    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    prepareTuple(attrs.size(), nullsIndicator, nameLength, name, age, height, salary, tuple, &tupleSize);
    rc = rm->insertTuple(tableName, tuple, rid);
    assert(rc == success && "RelationManager::insertTuple() should not fail.");
    cout << "Original RID:  " << rid.pageNum << " " << rid.slotNum << endl;
    
    // Test Update Tuple
    prepareTuple(attrs.size(), nullsIndicator, 7, "Barbara", age, height, 12000, updatedTuple, &updatedTupleSize);
    rc = rm->updateTuple(tableName, updatedTuple, rid);
    assert(rc == success && "RelationManager::updateTuple() should not fail.");
    
    // Test Read Tuple
    rc = rm->readTuple(tableName, rid, returnedData);
    assert(rc == success && "RelationManager::readTuple() should not fail.");
    
    // Print the tuples
    cout << "Inserted Data:" << endl;
    rm->printTuple(attrs, tuple);
    cout << endl;
    
    cout << "Updated data:" << endl;
    rm->printTuple(attrs, updatedTuple);
    cout << endl;
    
    cout << "Returned Data:" << endl;
    rm->printTuple(attrs, returnedData);
    cout << endl;
    
    if (memcmp(updatedTuple, returnedData, updatedTupleSize) == 0)
    {
        cout << "***** RM Test Case 3 Finished. The result will be examined. *****" << endl << endl;
        free(tuple);
        free(updatedTuple);
        free(returnedData);
        free(nullsIndicator);
        return 0;
    }
    else
    {
        cout << "***** [FAIL] RM Test case 3 Failed *****" << endl << endl;
        free(tuple);
        free(updatedTuple);
        free(returnedData);
        free(nullsIndicator);
        return -1;
    }
    
}

RC TEST_RM_4(const string &tableName, const int nameLength, const string &name, const int age, const float height, const int salary)
{
    // Functions Tested
    // 1. Insert tuple
    // 2. Read Attributes **
    cout << endl << "***** In RM Test Case 4 *****" << endl;
    
    RID rid;
    int tupleSize = 0;
    void *tuple = malloc(200);
    void *returnedData = malloc(200);
    
    // Test Insert the Tuple
    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    prepareTuple(attrs.size(), nullsIndicator, nameLength, name, age, height, salary, tuple, &tupleSize);
    rc = rm->insertTuple(tableName, tuple, rid);
    assert(rc == success && "RelationManager::insertTuple() should not fail.");
    
    // Test Read Attribute
    rc = rm->readAttribute(tableName, rid, "Salary", returnedData);
    assert(rc == success && "RelationManager::readAttribute() should not fail.");
    
    int salaryBack = *(int *)((char *)returnedData+nullAttributesIndicatorActualSize);
    
    cout << "Salary: " << salary << " Returned Salary: " << salaryBack << endl;
    if (memcmp((char *)returnedData+nullAttributesIndicatorActualSize, (char *)tuple+19+nullAttributesIndicatorActualSize, 4) == 0)
    {
        cout << "***** RM Test case 4 Finished. The result will be examined. *****" << endl << endl;
        free(tuple);
        free(returnedData);
        free(nullsIndicator);
        return success;
    }
    else
    {
        cout << "***** [FAIL] RM Test Case 4 Failed. *****" << endl << endl;
        free(tuple);
        free(returnedData);
        free(nullsIndicator);
        return -1;
    }
    
}

RC TEST_RM_5(const string &tableName, const int nameLength, const string &name, const int age, const float height, const int salary)
{
    // Functions Tested
    // 0. Insert tuple;
    // 1. Read Tuple
    // 2. Delete Table **
    // 3. Read Tuple
    // 4. Insert Tuple
    cout << endl << "***** In RM Test Case 5 *****" << endl;
    
    RID rid;
    int tupleSize = 0;
    void *tuple = malloc(200);
    void *returnedData = malloc(200);
    void *returnedData1 = malloc(200);
    
    // Test Insert Tuple
    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    prepareTuple(attrs.size(), nullsIndicator, nameLength, name, age, height, salary, tuple, &tupleSize);
    rc = rm->insertTuple(tableName, tuple, rid);
    assert(rc == success && "RelationManager::insertTuple() should not fail.");
    
    // Test Read Tuple
    rc = rm->readTuple(tableName, rid, returnedData);
    assert(rc == success && "RelationManager::readTuple() should not fail.");
    
    // Test Delete Table
    rc = rm->deleteTable(tableName);
    assert(rc == success && "RelationManager::deleteTable() should not fail.");
    
    // Reading a tuple on a deleted table
    memset((char*)returnedData1, 0, 200);
    rc = rm->readTuple(tableName, rid, returnedData1);
    assert(rc != success && "RelationManager::readTuple() on a deleted table should fail.");
    
    // Inserting a tuple on a deleted table
    rc = rm->insertTuple(tableName, tuple, rid);
    assert(rc != success && "RelationManager::insertTuple() on a deleted table should fail.");
    
    if(memcmp(returnedData, returnedData1, tupleSize) != 0)
    {
        cout << "***** Test Case 5 Finished. The result will be examined. *****" << endl << endl;
        free(tuple);
        free(returnedData);
        free(returnedData1);
        free(nullsIndicator);
        return success;
    }
    else
    {
        cout << "***** [FAIL] Test Case 5 Failed *****" << endl << endl;
        free(tuple);
        free(returnedData);
        free(returnedData1);
        free(nullsIndicator);
        return -1;
    }
}

RC TEST_RM_6(const string &tableName)
{
    // Functions Tested
    // 1. Simple scan **
    cout << endl << "***** In RM Test Case 6 *****" << endl;
    
    RID rid;
    int tupleSize = 0;
    int numTuples = 100;
    void *tuple;
    void *returnedData = malloc(200);
    
    // Test Insert Tuple
    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    RID rids[numTuples];
    set<int> ages;
    for(int i = 0; i < numTuples; i++)
    {
        tuple = malloc(200);
        
        // Insert Tuple
        float height = (float)i;
        int age = 20+i;
        prepareTuple(attrs.size(), nullsIndicator, 6, "Tester", age, height, age*10, tuple, &tupleSize);
        ages.insert(age);
        rc = rm->insertTuple(tableName, tuple, rid);
        assert(rc == success && "RelationManager::insertTuple() should not fail.");
        
        rids[i] = rid;
        free(tuple);
    }
    
    // Set up the iterator
    RM_ScanIterator rmsi; // constructor will be called even as early as at this initialization step
    string attr = "Age";
    vector<string> attributes;
    attributes.push_back(attr);
    rc = rm->scan(tableName, "", NO_OP, NULL, attributes, rmsi);
    assert(rc == success && "RelationManager::scan() should not fail.");
    
    nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attributes.size());
    while(rmsi.getNextTuple(rid, returnedData) != RM_EOF)
    {
         cout << "Returned Age: " << *(int *)((char *)returnedData+nullAttributesIndicatorActualSize) << endl;
        if (ages.find(*(int *)((char *)returnedData+nullAttributesIndicatorActualSize)) == ages.end())
        {
            cout << "***** [FAIL] Test Case 6 Failed *****" << endl << endl;
            rmsi.close();
            free(returnedData);
            free(nullsIndicator);
            return -1;
        }
    }
    rmsi.close();
    
    free(returnedData);
    free(nullsIndicator);
    cout << "***** Test Case 6 Finished. The result will be examined. *****" << endl << endl;
    return 0;
}

RC TEST_RM_7(const string &tableName)
{
    // Functions Tested
    // 1. Simple scan **
    // 2. Delete the given table
    cout << endl << "***** In RM Test Case 7 *****" << endl;
    
    RID rid;
    int numTuples = 100;
    void *returnedData = malloc(200);
    
    set<int> ages;
    RC rc = 0;
    for(int i = 0; i < numTuples; i++)
    {
        int age = 20+i;
        ages.insert(age);
    }
    
    // Set up the iterator
    RM_ScanIterator rmsi;
    string attr = "Age";
    vector<string> attributes;
    attributes.push_back(attr);
    rc = rm->scan(tableName, "", NO_OP, NULL, attributes, rmsi);
    assert(rc == success && "RelationManager::scan() should not fail.");
    int ageReturned = 0;
    
    while(rmsi.getNextTuple(rid, returnedData) != RM_EOF)
    {
        // cout << "Returned Age: " << *(int *)((char *)returnedData+1) << endl;
        ageReturned = *(int *)((char *)returnedData+1);
        if (ages.find(ageReturned) == ages.end())
        {
            cout << "***** [FAIL] Test Case 7 Failed *****" << endl << endl;
            rmsi.close();
            free(returnedData);
            return -1;
        }
    }
    rmsi.close();
    
    // Delete a Table
    rc = rm->deleteTable(tableName);
    assert(rc == success && "RelationManager::deleteTable() should not fail.");
    
    free(returnedData);
    cout << "***** Test Case 7 Finished. The result will be examined. *****" << endl << endl;
    return success;
}

RC TEST_RM_8(const string &tableName, vector<RID> &rids, vector<int> &sizes)
{
    // Functions Tested for large tables:
    // 1. getAttributes
    // 2. insert tuple
    cout << endl << "***** In RM Test Case 8 *****" << endl;
    
    RID rid;
    void *tuple = malloc(4000);
    int numTuples = 2000;
    
    // GetAttributes
    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    // Insert 2000 tuples into table
    for(int i = 0; i < numTuples; i++)
    {
        // Test insert Tuple
        int size = 0;
        memset(tuple, 0, 2000);
        prepareLargeTuple(attrs.size(), nullsIndicator, i, tuple, &size);
        
        rc = rm->insertTuple(tableName, tuple, rid);
        assert(rc == success && "RelationManager::insertTuple() should not fail.");
        
        rids.push_back(rid);
        sizes.push_back(size);
    }
    
    free(tuple);
    free(nullsIndicator);
    
    writeRIDsToDisk(rids);
    writeSizesToDisk(sizes);
    
    cout << "***** Test Case 8 Finished. The result will be examined. *****" << endl << endl;
    
    return success;
}

RC TEST_RM_09(const string &tableName, vector<RID> &rids, vector<int> &sizes)
{
    // Functions Tested for large tables:
    // 1. read tuple
    cout << "***** In RM Test case 9 *****" << endl;
    
    int size = 0;
    int numTuples = 2000;
    void *tuple = malloc(4000);
    void *returnedData = malloc(4000);
    
    // read the saved rids and the sizes of records
    readRIDsFromDisk(rids, numTuples);
    readSizesFromDisk(sizes, numTuples);
    
    // GetAttributes
    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    for(int i = 0; i < numTuples; i++)
    {
        memset(tuple, 0, 4000);
        memset(returnedData, 0, 4000);
        rc = rm->readTuple(tableName, rids[i], returnedData);
        assert(rc == success && "RelationManager::readTuple() should not fail.");
        
        size = 0;
        prepareLargeTuple(attrs.size(), nullsIndicator, i, tuple, &size);
        if(memcmp(returnedData, tuple, sizes[i]) != 0)
        {
            cout << "***** [FAIL] Test Case 9 Failed *****" << endl << endl;
            free(tuple);
            free(returnedData);
            free(nullsIndicator);
            return -1;
        }
    }
    
    free(tuple);
    free(returnedData);
    free(nullsIndicator);
    
    cout << "***** Test Case 9 Finished. The result will be examined. *****" << endl << endl;
    
    return success;
}

RC TEST_RM_10(const string &tableName, vector<RID> &rids, vector<int> &sizes)
{
    // Functions Tested for large tables:
    // 1. update tuple
    // 2. read tuple
    cout << endl << "***** In RM Test case 10 *****" << endl;
    
    int numTuples = 2000;
    void *tuple = malloc(4000);
    void *returnedData = malloc(4000);
    
    readRIDsFromDisk(rids, numTuples);
    readSizesFromDisk(sizes, numTuples);
    
    // GetAttributes
    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    // Update the first 1000 tuples
    int size = 0;
    for(int i = 0; i < 1000; i++)
    {
        memset(tuple, 0, 4000);
        RID rid = rids[i];
        
        prepareLargeTuple(attrs.size(), nullsIndicator, i+10, tuple, &size);
        rc = rm->updateTuple(tableName, tuple, rid);
        assert(rc == success && "RelationManager::updateTuple() should not fail.");
        
        sizes[i] = size;
        rids[i] = rid;
    }
    
    // Read the updated records and check the integrity
    for(int i = 0; i < 1000; i++)
    {
        memset(tuple, 0, 4000);
        memset(returnedData, 0, 4000);
        prepareLargeTuple(attrs.size(), nullsIndicator, i+10, tuple, &size);
        rc = rm->readTuple(tableName, rids[i], returnedData);
        assert(rc == success && "RelationManager::readTuple() should not fail.");
        
        if(memcmp(returnedData, tuple, sizes[i]) != 0)
        {
            cout << "***** [FAIL] Test Case 10 Failed *****" << endl << endl;
            free(tuple);
            free(returnedData);
            free(nullsIndicator);
            return -1;
        }
    }
    
    free(tuple);
    free(returnedData);
    free(nullsIndicator);
    
    cout << "***** Test Case 10 Finished. The result will be examined. *****" << endl << endl;
    
    return success;
    
}

RC TEST_RM_11(const string &tableName, vector<RID> &rids)
{
    // Functions Tested for large tables:
    // 1. delete tuple
    // 2. read tuple
    cout << endl << "***** In RM Test Case 11 *****" << endl;
    
    int numTuples = 2000;
    RC rc = 0;
    void * returnedData = malloc(4000);
    
    readRIDsFromDisk(rids, numTuples);
    
    // Delete the first 1000 tuples
    for(int i = 0; i < 1000; i++)
    {
        rc = rm->deleteTuple(tableName, rids[i]);
        assert(rc == success && "RelationManager::deleteTuple() should not fail.");
    }
    
    // Try to read the first 1000 deleted tuples
    for(int i = 0; i < 1000; i++)
    {
        rc = rm->readTuple(tableName, rids[i], returnedData);
        assert(rc != success && "RelationManager::readTuple() on a deleted tuple should fail.");
    }
    
    for(int i = 1000; i < 2000; i++)
    {
        cout << "rids[i]: " << rids[i].pageNum << '\t' << rids[i].slotNum << '\t' << "i: " << i << endl;
        rc = rm->readTuple(tableName, rids[i], returnedData);
        assert(rc == success && "RelationManager::readTuple() should not fail.");
    }
    cout << "***** Test Case 11 Finished. The result will be examined. *****" << endl << endl;
    
    free(returnedData);
    
    return success;
}

RC TEST_RM_12(const string &tableName)
{
    // Functions Tested for large tables
    // 1. scan
    cout << endl << "***** In RM Test case 12 *****" << endl;
    
    RM_ScanIterator rmsi;
    vector<string> attrs;
    attrs.push_back("attr5");
    attrs.push_back("attr12");
    attrs.push_back("attr28");
    
    RC rc = rm->scan(tableName, "", NO_OP, NULL, attrs, rmsi);
    assert(rc == success && "RelationManager::scan() should not fail.");
    
    RID rid;
    int j = 0;
    void *returnedData = malloc(4000);
    
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    
    while(rmsi.getNextTuple(rid, returnedData) != RM_EOF)
    {
        if(j % 200 == 0)
        {
            int offset = 0;
            
            cout << "Real Value: " << *(float *)((char *)returnedData+nullAttributesIndicatorActualSize) << endl;
            offset += 4;
            
            int size = *(int *)((char *)returnedData + offset + nullAttributesIndicatorActualSize);
            cout << "Varchar size: " << size << endl;
            offset += 4;
            
            char *buffer = (char *)malloc(size + 1);
            memcpy(buffer, (char *)returnedData + offset + nullAttributesIndicatorActualSize, size);
            buffer[size] = 0;
            offset += size;
            
            cout << "VarChar Value: " << buffer << endl;
            
            cout << "Integer Value: " << *(int *)((char *)returnedData + offset + nullAttributesIndicatorActualSize) << endl << endl;
            offset += 4;
            
            free(buffer);
        }
        j++;
        memset(returnedData, 0, 4000);
    }
    rmsi.close();
    cout << "Total number of tuples: " << j << endl << endl;
    if (j > 1000) {
        cout << "***** [FAIL] Test Case 12 Failed *****" << endl << endl;
        free(returnedData);
        return -1;
    }
    
    cout << "***** Test Case 12 Finished. The result will be examined. *****" << endl << endl;
    free(returnedData);
    
    return success;
}

RC TEST_RM_13(const string &tableName)
{
    // Functions Tested:
    // 1. Conditional scan
    cout << endl << "***** In RM Test Case 13 *****" << endl;
    
    RID rid;
    int tupleSize = 0;
    int numTuples = 500;
    void *tuple;
    void *returnedData = malloc(200);
    int ageVal = 25;
    int age = 0;
    
    RID rids[numTuples];
    vector<char *> tuples;
    
    // GetAttributes
    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    for(int i = 0; i < numTuples; i++)
    {
        tuple = malloc(100);
        
        // Insert Tuple
        float height = (float)i;
        
        age = (rand()%10) + 23;
        
        prepareTuple(attrs.size(), nullsIndicator, 6, "Tester", age, height, 123, tuple, &tupleSize);
        rc = rm->insertTuple(tableName, tuple, rid);
        assert(rc == success && "RelationManager::insertTuple() should not fail.");
        
        rids[i] = rid;
        free(tuple);
    }
    
    // Set up the iterator
    RM_ScanIterator rmsi;
    string attr = "Age";
    vector<string> attributes;
    attributes.push_back(attr);
    rc = rm->scan(tableName, attr, GT_OP, &ageVal, attributes, rmsi);
    assert(rc == success && "RelationManager::scan() should not fail.");
    
    while(rmsi.getNextTuple(rid, returnedData) != RM_EOF)
    {
        age = *(int *)((char *)returnedData+1);
        if (age <= ageVal) {
            cout << "Returned value from a scan is not correct." << endl;
            cout << "***** [FAIL] Test Case 13 Failed *****" << endl << endl;
            rmsi.close();
            free(returnedData);
            free(nullsIndicator);
            return -1;
        }
    }
    rmsi.close();
    free(returnedData);
    free(nullsIndicator);
    
    rc = rm->deleteTable("tbl_b_employee4");
    
    
    cout << "***** Test Case 13 Finished. The result will be examined. *****" << endl << endl;
    
    return success;
}


RC TEST_RM_13b(const string &tableName)
{
    // Functions Tested:
    // 1. Conditional scan - including NULL values
    cout << endl << "***** In RM Test Case 13B *****" << endl;
    
    RID rid;
    int tupleSize = 0;
    int numTuples = 500;
    void *tuple;
    void *returnedData = malloc(200);
    int ageVal = 25;
    int age = 0;
    
    RID rids[numTuples];
    vector<char *> tuples;
    string tupleName;
    char *suffix = (char *)malloc(10);
    
    bool nullBit = false;
    
    // GetAttributes
    vector<Attribute> attrs;
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    unsigned char *nullsIndicatorWithNull = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicatorWithNull, 0, nullAttributesIndicatorActualSize);
    
    // age field : NULL
    nullsIndicatorWithNull[0] = 64; // 01000000
    
    for(int i = 0; i < numTuples; i++)
    {
        tuple = malloc(100);
        
        // Insert Tuple
        float height = (float)i;
        
        age = (rand()%20) + 15;
        
        sprintf(suffix, "%d", i);
        
        if (i % 10 == 0) {
            tupleName = "TesterNull";
            tupleName += suffix;
            prepareTuple(attrs.size(), nullsIndicatorWithNull, tupleName.length(), tupleName, 0, height, 456, tuple, &tupleSize);
        } else {
            tupleName = "Tester";
            tupleName += suffix;
            prepareTuple(attrs.size(), nullsIndicator, tupleName.length(), tupleName, age, height, 123, tuple, &tupleSize);
        }
        rc = rm->insertTuple(tableName, tuple, rid);
        assert(rc == success && "RelationManager::insertTuple() should not fail.");
        
        rids[i] = rid;
        free(tuple);
    }
    
    // Set up the iterator
    RM_ScanIterator rmsi;
    string attr = "Age";
    vector<string> attributes;
    attributes.push_back(attr);
    rc = rm->scan(tableName, attr, GT_OP, &ageVal, attributes, rmsi);
    assert(rc == success && "RelationManager::scan() should not fail.");
    
    while(rmsi.getNextTuple(rid, returnedData) != RM_EOF)
    {
        // Check the first bit of the returned data since we only return one attribute in this test case
        // However, the age with NULL should not be returned since the condition NULL > 25 can't hold.
        // All comparison operations with NULL should return FALSE
        // (e.g., NULL > 25, NULL >= 25, NULL <= 25, NULL < 25, NULL == 25, NULL != 25: ALL FALSE)
        nullBit = *(unsigned char *)((char *)returnedData) & (1 << 7);
        if (!nullBit) {
            age = *(int *)((char *)returnedData+1);
            if (age <= ageVal) {
                // Comparison didn't work in this case
                cout << "Returned value from a scan is not correct: returned Age <= 25." << endl;
                cout << "***** [FAIL] Test Case 13B Failed *****" << endl << endl;
                rmsi.close();
                free(returnedData);
                free(suffix);
                free(nullsIndicator);
                free(nullsIndicatorWithNull);
                return -1;
            }
        } else {
            // Age with NULL value should not be returned.
            cout << "Returned value from a scan is not correct. NULL returned." << endl;
            cout << "***** [FAIL] Test Case 13B Failed *****" << endl << endl;
            rmsi.close();
            free(returnedData);
            free(suffix);
            free(nullsIndicator);
            free(nullsIndicatorWithNull);
            return -1;
        }
    }
    rmsi.close();
    free(returnedData);
    free(suffix);
    free(nullsIndicator);
    free(nullsIndicatorWithNull);
    
    rc = rm->deleteTable("tbl_b_employee5");
    
    cout << "Test Case 13B Finished. The result will be examined. *****" << endl << endl;
    
    return success;
}


int main()
{

    create();
    
    /*
     * unit test
     */
    TEST_RM_0("tbl_employee");

    TEST_RM_1("tbl_employee", 14, "Peter Anteater", 27, 6.2, 10000);

    TEST_RM_2("tbl_employee", 5, "Peter", 23, 5.11, 12000);

    TEST_RM_3("tbl_employee", 4, "Paul", 28, 6.5, 6000);

    TEST_RM_4("tbl_employee", 7, "Hoffman", 31, 5.8, 9999);

    TEST_RM_5("tbl_employee", 6, "Martin", 29, 193.6, 20000); // "tbl_employee" will be deleted

    TEST_RM_6("tbl_employee3");

    TEST_RM_7("tbl_employee3");
    
    /*
     * performance test
     */
    vector<RID> rids;
    vector<int> sizes;
    
    TEST_RM_8("tbl_employee4", rids, sizes);

    TEST_RM_09("tbl_employee4", rids, sizes);

    TEST_RM_10("tbl_employee4", rids, sizes);

    TEST_RM_11("tbl_employee4", rids);

    TEST_RM_12("tbl_employee4");
    
    createTable("tbl_b_employee4");
    TEST_RM_13("tbl_b_employee4");

    createTable("tbl_b_employee5");
    TEST_RM_13b("tbl_b_employee5");
    
    return 0;
}
