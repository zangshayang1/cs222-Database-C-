#include "../RelationManager/rm_test_util.h"


int TEST_RM_PRIVATE_0(const string &tableName)
{
    // Functions tested
    // 1. Insert 1000 Tuples
    // 2. Scan Table (with condition on varchar attr)
    // 3. Delete Table
    cerr << endl << "***** In RM Test Case Private 0 *****" << endl;
    
    RID rid;
    int tupleSize = 0;
    int numTuples = 1000;
    void *tuple;
    void *returnedData = malloc(100);
    RC rc = 0;
    
    // GetAttributes
    vector<Attribute> attrs;
    rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    RID rids[numTuples];
    vector<char *> tuples;
    set<int> ages;
    for(int i = 0; i < numTuples; i++)
    {
        tuple = malloc(100);
        
        // Insert Tuple
        float height = (float)i;
        int age = i;
        
        string name;
        if (i % 2 == 0) {
            name = "aaa";
        } else {
            name = "bbb";
        }
        prepareTuple(attrs.size(), nullsIndicator, name.size(), name, age, height, 2000 + i, tuple, &tupleSize);
        ages.insert(age);
        rc = rm->insertTuple(tableName, tuple, rid);
        assert(rc == success && "RelationManager::insertTuple() should not fail.");
        
        tuples.push_back((char *)tuple);
        rids[i] = rid;
    }
    cerr << "All records have been processed." << endl;
    
    // Set up the iterator
    RM_ScanIterator rmsi;
    string attr = "EmpName";
    vector<string> attributes;
    attributes.push_back("Age");
    
    void *value = malloc(7);
    string name = "aaa";
    int nameLength = 3;
    
    memcpy((char *)value, &nameLength, 4);
    memcpy((char *)value + 4, name.c_str(), nameLength);
    
    rc = rm->scan(tableName, attr, EQ_OP, value, attributes, rmsi);
    if(rc != success) {
        free(returnedData);
        for(int i = 0; i < numTuples; i++)
        {
            free(tuples[i]);
        }
        cerr << "***** RM Test Case Private 0 failed. *****" << endl << endl;
        return -1;
    }
    int counter = 0;
    int ageReturned = 0;
    
    while(rmsi.getNextTuple(rid, returnedData) != RM_EOF)
    {
        ageReturned = *(int *)((char *)returnedData + 1);
        cout << "ageReturned: " << ageReturned << endl;
        if (ageReturned % 2 != 0){
            cerr << "***** A returned value is not correct. RM Test Case Private 0 failed. *****" << endl << endl;
            rmsi.close();
            free(returnedData);
            for(int i = 0; i < numTuples; i++)
            {
                free(tuples[i]);
            }
            return -1;
        }
        counter++;
    }
    rmsi.close();
    
    if (counter != numTuples / 2){
        cerr << "***** The number of returned tuples:" << counter << ". RM Test Case Private 0 failed. *****" << endl << endl;
        free(returnedData);
        for(int i = 0; i < numTuples; i++)
        {
            free(tuples[i]);
        }
        return -1;
    }
    
    // Delete a Table
    rc = rm->deleteTable(tableName);
    if(rc != success) {
        cerr << "***** RelationManager::deleteTale() should not fail. RM Test Case Private 0 failed. *****" << endl << endl;
        free(returnedData);
        for(int i = 0; i < numTuples; i++)
        {
            free(tuples[i]);
        }
        return -1;
    }
    
    free(returnedData);
    for(int i = 0; i < numTuples; i++)
    {
        free(tuples[i]);
    }
    cerr << "***** RM Test Case Private 0 finished. The result will be examined. *****" << endl << endl;
    return 0;
}

RC TEST_RM_PRIVATE_1(const string &tableName)
{
    // Functions tested
    // 1. Insert 100,000 tuples
    // 2. Read Attribute
    cerr << endl << "***** In RM Test Case Private 1 *****" << endl;
    
    RID rid;
    int tupleSize = 0;
    int numTuples = 1000;
    void *tuple;
    void *returnedData = malloc(300);
    
    vector<RID> rids;
    vector<char *> tuples;
    set<int> user_ids;
    RC rc = 0;
    
    // GetAttributes
    vector<Attribute> attrs;
    rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    for(int i = 0; i < numTuples; i++)
    {
        tuple = malloc(300);
        
        // Insert Tuple
        float sender_location = (float)i;
        float send_time = (float)i + 2000;
        int tweetid = i;
        int userid = i + i % 100;
        stringstream ss;
        ss << setw(5) << setfill('0') << i;
        string msg = "Msg" + ss.str();
        string referred_topics = "Rto" + ss.str();
        
        prepareTweetTuple(attrs.size(), nullsIndicator, tweetid, userid, sender_location, send_time, referred_topics.size(), referred_topics, msg.size(), msg, tuple, &tupleSize);
        
        user_ids.insert(userid);
        rc = rm->insertTuple(tableName, tuple, rid);
        assert(rc == success && "RelationManager::insertTuple() should not fail.");
        
        tuples.push_back((char *)tuple);
        rids.push_back(rid);
        
        if (i % 1000 == 0){
            cerr << (i+1) << "/" << numTuples << " records have been inserted so far." << endl;
        }
    }
    cerr << "All records have been inserted." << endl;
    
    // Required for the other tests
    writeRIDsToDisk(rids);
    writeUserIdsToDisk(user_ids);
    
    bool testFail = false;
    string attributeName;
    
    for(int i = 0; i < numTuples; i=i+10)
    {
        int attrID = rand() % 6;
        if (attrID == 0) {
            attributeName = "tweetid";
        } else if (attrID == 1) {
            attributeName = "userid";
        } else if (attrID == 2) {
            attributeName = "sender_location";
        } else if (attrID == 3) {
            attributeName = "send_time";
        } else if (attrID == 4){
            attributeName = "referred_topics";
        } else if (attrID == 5){
            attributeName = "message_text";
        }
        rc = rm->readAttribute(tableName, rids[i], attributeName, returnedData);
        assert(rc == success && "RelationManager::readAttribute() should not fail.");
        
        int value = 0;
        float fvalue = 0;
        stringstream ss;
        ss << setw(5) << setfill('0') << i;
        string msgToCheck = "Msg" + ss.str();
        string referred_topicsToCheck = "Rto" + ss.str();
        
        // tweetid
        if (attrID == 0) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(i) + 1), 4) != 0) {
                testFail = true;
            } else {
                value = *(int *)((char *)returnedData + 1);
                if (value != i) {
                    testFail = true;
                }
            }
            // userid
        } else if (attrID == 1) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(i) + 5), 4) != 0) {
                testFail = true;
            } else {
                value = *(int *)((char *)returnedData + 1);
                if (value != (i + i % 100)) {
                    testFail = true;
                }
            }
            // sender_location
        } else if (attrID == 2) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(i) + 9), 4) != 0) {
                testFail = true;
            } else {
                fvalue = *(float *)((char *)returnedData + 1);
                if (fvalue != (float) i) {
                    testFail = true;
                }
            }
            // send_time
        } else if (attrID == 3) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(i) + 13), 4) != 0) {
                testFail = true;
            }
            // referred_topics
        } else if (attrID == 4) {
            if (memcmp(((char *)returnedData + 5), ((char *)tuples.at(i) + 21), 8) != 0) {
                testFail = true;
            } else {
                std::string strToCheck (((char *)returnedData + 5), 8);
                if (strToCheck.compare(referred_topicsToCheck) != 0) {
                    testFail = true;
                }
            }
            // message_text
        } else if (attrID == 5) {
            if (memcmp(((char *)returnedData + 5), ((char *)tuples.at(i) + 33), 8) != 0) {
                testFail = true;
            } else {
                std::string strToCheck (((char *)returnedData + 5), 8);
                if (strToCheck.compare(msgToCheck) != 0) {
                    testFail = true;
                }
            }
        }
        
        if (testFail) {
            cerr << "***** RM Test Case Private 1 failed on " << i << "th tuple - attr: " << attrID << "*****" << endl << endl;
            free(returnedData);
            for(int j = 0; j < numTuples; j++)
            {
                free(tuples[j]);
            }
            rc = rm->deleteTable(tableName);
            remove("rids_file");
            remove("user_ids_file");
            
            return -1;
        }
        
    }
    
    free(returnedData);
    for(int i = 0; i < numTuples; i++)
    {
        free(tuples[i]);
    }
    
    cerr << "***** RM Test Case Private 1 finished. The result will be examined. *****" << endl << endl;
    
    return 0;
}

RC TEST_RM_PRIVATE_2(const string &tableName)
{
    // Functions tested
    // 1. Insert 1000 Tuples - with null values
    // 2. Read Attribute
    cerr << endl << "***** In RM Test Case Private 2 *****" << endl;
    
    RID rid;
    int tupleSize = 0;
    int numTuples = 1000;
    void *tuple;
    void *returnedData = malloc(300);
    
    vector<RID> rids;
    vector<char *> tuples;
    RC rc = 0;
    
    // GetAttributes
    vector<Attribute> attrs;
    rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    unsigned char *nullsIndicatorWithNulls = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    memset(nullsIndicatorWithNulls, 0, nullAttributesIndicatorActualSize);
    nullsIndicatorWithNulls[0] = 84; // 01010100 - the 2nd, 4th, and 6th column is null. - userid, send_time, and message_text
    
    for(int i = 0; i < numTuples; i++)
    {
        tuple = malloc(300);
        
        // Insert Tuple
        float sender_location = (float)i;
        float send_time = (float)i + 2000;
        int tweetid = i;
        int userid = i;
        stringstream ss;
        ss << setw(5) << setfill('0') << i;
        string msg = "Msg" + ss.str();
        string referred_topics = "Rto" + ss.str();
        
        // There will be some tuples with nulls.
        if (i % 50 == 0) {
            prepareTweetTuple(attrs.size(), nullsIndicatorWithNulls, tweetid, userid, sender_location, send_time, referred_topics.size(), referred_topics, msg.size(), msg, tuple, &tupleSize);
        } else {
            prepareTweetTuple(attrs.size(), nullsIndicator, tweetid, userid, sender_location, send_time, referred_topics.size(), referred_topics, msg.size(), msg, tuple, &tupleSize);
        }
        
        rc = rm->insertTuple(tableName, tuple, rid);
        assert(rc == success && "RelationManager::insertTuple() should not fail.");
        
        tuples.push_back((char *)tuple);
        rids.push_back(rid);
        
        if (i % 1000 == 0){
            cerr << (i+1) << "/" << numTuples << " records have been inserted so far." << endl;
        }
    }
    cerr << "All records have been inserted." << endl;
    
    bool testFail = false;
    bool nullBit = false;
    for(int i = 0; i < numTuples; i++)
    {
        int attrID = rand() % 6;
        string attributeName;
        
        // Force attrID to be the ID that contains NULL when a i%50 is 0.
        if (i%50 == 0) {
            if (attrID % 2 == 0) {
                attrID = 1;
            } else {
                attrID = 5;
            }
        }
        
        if (attrID == 0) {
            attributeName = "tweetid";
        } else if (attrID == 1) {
            attributeName = "userid";
        } else if (attrID == 2) {
            attributeName = "sender_location";
        } else if (attrID == 3) {
            attributeName = "send_time";
        } else if (attrID == 4){
            attributeName = "referred_topics";
        } else if (attrID == 5){
            attributeName = "message_text";
        }
        rc = rm->readAttribute(tableName, rids[i], attributeName, returnedData);
        assert(rc == success && "RelationManager::readAttribute() should not fail.");
        
        // NULL indicator should say that a NULL value is returned.
        if (i%50 == 0) {
            nullBit = *(unsigned char *)((char *)returnedData) & (1 << 7);
            if (!nullBit) {
                cerr << "A returned value from a readAttribute() is not correct: attrID - " << attrID << endl;
                testFail = true;
            }
        } else {
            int value = 0;
            float fvalue = 0;
            stringstream ss;
            ss << setw(5) << setfill('0') << i;
            string msgToCheck = "Msg" + ss.str();
            string referred_topicsToCheck = "Rto" + ss.str();
            
            // tweetid
            if (attrID == 0) {
                if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(i) + 1), 4) != 0) {
                    testFail = true;
                } else {
                    value = *(int *)((char *)returnedData + 1);
                    if (value != i) {
                        testFail = true;
                    }
                }
                // userid
            } else if (attrID == 1) {
                if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(i) + 5), 4) != 0) {
                    testFail = true;
                } else {
                    value = *(int *)((char *)returnedData + 1);
                    if (value != i) {
                        testFail = true;
                    }
                }
                // sender_location
            } else if (attrID == 2) {
                if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(i) + 9), 4) != 0) {
                    testFail = true;
                } else {
                    fvalue = *(float *)((char *)returnedData + 1);
                    if (fvalue != (float) i) {
                        testFail = true;
                    }
                }
                // send_time
            } else if (attrID == 3) {
                if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(i) + 13), 4) != 0) {
                    testFail = true;
                }
                // referred_topics
            } else if (attrID == 4) {
                if (memcmp(((char *)returnedData + 5), ((char *)tuples.at(i) + 21), 8) != 0) {
                    testFail = true;
                } else {
                    std::string strToCheck (((char *)returnedData + 5), 8);
                    if (strToCheck.compare(referred_topicsToCheck) != 0) {
                        testFail = true;
                    }
                }
                // message_text
            } else if (attrID == 5) {
                if (memcmp(((char *)returnedData + 5), ((char *)tuples.at(i) + 33), 8) != 0) {
                    testFail = true;
                } else {
                    std::string strToCheck (((char *)returnedData + 5), 8);
                    if (strToCheck.compare(msgToCheck) != 0) {
                        testFail = true;
                    }
                }
            }
        }
        
        if (testFail) {
            cerr << "***** RM Test Case Private 2 failed on the tuple #" << i << " - attrID: " << attrID << "*****" << endl << endl;
            free(returnedData);
            for(int j = 0; j < numTuples; j++)
            {
                free(tuples[j]);
            }
            rc = rm->deleteTable(tableName);
            
            return -1;
        }
        
    }
    
    free(returnedData);
    for(int i = 0; i < numTuples; i++)
    {
        free(tuples[i]);
    }
    
    cerr << "***** RM Test Case Private 2 finished. The result will be examined. *****" << endl << endl;
    
    return 0;
}

RC TEST_RM_PRIVATE_3(const string &tableName)
{
    // Functions tested
    // 1. Simple Scan
    cerr << endl << "***** In RM Test Case Private 3 *****" << endl;
    
    RID rid;
    int numTuples = 1000;
    void *returnedData = malloc(300);
    set<int> user_ids;
    
    // Read UserIds that was created in the private test 1
    readUserIdsFromDisk(user_ids, numTuples);
    
    // Set up the iterator
    RM_ScanIterator rmsi;
    string attr = "userid";
    vector<string> attributes;
    attributes.push_back(attr);
    RC rc = rm->scan(tableName, "", NO_OP, NULL, attributes, rmsi);
    if(rc != success) {
        cerr << "***** RM Test Case Private 3 failed. *****" << endl << endl;
        return -1;
    }
    
    int userid = 0;
    while(rmsi.getNextTuple(rid, returnedData) != RM_EOF)
    {
        userid = *(int *)((char *)returnedData + 1);
        
        if (user_ids.find(userid) == user_ids.end())
        {
            cerr << "***** RM Test Case Private 3 failed. *****" << endl << endl;
            rmsi.close();
            free(returnedData);
            return -1;
        }
    }
    rmsi.close();
    free(returnedData);
    
    cerr << "***** RM Test Case Private 3 finished. The result will be examined. *****" << endl << endl;
    return 0;
}

int TEST_RM_PRIVATE_4(const string &tableName)
{
    // Functions tested
    // 1. Scan Table (VarChar)
    cerr << endl << "***** In RM Test Case Private 4 *****" << endl;
    
    RID rid;
    vector<string> attributes;
    void *returnedData = malloc(300);
    
    void *value = malloc(16);
    string msg = "Msg00099";
    int msgLength = 8;
    
    memcpy((char *)value, &msgLength, 4);
    memcpy((char *)value + 4, msg.c_str(), msgLength);
    
    string attr = "message_text";
    attributes.push_back("sender_location");
    attributes.push_back("send_time");
    
    RM_ScanIterator rmsi2;
    RC rc = rm->scan(tableName, attr, LT_OP, value, attributes, rmsi2);
    if(rc != success) {
        free(returnedData);
        cerr << "***** RM Test Case Private 4 failed. *****" << endl << endl;
        return -1;
    }
    
    float sender_location = 0.0;
    float send_time = 0.0;
    
    int counter = 0;
    while(rmsi2.getNextTuple(rid, returnedData) != RM_EOF)
    {
        counter++;
        
        sender_location = *(float *)((char *)returnedData + 1);
        send_time = *(float *)((char *)returnedData + 5);
        
        if (!(sender_location >= 0.0 || sender_location <= 98.0 || send_time >= 2000.0 || send_time <= 2098.0))
        {
            cerr << "***** A wrong entry was returned. RM Test Case Private 4 failed *****" << endl << endl;
            rmsi2.close();
            free(returnedData);
            free(value);
            return -1;
        }
    }
    
    rmsi2.close();
    free(returnedData);
    free(value);
    
    if (counter != 99){
        cerr << "***** The number of returned tuple: " << counter << " is not correct. RM Test Case Private 4 failed *****" << endl << endl;
    } else {
        cerr << "***** RM Test Case Private 4 finished. The result will be examined. *****" << endl << endl;
    }
    return 0;
}

int TEST_RM_PRIVATE_5(const string &tableName)
{
    // Functions tested
    // 1. Scan Table (VarChar with Nulls)
    cerr << endl << "***** In RM Test Case Private 5 *****" << endl;
    
    RID rid;
    vector<string> attributes;
    void *returnedData = malloc(300);
    
    void *value = malloc(16);
    string msg = "Msg00099";
    int msgLength = 8;
    
    memcpy((char *)value, &msgLength, 4);
    memcpy((char *)value + 4, msg.c_str(), msgLength);
    
    string attr = "message_text";
    attributes.push_back("sender_location");
    attributes.push_back("send_time");
    
    RM_ScanIterator rmsi2;
    RC rc = rm->scan(tableName, attr, LT_OP, value, attributes, rmsi2);
    if(rc != success) {
        free(returnedData);
        cerr << "***** RM Test Case Private 5 failed. *****" << endl << endl;
        return -1;
    }
    
    float sender_location = 0.0;
    float send_time = 0.0;
    bool nullBit = false;
    int counter = 0;
    
    while(rmsi2.getNextTuple(rid, returnedData) != RM_EOF)
    {
        // There are 2 tuples whose message_text value is NULL
        // For these tuples, send_time is also NULL. These NULLs should not be returned.
        nullBit = *(unsigned char *)((char *)returnedData) & (1 << 6);
        
        if (nullBit) {
            cerr << "***** A wrong entry was returned. RM Test Case Private 4 failed *****" << endl << endl;
            rmsi2.close();
            free(returnedData);
            free(value);
            return -1;
        }
        counter++;
        
        sender_location = *(float *)((char *)returnedData + 1);
        send_time = *(float *)((char *)returnedData + 5);
        
        if (!(sender_location >= 0.0 || sender_location <= 98.0 || send_time >= 2000.0 || send_time <= 2098.0))
        {
            cerr << "***** A wrong entry was returned. RM Test Case Private 5 failed *****" << endl << endl;
            rmsi2.close();
            free(returnedData);
            free(value);
            return -1;
        }
    }
    
    rmsi2.close();
    free(returnedData);
    free(value);
    
    if (counter != 97){
        cerr << "***** The number of returned tuple: " << counter << " is not correct. RM Test Case Private 5 failed *****" << endl << endl;
    } else {
        cerr << "***** RM Test Case Private 5 finished. The result will be examined. *****" << endl << endl;
    }
    return 0;
}

RC TEST_RM_PRIVATE_6(const string &tableName)
{
    // Functions tested
    // 1. Update tuples
    // 2. Read Attribute
    cerr << endl << "***** In RM Test Case Private 6 *****" << endl;
    
    RID rid;
    int tupleSize = 0;
    int numTuples = 1000;
    void *tuple;
    void *returnedData = malloc(300);
    
    vector<RID> rids;
    vector<char *> tuples;
    set<int> user_ids;
    RC rc = 0;
    
    readRIDsFromDisk(rids, numTuples);
    
    // GetAttributes
    vector<Attribute> attrs;
    rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttributes() should not fail.");
    
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    int updateCount = 0;
    
    for(int i = 0; i < numTuples; i=i+100)
    {
        tuple = malloc(300);
        
        // Update Tuple
        float sender_location = (float)i + 5000;
        float send_time = (float)i + 7000;
        int tweetid = i;
        int userid = i + i % 500;
        stringstream ss;
        ss << setw(5) << setfill('0') << i;
        string msg = "UpdatedMsg" + ss.str();
        string referred_topics = "UpdatedRto" + ss.str();
        
        prepareTweetTuple(attrs.size(), nullsIndicator, tweetid, userid, sender_location, send_time, referred_topics.size(), referred_topics, msg.size(), msg, tuple, &tupleSize);
        
        // Update tuples
        rc = rm->updateTuple(tableName, tuple, rids[i]);
        assert(rc == success && "RelationManager::updateTuple() should not fail.");
        
        if (i % 1000 == 0){
            cerr << (i+1) << "/" << numTuples << " records have been processed so far." << endl;
        }
        updateCount++;
        
        tuples.push_back((char *)tuple);
    }
    cerr << "All records have been processed - update count: " << updateCount << endl;
    
    bool testFail = false;
    string attributeName;
    
    int readCount = 0;
    // Integrity check
    for(int i = 0; i < numTuples; i=i+100)
    {
        int attrID = rand() % 6;
        if (attrID == 0) {
            attributeName = "tweetid";
        } else if (attrID == 1) {
            attributeName = "userid";
        } else if (attrID == 2) {
            attributeName = "sender_location";
        } else if (attrID == 3) {
            attributeName = "send_time";
        } else if (attrID == 4){
            attributeName = "referred_topics";
        } else if (attrID == 5){
            attributeName = "message_text";
        }
        rc = rm->readAttribute(tableName, rids[i], attributeName, returnedData);
        assert(rc == success && "RelationManager::readAttribute() should not fail.");
        
        int value = 0;
        float fvalue = 0;
        stringstream ss;
        ss << setw(5) << setfill('0') << i;
        string msgToCheck = "UpdatedMsg" + ss.str();
        string referred_topicsToCheck = "UpdatedRto" + ss.str();
        
        // tweetid
        if (attrID == 0) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(readCount) + 1), 4) != 0) {
                testFail = true;
            } else {
                value = *(int *)((char *)returnedData + 1);
                if (value != i) {
                    testFail = true;
                }
            }
            // userid
        } else if (attrID == 1) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(readCount) + 5), 4) != 0) {
                testFail = true;
            } else {
                value = *(int *)((char *)returnedData + 1);
                if (value != (i + i % 500)) {
                    testFail = true;
                }
            }
            // sender_location
        } else if (attrID == 2) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(readCount) + 9), 4) != 0) {
                testFail = true;
            } else {
                fvalue = *(float *)((char *)returnedData + 1);
                if (fvalue != ((float) i + 5000)) {
                    testFail = true;
                }
            }
            // send_time
        } else if (attrID == 3) {
            if (memcmp(((char *)returnedData + 1), ((char *)tuples.at(readCount) + 13), 4) != 0) {
                testFail = true;
            }
            // referred_topics
        } else if (attrID == 4) {
            if (memcmp(((char *)returnedData + 5), ((char *)tuples.at(readCount) + 21), 15) != 0) {
                testFail = true;
            } else {
                std::string strToCheck (((char *)returnedData + 5), 15);
                if (strToCheck.compare(referred_topicsToCheck) != 0) {
                    testFail = true;
                }
            }
            // message_text
        } else if (attrID == 5) {
            if (memcmp(((char *)returnedData + 5), ((char *)tuples.at(readCount) + 40), 15) != 0) {
                testFail = true;
            } else {
                std::string strToCheck (((char *)returnedData + 5), 15);
                if (strToCheck.compare(msgToCheck) != 0) {
                    testFail = true;
                }
            }
        }
        
        if (testFail) {
            cerr << "***** RM Test Case Private 6 failed on " << i << "th tuple - attr: " << attrID << "*****" << endl << endl;
            free(returnedData);
            for(int j = 0; j < numTuples; j++)
            {
                free(tuples[j]);
            }
            rc = rm->deleteTable(tableName);
            remove("rids_file");
            remove("user_ids_file");
            
            return -1;
        }
        readCount++;
    }
    
    free(returnedData);
    for(int i = 0; i < updateCount; i++)
    {
        free(tuples[i]);
    }
    
    cerr << "***** RM Test Case Private 6 finished. The result will be examined. *****" << endl << endl;
    
    return 0;
}

int TEST_RM_PRIVATE_7(const string &tableName)
{
    // Functions tested
    // 1. Scan Table
    cerr << endl << "***** In RM Test Case Private 7 *****" << endl;
    
    RID rid;
    vector<string> attributes;
    void *returnedData = malloc(300);
    
    void *value = malloc(20);
    string msg = "UpdatedMsg00100";
    int msgLength = 15;
    
    memcpy((char *)value, &msgLength, 4);
    memcpy((char *)value + 4, msg.c_str(), msgLength);
    
    string attr = "message_text";
    attributes.push_back("sender_location");
    attributes.push_back("send_time");
    
    RM_ScanIterator rmsi2;
    RC rc = rm->scan(tableName, attr, EQ_OP, value, attributes, rmsi2);
    if(rc != success) {
        free(returnedData);
        cerr << "***** RM Test Case Private 7 failed. *****" << endl << endl;
        return -1;
    }
    
    float sender_location = 0.0;
    float send_time = 0.0;
    bool nullBit = false;
    int counter = 0;
    
    while(rmsi2.getNextTuple(rid, returnedData) != RM_EOF)
    {
        counter++;
        if (counter > 1) {
            cerr << "***** A wrong entry was returned. RM Test Case Private 7 failed *****" << endl << endl;
            rmsi2.close();
            free(returnedData);
            free(value);
            return -1;
        }
        
        sender_location = *(float *)((char *)returnedData + 1);
        send_time = *(float *)((char *)returnedData + 5);
        
        if (!(sender_location == 5100.0 || send_time == 7100.0))
        {
            cerr << "***** A wrong entry was returned. RM Test Case Private 7 failed *****" << endl << endl;
            rmsi2.close();
            free(returnedData);
            free(value);
            return -1;
        }
    }
    
    rmsi2.close();
    free(returnedData);
    free(value);
    
    cerr << "***** RM Test Case Private 7 finished. The result will be examined. *****" << endl << endl;
    
    return 0;
}

RC TEST_RM_PRIVATE_8(const string &tableName)
{
    // Functions tested
    // 1. Delete Tuples
    // 2. Scan Empty table
    // 3. Delete Table
    cerr << endl << "***** In RM Test Case Private 8 *****" << endl;
    
    RC rc;
    RID rid;
    int numTuples = 1000;
    void *returnedData = malloc(300);
    vector<RID> rids;
    vector<string> attributes;
    
    attributes.push_back("tweetid");
    attributes.push_back("userid");
    attributes.push_back("sender_location");
    attributes.push_back("send_time");
    attributes.push_back("referred_topics");
    attributes.push_back("message_text");
    
    readRIDsFromDisk(rids, numTuples);
    
    for(int i = 0; i < numTuples; i++)
    {
        rc = rm->deleteTuple(tableName, rids[i]);
        if(rc != success) {
            free(returnedData);
            cout << "***** RelationManager::deleteTuple() failed. RM Test Case Private 8 failed *****" << endl << endl;
            return -1;
        }
        
        rc = rm->readTuple(tableName, rids[i], returnedData);
        if(rc == success) {
            free(returnedData);
            cout << "***** RelationManager::readTuple() should fail at this point. RM Test Case Private 8 failed *****" << endl << endl;
            return -1;
        }
        
        if (i % 1000 == 0){
            cout << (i+1) << " / " << numTuples << " have been processed." << endl;
        }
    }
    cout << "All records have been processed." << endl;
    
    // Set up the iterator
    RM_ScanIterator rmsi3;
    rc = rm->scan(tableName, "", NO_OP, NULL, attributes, rmsi3);
    if(rc != success) {
        free(returnedData);
        cout << "***** RelationManager::scan() failed. RM Test Case Private 8 failed. *****" << endl << endl;
        return -1;
    }
    
    if(rmsi3.getNextTuple(rid, returnedData) != RM_EOF)
    {
        cout << "***** RM_ScanIterator::getNextTuple() should fail at this point. RM Test Case Private 8 failed. *****" << endl << endl;
        rmsi3.close();
        free(returnedData);
        return -1;
    }
    rmsi3.close();
    free(returnedData);
    
    // Delete a Table
    rc = rm->deleteTable(tableName);
    if(rc != success) {
        cout << "***** RelationManager::deleteTable() failed. RM Test Case Private 8 failed. *****" << endl << endl;
        return -1;
    }
    
    cerr << "***** RM Test Case Private 8 finished. The result will be examined. *****" << endl << endl;
    return 0;
}

RC TEST_RM_PRIVATE_9(const string &tableName)
{
    // Functions tested
    // An attempt to modify System Catalogs tables - should fail
    cerr << endl << "***** In RM Test Case Private 9 *****" << endl;
    
    RID rid;
    vector<Attribute> attrs;
    void *returnedData = malloc(200);
    
    RC rc = rm->getAttributes(tableName, attrs);
    assert(rc == success && "RelationManager::getAttribute() should not fail.");
    
    // print attribute name
    cerr << "Tables table: (";
    for(unsigned i = 0; i < attrs.size(); i++)
    {
        if (i < attrs.size() - 1)
            cerr << attrs[i].name << ", ";
        else
            cerr << attrs[i].name << ")" << endl << endl;
    }
    
    // Try to insert a row - should fail
    int offset = 0;
    int nullAttributesIndicatorActualSize = getActualByteForNullsIndicator(attrs.size());
    unsigned char *nullsIndicator = (unsigned char *) malloc(nullAttributesIndicatorActualSize);
    memset(nullsIndicator, 0, nullAttributesIndicatorActualSize);
    
    void *buffer = malloc(1000);
    
    // Nulls-indicator
    memcpy((char *)buffer + offset, nullsIndicator, nullAttributesIndicatorActualSize);
    offset += nullAttributesIndicatorActualSize;
    
    int intValue = 0;
    int varcharLength = 7;
    string varcharStr = "Testing";
    float floatValue = 0.0;
    
    for (unsigned i = 0; i < attrs.size(); i++) {
        // Generating INT value
        if (attrs[i].type == TypeInt) {
            intValue = 9999;
            memcpy((char *) buffer + offset, &intValue, sizeof(int));
            offset += sizeof(int);
        } else if (attrs[i].type == TypeReal) {
            // Generating FLOAT value
            floatValue = 9999.9;
            memcpy((char *) buffer + offset, &floatValue, sizeof(float));
            offset += sizeof(float);
        } else if (attrs[i].type == TypeVarChar) {
            // Generating VarChar value
            memcpy((char *) buffer + offset, &varcharLength, sizeof(int));
            offset += sizeof(int);
            memcpy((char *) buffer + offset, varcharStr.c_str(), varcharLength);
            offset += varcharLength;
        }
    }
    
    rc = rm->insertTuple(tableName, buffer, rid);
    if (rc == success) {
        cerr << "***** [FAIL] The system catalog should not be altered by a user's insertion call. RM Test Case Private 9 failed. *****" << endl;
        free(returnedData);
        free(buffer);
        return -1;
    }
    
    // Try to delete the system catalog
    rc = rm->deleteTable(tableName);
    if (rc == success){
        cerr << "***** [FAIL] The system catalog should not be deleted by a user call. RM Test Case Private 9 failed. *****" << endl;
        free(returnedData);
        free(buffer);
        return -1;
    }
    
    // Set up the iterator
    RM_ScanIterator rmsi;
    vector<string> projected_attrs;
    if (attrs[1].name == "table-name") {
        projected_attrs.push_back(attrs[1].name);
    } else {
        cerr << "***** [FAIL] The system catalog implementation is not correct. RM Test Case Private 9 failed. *****" << endl;
        free(returnedData);
        free(buffer);
        return -1;
    }
    
    rc = rm->scan(tableName, "", NO_OP, NULL, projected_attrs, rmsi);
    assert(rc == success && "RelationManager::scan() should not fail.");
    
    int counter = 0;
    while(rmsi.getNextTuple(rid, returnedData) != RM_EOF)
    {
        counter++;
    }
    rmsi.close();
    
    // At least two system catalog tables exist (Tables and Columns)
    if (counter < 3) {
        cerr << "***** [FAIL] The system catalog implementation is not correct. RM Test Case Private 9 failed. *****" << endl;
        free(returnedData);
        free(buffer);
        return -1;
    }
    
    cerr << "***** RM Test Case Private 9 finished. The result will be examined. *****" << endl << endl;
    free(returnedData);
    free(buffer);
    return 0;
}

int main()
{
    /* resourceManager declaration and catalog creation is done in rm_test_util.h */
    
    createTable("tbl_private_0");
    TEST_RM_PRIVATE_0("tbl_private_0");
    
    createTweetTable("tbl_private_1");
    TEST_RM_PRIVATE_1("tbl_private_1");
    
    createTweetTable("tbl_private_2");
    TEST_RM_PRIVATE_2("tbl_private_2");
    
    TEST_RM_PRIVATE_3("tbl_private_1");
    
    TEST_RM_PRIVATE_4("tbl_private_1");
    
    TEST_RM_PRIVATE_5("tbl_private_2");
    
    TEST_RM_PRIVATE_6("tbl_private_1");
    
    TEST_RM_PRIVATE_7("tbl_private_1");
    
    TEST_RM_PRIVATE_8("tbl_private_1");
    
    // "TABLE" is one of the catalog tables.
    TEST_RM_PRIVATE_9("TABLE");
    
    return 0;
}
