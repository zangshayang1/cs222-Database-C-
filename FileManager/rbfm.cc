#include "rbfm.h"

RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = nullptr;

RecordBasedFileManager* RecordBasedFileManager::instance()
{
    if(!_rbf_manager)
    _rbf_manager = new RecordBasedFileManager();
    
    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager()
{
    _pbf_manager = PagedFileManager::instance();
}

RecordBasedFileManager::~RecordBasedFileManager()
{
    delete & _rbf_manager;
}

/*
 The following file-based operations are entired built on top of FileBasedManager()
 */
RC RecordBasedFileManager::createFile(const string &fileName) {
    return _pbf_manager->createFile(fileName);
}

RC RecordBasedFileManager::destroyFile(const string &fileName) {
    return _pbf_manager->destroyFile(fileName);
}

RC RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
    return _pbf_manager->openFile(fileName, fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
    return _pbf_manager->closeFile(fileHandle);
}

/* ---------------------------------------------------------------------------------------
 General
 
 Utils
 
 Defined
 
 Below
--------------------------------------------------------------------------------------- */



short getTotalSlotsNum(const void * data) {
    short totSlots;
    memcpy(&totSlots, (char*)data + SLOT_NUM_INFO_POS, sizeof(short));
    return totSlots;
}

short getFreeOffset(const void * data) {
    short offset;
    memcpy(&offset, (char*)data + FREE_SPACE_INFO_POS, sizeof(short));
    return offset;
}

RC putFreeOffset(const void * data, const short & offset) {
    memcpy((char*)data + FREE_SPACE_INFO_POS, & offset, sizeof(short));
    return 0;
}

RC putTotalSlotsNum(const void * data, const short & totSlots) {
    memcpy((char*)data + SLOT_NUM_INFO_POS, & totSlots, sizeof(short));
    return 0;
}

short getRecOffset(const void * data, const SlotNum & slotIdx) {
    short offset;
    memcpy(&offset, (char*)data + RIGHT_MOST_SLOT_OFFSET - slotIdx * sizeof(int), sizeof(short));
    return offset;
}

short getRecLength(const void * data, const SlotNum & slotIdx) {
    short recLen;
    memcpy(&recLen, (char*)data + RIGHT_MOST_SLOT_OFFSET - slotIdx * sizeof(int) + sizeof(short), sizeof(short));
    return recLen;
}

SlotNum nextAvaiSlot(const void * buffer) {
    short totSlots = getTotalSlotsNum(buffer);
    SlotNum i = 0;
    while (i < totSlots) {
        short recOffset = getRecOffset(buffer, i);
        short recLength = getRecLength(buffer, i);
        if (recOffset == SLOT_OFFSET_CLEAN && recLength == SLOT_RECLEN_CLEAN) {
            break;
        }
        i++;
    }
    return i;
}

RC putRecOffset(void * data, const SlotNum & slotIdx, const short & offset) {
    memcpy((char*)data + RIGHT_MOST_SLOT_OFFSET - slotIdx * sizeof(int), & offset, sizeof(short));
    return 0;
}

RC putRecLength(void * data, const SlotNum & slotIdx, const short & length) {
    memcpy((char*)data + RIGHT_MOST_SLOT_OFFSET - slotIdx * sizeof(int) + sizeof(short), & length, sizeof(short));
    return 0;
}


/*
 the two functions, namely, compressed() and decompressed() work as following:
     Beacon = compressed(RID)
         -> take the first 3 bytes of rid.pageNum and put it into beacon.cpsdPageNum
         -> take the first 2 bytes of rid.slotNum and put it into beacon.cosdSlotNum
 *** Special Note: (BigEndian VS LittleEndian) we gave up the most significant byte in rid.pageNum because we believe the rest bytes can represent enough space for this project. Depending on your machine hardware, you need to configure which byte is the most significant byte in an unsigned int type of data structure. Here the implementation indicates we are using LittleEndian.
 
     RID = decompressed(Beacon)
         -> simply reverse
 */

Beacon compressed(const RID & rid) {
    // rid.pageNum -> 4-byte unsigned
    Beacon beacon;
    // beacon.cpsdPageNum -> unsigned char[3]
    memcpy(beacon.cpsdPageNum, &rid.pageNum, 3);
    // beacon.cpsdSlotNum -> unsigned char[2]
    memcpy(beacon.cpsdSlotNum, &rid.slotNum, 2);

    return beacon;
}

RID decompressed(const Beacon & beacon) {
    RID rid;
    rid.pageNum = 0; // init as all 0
    rid.slotNum = 0; // init as all 0
    memcpy(&rid.pageNum, beacon.cpsdPageNum, 3);
    memcpy(&rid.slotNum, beacon.cpsdSlotNum, 2);
    return rid;
}

bool fileHandleNotExists(FileHandle &fileHandle) {
    return (fileHandle.pFile == nullptr);
}

bool recordDescriptorNotExists(const vector<Attribute> &recordDescriptor) {
    return (recordDescriptor.empty());
}

bool pageNumInvalid(FileHandle & fileHandle, const PageNum & pageNum) {
    unsigned totalPageNum = fileHandle.getNumberOfPages();
    // pageNum is type of unsigned int, thus no need to compare with 0.
    return (pageNum >= totalPageNum);
}

short getSlotsLeftBound(const void * buffer,
                        const short & totSlots) {
    if (totSlots == 0) {
        // handle empty page
        return RIGHT_MOST_SLOT_OFFSET; // 4092
    }
    SlotNum slotIdx = 0;
    short counter = 0;
    short recOffset;
    short recLength;
    while (counter < totSlots) {
        recOffset = getRecOffset(buffer, slotIdx);
        recLength = getRecLength(buffer, slotIdx);
        if (recOffset == SLOT_OFFSET_CLEAN &&  recLength == SLOT_RECLEN_CLEAN) {
            // nullified slot, doesn't count towards totSlots
            slotIdx += 1; // move slotIdx left by 1
        }
        else if (recOffset == SLOT_OFFSET_CLEAN || recLength == SLOT_RECLEN_CLEAN) {
            throw("UNEXPECTED");
        }
        else {
            slotIdx += 1;
            counter += 1;
        }
    }
    /*
     This function should handle 3 cases:
         1. the leftmost slot is the last "alive" slot, where a valid RID is stored.
         2. the leftmost slot(s) is one or more CLEANED_SLOT, where there used to be a RID but it got deleted and new RIDs haven't entered.
         3. For above 2 cases, one more byte left to the leftmost slot could be EMPTY_BYTE or filled with record data.
     Now, how to find the left cut off?
         1. The above codes run counter out of totSlots
         2. The below codes recognize CLEAN_SLOT and extend left bound accordingly. In any other cases, the left bound shouldn't be extended.
    */
    recOffset = getRecOffset(buffer, slotIdx);
    recLength = getRecLength(buffer, slotIdx);
    while (recOffset == SLOT_OFFSET_CLEAN &&  recLength == SLOT_RECLEN_CLEAN) {
        slotIdx++; // advance to the next one
        recOffset = getRecOffset(buffer, slotIdx);
        recLength = getRecLength(buffer, slotIdx);
    }
    
    return RIGHT_MOST_SLOT_OFFSET - (slotIdx - 1) * sizeof(int);
}

bool slotNumInvalid(const void * buffer, const SlotNum & slotNum) {
    short leftMostOffset = getSlotsLeftBound(buffer, getTotalSlotsNum(buffer));
    // SlotNum unsigned typed, no need to check < 0
    // slotNum index should never fall into the left side of leftMostOffset
    return (RIGHT_MOST_SLOT_OFFSET - slotNum * sizeof(int) < leftMostOffset);
}

bool recordDeleted(const void * buffer, const RID & rid) {
    short recOffset = getRecOffset(buffer, rid.slotNum);
    short recLength = getRecLength(buffer, rid.slotNum);
    
    if (recOffset == SLOT_OFFSET_CLEAN && recLength == SLOT_RECLEN_CLEAN) {
        return true;
    }
    else if (recOffset == SLOT_OFFSET_CLEAN || recLength == SLOT_RECLEN_CLEAN) {
        throw("Slots info got messed up.");
    }
    else {
        return false;
    }
}

bool recordRelocated(const void * buffer, const RID & rid)
{
    short recLength = getRecLength(buffer, rid.slotNum);
    if (recLength == (short)BEACON_SIZE) {
        return true;
    }
    else {
        return false;
    }
}

void * getRecordStraight(const void * buffer, const RID & rid, short & recLen)
{
    recLen = getRecLength(buffer, rid.slotNum); // find out the length
    short recOfs = getRecOffset(buffer, rid.slotNum);
    void * record = malloc(recLen);
    memcpy(record, (char*)buffer + recOfs, recLen);
    return record;
}

void * getRecordRecursive(FileHandle & fileHandle,
                 const RID & rid,
                 RID & realRid,
                 short & realRecLen) {
    void * buffer = malloc(PAGE_SIZE);
    fileHandle.readPage(rid.pageNum, buffer);
    short recOfs = getRecOffset(buffer, rid.slotNum);
    short recLen = getRecLength(buffer, rid.slotNum);
    if (recordDeleted(buffer, rid)) {
        // missing piece
        void * record = nullptr;
        return record;
    }
    if (recLen > (short)BEACON_SIZE) {
        void * record = malloc(recLen);
        memcpy(record, (char*)buffer + recOfs, (size_t) recLen);
        realRid.pageNum = rid.pageNum;
        realRid.slotNum = rid.slotNum;
        realRecLen = recLen;
        free(buffer);
        return record;
    }
    else if (recLen == (short)BEACON_SIZE) {
        Beacon beacon;
        memcpy(& beacon, (char*)buffer + recOfs, (size_t)BEACON_SIZE);
        RID newRid = decompressed(beacon);
        free(buffer);
        return getRecordRecursive(fileHandle, newRid, realRid, realRecLen);
    }
    else {
        throw("There shouldn't be a record whose length < 5.");
    }
}



/* ---------------------------------------------------------------------------------------
 General
 
 Utils
 
 Defined
 
 Above
 --------------------------------------------------------------------------------------- */

// draw a distinct line between getTotalSlotsNum() and getTotalUsedSlotsNum().
// the former doesn't count those slots filled with SLOT_OFFSET_CLEAN and SLOT_RECLEN_CLEAN but the latter does.
short getTotalUsedSlotsNum(const void * buffer)
{
    short leftBound = getSlotsLeftBound(buffer, getTotalSlotsNum(buffer));
    return (short) ((RIGHT_MOST_SLOT_OFFSET - leftBound) / sizeof(int) + 1);
}
// these two methods are defined for different purpose:
// 1. the above one is defined as a helper function so it can be used by RBFM_ScanIterator::loadNxtRecOnSlot()
// 2. the below one is defined as a public function so it can be used in RM module
// Common thing is they are built on top of two utility functions defined in this module.
short RecordBasedFileManager::getTotalUsedSlotsNum(const void * buffer)
{
    short leftBound = getSlotsLeftBound(buffer, getTotalSlotsNum(buffer));
    return (short) ((RIGHT_MOST_SLOT_OFFSET - leftBound) / 2 + 1);
}

// This func checks the absolute amount of freespace on the page pointed by fileHandle + pageNum
// freeSpace -> itself is a pointer variable pointing to the address passed in, in our case, [11]
short checkForSpace(FileHandle & fileHandle,
                    const PageNum & pageNum) {
    void* buffer = malloc(PAGE_SIZE);
    fileHandle.readPage(pageNum, buffer);
    short freeSpaceOffset = getFreeOffset(buffer);
    short leftMostSlotOffset = getSlotsLeftBound(buffer, getTotalSlotsNum(buffer));
    short freeSpaceAmount = leftMostSlotOffset - freeSpaceOffset;
    free(buffer);
    return freeSpaceAmount;
}


void * RecordBasedFileManager::decodeMetaFrom(const void* data,
                                              const vector<Attribute> & recordDescriptor,
                                              short & recordLen)
{    
    // void* record (pointer to be returned) = malloc(metadata + actualData);
    auto fieldLength = (short) recordDescriptor.size();
    short metaLength = sizeof(short) * fieldLength;
    void* meta = malloc((size_t) metaLength);
    // compute the number of bytes taken to repr NULL fields
    auto n_bytes = (short) ceil((float)fieldLength / BITES_PER_BYTE);
    short dataLength = n_bytes; // points to where the first actual data byte located
    
    short i = 0;
    while (i < n_bytes) {
        char metaByte; // copy out each byte that contains "isNull" info about 8 fields of the data.
        memcpy(&metaByte, (char*)data + i, 1);
        
        short j = 0;
        while (j < BITES_PER_BYTE) {
            auto mask = (unsigned char) (0x80 >> j); // 1000 0000 >> j
            short target_field_idx = i * (short) BITES_PER_BYTE + j;
            // check eligibility
            if (target_field_idx >= fieldLength) {
                break;
            }
            // if current metaByte has j(th) bit being 1 -> (i * 8 + j)th field is NULL;
            if ((metaByte & mask) == mask) {
                // if the (i * 8 + j)th field is NULL, set [2 * (i * 8 + j), 2 * (i * 8 + j) + 2] in "meta" memory as 0.
                memcpy((char*)meta + sizeof(short) * target_field_idx, & ATTR_NULL_FLAG, sizeof(short));
                j++;
                continue;
            }
            // if this field is not NULL
            switch (recordDescriptor[target_field_idx].type) {
                case TypeInt:
                    dataLength += (short) sizeof(int);
                    break;
                case TypeReal:
                    dataLength += (short) sizeof(float);
                    break;
                case TypeVarChar:
                    dataLength += _utils->getStringFrom(data, dataLength).length();
                    dataLength += (short) sizeof(int);
                    break;
                default:
                    break;
            }
            // else "meta" memory records the position where this field of data ends in each 2bytes slot.
            short newEndsBy = dataLength - n_bytes + metaLength;
            memcpy((char*)meta + sizeof(short) * target_field_idx, &newEndsBy, sizeof(short));
            j++;
        }
        i++;
    }
    // now dataLength has been computed, record = [meta + data - n_byte], where dataLength = data - n_byte
    recordLen = metaLength + dataLength - n_bytes;
    // void * record -> [meta1, meta2,..., data1, data2,...]
    void * record = malloc(recordLen);
    memcpy(record, meta, (size_t) metaLength);
    // dataLength = n_bytes + actualData_length
    memcpy((char*)record + metaLength, (char*)data + n_bytes, (size_t) (dataLength - n_bytes));
    
    free(meta);
    return record;
}



int findNextAvaiPage(FileHandle & fileHandle,
                         const short & recordLen) {
    PageNum totalPageNum = fileHandle.getNumberOfPages();
    
    // if no page was ever allocated
    if ( totalPageNum == 0) {
        return PAGENUM_UNAVAILABLE;
    }
    // page indexing starts by 0!!!!!
    int i = 0;
    while (i < totalPageNum) {
        short spaceLeftEmpty = checkForSpace(fileHandle, i);
        // when freeSpace gets out of the above func, the 2bytes of memory were filled by the page's freespace value
        if (spaceLeftEmpty >= recordLen + sizeof(int)) { // freeSpace >= recordLen + oneSlotSpace
            return i;
        }
        i++;
    }
    // if no page is available
    return PAGENUM_UNAVAILABLE;
}

// this helper function is shared by insertIntoNewPage() and insertIntoPage()
RC insertIntoPageHelper(void * buffer,
                        const void * record,
                        const short & recOffset,
                        const short & recordLen,
                        const SlotNum & slotIdx) {
    
    // fill in slot info
    putRecOffset(buffer, slotIdx, recOffset);
    putRecLength(buffer, slotIdx, recordLen);
    
    // fill in record data
    memcpy((char*)buffer + recOffset, record, (size_t) recordLen);
    // fill in slotNum
    putTotalSlotsNum(buffer, getTotalSlotsNum(buffer) + 1);
    // fill in freeSpaceOffset
    putFreeOffset(buffer, getFreeOffset(buffer) + recordLen);
    
    return 0;
}

RC insertIntoNewPage(FileHandle & fileHandle,
                     const void * record,
                     RID &rid,
                     const short & recordLen) {
    
    void * buffer = malloc(PAGE_SIZE);
    // without the following memset(), the entire chunk of memory will remain uninitialized
    // ERROR: Syscall param write(buf) points to uninitialised byte(s)
    memset(buffer, EMPTY_BYTE, PAGE_SIZE);
    
    // init totalSlotsNum
    putTotalSlotsNum(buffer, (short) 0);
    // init freeSpaceOffset
    putFreeOffset(buffer, (short) 0);
    // init slotNum, indexing starts by 0
    rid.slotNum = (SlotNum) 0;
    
    // insert the record into buffer at 0 offset and fill in slotNum 0 with correct recordOffset and recordLen
    insertIntoPageHelper(buffer, record, (short) 0, recordLen, rid.slotNum);
    
    fileHandle.appendPage(buffer);
    
    rid.pageNum = fileHandle.getNumberOfPages() - 1; // indexing starts by 0
    
    free(buffer);
    return 0;
}

RC insertIntoPage(FileHandle & fileHandle,
                  const PageNum & next_avai_page,
                  const void * record,
                  RID & rid,
                  const short & recordLen) {
    void * buffer = malloc(PAGE_SIZE);
    fileHandle.readPage(next_avai_page, buffer);
    
    rid.pageNum = next_avai_page;
    rid.slotNum = nextAvaiSlot(buffer);
    
    // insert record into buffer at page rid.pageNum, recordOffset -> freeSpaceOffset in the buffer, fill in slot info at rid.slotNum
    insertIntoPageHelper(buffer, record, getFreeOffset(buffer), recordLen, rid.slotNum);
    
    fileHandle.writePage(next_avai_page, buffer);
    
    free(buffer);
    return 0;
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle,
                                        const vector<Attribute> &recordDescriptor,
                                        const void *data,
                                        RID &rid) {
    // check fileHandler
    if (fileHandleNotExists(fileHandle) || recordDescriptorNotExists(recordDescriptor)) {
        return -1;
    }
    // must first find recordLen and then find if a suitable page exists.
    short recordLen;
    // you cannot pass in a uninitialized pointer
    void* record = decodeMetaFrom(data, recordDescriptor, recordLen);
    
    int nxtAvaiPage = findNextAvaiPage(fileHandle, recordLen);
    if (nxtAvaiPage == PAGENUM_UNAVAILABLE) {
        // if no page was ever allocated, meaning this is the first ever record insertion,
        // OR if no page was found to have enough capacity to hold this record
        // execute the following
        insertIntoNewPage(fileHandle, record, rid, recordLen);
    }
    else {
        insertIntoPage(fileHandle, nxtAvaiPage, record, rid, recordLen);
    }
    
    free(record);
    return 0;
}
// ---------------------------------------------------------------------------------------

RC encodeMetaInto(void * data,
                  const void * record,
                  const vector<Attribute> & recordDescriptor)
{
    short fieldNum = recordDescriptor.size();
    // split record into meta and actualData two parts
    short metaLen = sizeof(short) * fieldNum;
    
    short i = 0;
    auto n_bytes = (short) ceil((float)fieldNum / BITES_PER_BYTE);
    void * encodedMeta = malloc((size_t) n_bytes);
    short dataLength = metaLen;
    
    while (i < n_bytes) {
        char thisByte = 0x0;
        short j = 0;
        while (j < BITES_PER_BYTE) {
            auto mask = (unsigned char) (0x80 >> j); // 1000 0000
            short target_field_idx = i * (short) BITES_PER_BYTE + j;
            // check eligibility
            if (target_field_idx >= fieldNum) {
                break;
            }
            
            short curtFieldEnds;
            memcpy(& curtFieldEnds, (char*)record + target_field_idx * sizeof(short), sizeof(short));
            
            if (curtFieldEnds == ATTR_NULL_FLAG) {
                // 0000 0000 | 1000 0000 -> 1000 0000
                // 1000 0000 | 0100 0000 -> 1100 0000
                // if NULL bit is turned on, mask thisByte
                thisByte = (thisByte | mask);
            }
            else {
                // else update dataLength with record meta
                dataLength = curtFieldEnds;
            }
            j++;
        }
        // burn thisByte into encodedMeta
        memcpy((char*)encodedMeta + i, &thisByte, sizeof(char));
        i++;
    }
    // fill in encoded meta
    memcpy(data, encodedMeta, (size_t) n_bytes);
    free(encodedMeta);
    // fill in actual data
    memcpy((char*)data + n_bytes, (char*)record + metaLen, (size_t) (dataLength - metaLen));

    return 0;
}


RC RecordBasedFileManager::readRecord(FileHandle &fileHandle,
                                      const vector<Attribute> &recordDescriptor,
                                      const RID &rid,
                                      void *data) {
    // check fileHandler
    if (fileHandleNotExists(fileHandle) || recordDescriptorNotExists(recordDescriptor)) {
        return -1;
    }
    // check page number
    if (pageNumInvalid(fileHandle, rid.pageNum)) {
        return -1;
    }
    // read page in
    void * buffer = malloc(PAGE_SIZE);
    fileHandle.readPage(rid.pageNum, buffer);
    // check slot number
    if (slotNumInvalid(buffer, rid.slotNum)) {
        return -1;
    }
    // check if the record still there
    if (recordDeleted(buffer, rid)) {
        return -1;
    }

    // recursively looking for the actual record since multiple updates could potentially lead to multiple beacon-guided locators
    RID realRid;
    short realRecLen;
    void * record = getRecordRecursive(fileHandle, rid, realRid, realRecLen);
    
    if (record == nullptr) {
        // after going into the page and recursively looking for the record, it found the record was deleted.
        return -1;
    }
    
    encodeMetaInto(data, record, recordDescriptor);
    
    free(buffer);
    free(record);
    
    return 0;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor,
                                       const void *data) {
    string output;
    string tab = "\t";
    string newline = "\n";
    string nullstr = "NULL";
    
    auto fieldNum = (short) recordDescriptor.size();
    auto n_bytes = (short) ceil((float)fieldNum / BITES_PER_BYTE);
    short offset = n_bytes;
    
    short i = 0;
    while (i < n_bytes) {
        char metaByte = 0x0;
        memcpy(&metaByte, (char*)data + i, sizeof(char));
        short j = 0;
        while (j < BITES_PER_BYTE) {
            short target_field_idx = i * (short) BITES_PER_BYTE + j;
            // check eligibility
            if (target_field_idx >= fieldNum) {
                break;
            }
            string fieldName = recordDescriptor[target_field_idx].name;
            // append output
            output += fieldName;
            output += tab;
            auto mask = (unsigned char) (0x80 >> j); // 1000 0000 -> 0100 0000
            if ((metaByte & mask) == mask) {
                output += nullstr;
                output += newline;
                j++;
                continue;
            }
            switch (recordDescriptor[target_field_idx].type) {
                case TypeInt: {
                    int intValue;
                    memcpy(&intValue, (char*)data + offset, sizeof(int));
                    offset += (short) sizeof(int);
                    output += to_string(intValue);
                    break;
                }
                case TypeReal: {
                    float floatValue;
                    memcpy(&floatValue, (char*)data + offset, sizeof(float));
                    offset += (short) sizeof(float);
                    output += to_string(floatValue);
                    break;
                }
                case TypeVarChar: {
                    string str = _utils->getStringFrom(data, offset);
                    output += str;
                    offset += (short) sizeof(int);
                    offset += (short) str.length();
                    break;
                }
                default:
                    break;
            }
            output += newline;
            j++;
        }
        i++;
    }
    cout << output << endl;
    return 0;
}

bool allEmptyBytes(const void * buffer,
                   const short & start,
                   const short & length) {
    void * emptyChunk = malloc((size_t) length);
    memset(emptyChunk, EMPTY_BYTE, (size_t)length);
    int diff = memcmp((char*)buffer + start, emptyChunk, (size_t)length);
    // only when diff == 0, it will return !false
    // else, it returns !true.
    return !diff;
}

RC kickinRemainingRecords(void * buffer,
                          const short & breakPoint,
                          const short & breakLength,
                          const short & slotLeftBound) {
    // base case: [breakPoint ... slotsLeftBound] == EMPTY_BYTE
    if (allEmptyBytes(buffer, breakPoint, slotLeftBound - breakPoint)) {
        return 0;
    }
    short nxtRecOffset = breakPoint + breakLength;
    short nxtRecLength = SLOT_RECLEN_CLEAN; // need to be figured out
    SlotNum curtSlotIdx = 0;
    while (RIGHT_MOST_SLOT_OFFSET - curtSlotIdx * sizeof(int) >= slotLeftBound) {
        short curtRecOffset = getRecOffset(buffer, curtSlotIdx);
        if (nxtRecOffset == curtRecOffset) {
            nxtRecLength = getRecLength(buffer, curtSlotIdx);
            break;
        }
        curtSlotIdx++;
    }
    // eligibility check : actually found nxtRecLength
    if (nxtRecLength == SLOT_RECLEN_CLEAN) {
        throw("Didn't find the slot storing nxtRecOffset.");
    }
    // ready to move nxtRec
    memcpy((char*)buffer + breakPoint, (char*)buffer + nxtRecOffset, (size_t) nxtRecLength);
    short newBreakPoint = breakPoint + nxtRecLength;
    // don't forget to move pointers in the slot!
    putRecOffset(buffer, curtSlotIdx, breakPoint);
    putRecLength(buffer, curtSlotIdx, nxtRecLength);
    // create the next break
    memset((char*)buffer + newBreakPoint, EMPTY_BYTE, (size_t)breakLength);
    // starting from newBreakPoint, the remaining length equals to the old breakLength
    // after all, breakLength is not gonna change throughout the process
    kickinRemainingRecords(buffer, newBreakPoint, breakLength, slotLeftBound);
    return 0;
}

RC deleteRecordAndRearrange(void * buffer, const RID & rid) {
    // erase record in memory
    short recLength = getRecLength(buffer, rid.slotNum);
    short recOffset = getRecOffset(buffer, rid.slotNum);
    memset((char*)buffer + recOffset, EMPTY_BYTE, (size_t)recLength);
    
    // reset slot info, freeOffset, totalslotNum, etc
    putRecOffset(buffer, rid.slotNum, SLOT_OFFSET_CLEAN);
    putRecLength(buffer, rid.slotNum, SLOT_RECLEN_CLEAN);
    putTotalSlotsNum(buffer, getTotalSlotsNum(buffer) - 1);
    putFreeOffset(buffer, getFreeOffset(buffer) - recLength);
    
    short totalSlots = getTotalSlotsNum(buffer);
    if (totalSlots == 0) {
        // the page has become empty
        return 0;
    }
    // else fill up the hole
    short slotLeftBound = getSlotsLeftBound(buffer, totalSlots);
    kickinRemainingRecords(buffer, recOffset, recLength, slotLeftBound);
    return 0;
}


RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle,
                                        const vector<Attribute> & recordDescriptor,
                                        const RID &rid) {
    // check fileHandler
    if (fileHandleNotExists(fileHandle) || recordDescriptorNotExists(recordDescriptor)) {
        return -1;
    }
    // check page number
    if (pageNumInvalid(fileHandle, rid.pageNum)) {
        return -1;
    }
    // read page in
    void * buffer = malloc(PAGE_SIZE);
    fileHandle.readPage(rid.pageNum, buffer);
    // check slot number
    if (slotNumInvalid(buffer, rid.slotNum)) {
        return -1;
    }
    // find actual RID
    RID actRid;
    short actRecLen;
    getRecordRecursive(fileHandle, rid, actRid, actRecLen);
    
    fileHandle.readPage(actRid.pageNum, buffer); // reload buffer with actRid
    
    deleteRecordAndRearrange(buffer, actRid);
    
    fileHandle.writePage(actRid.pageNum, buffer);
    
    free(buffer);
    return 0;
}

RC putBeaconIntoBuffer(void * buffer,
                       const SlotNum & slotNum,
                       const short & beaconOfs,
                       const Beacon & beacon) {
    // fill in slot info
    putRecOffset(buffer, slotNum, beaconOfs);
    putRecLength(buffer, slotNum, BEACON_SIZE);
    // fill in record data
    memcpy((char*)buffer + beaconOfs, beacon.cpsdPageNum, (size_t) 3);
    memcpy((char*)buffer + beaconOfs + 3, beacon.cpsdSlotNum, (size_t) 2);
    // fill in slotNum
    putTotalSlotsNum(buffer, getTotalSlotsNum(buffer) + 1);
    // fill in freeSpaceOffset
    putFreeOffset(buffer, beaconOfs + BEACON_SIZE);
    return 0;
}

/*
 Implementation Algorithm:
 
 if the updated record length <= original record length + remaining space, just delete the original and call kickinRemainingRecord() and append the updated one in the end. Don't forget update the slot info.
 else, just delete the original record, allocate 1 byte memory to store a RID pointing to somewhere in another page, and call kickinRemainingRecord(), and put the updated one in the new place.
 
 *** A "regular" record takes at least 6 bytes = [ 2 byte short pointer + 4 byte int data] (Varchar type of data includes 4 byte int indicating its length)
 *** A slot takes 4 bytes
 *** A page (4096 B) will hold at most [4096 / 1000]  = 409 such records.
 *** If we use 3 B to hold pageNum, it allows us to append(2^{24}) pages, which is 512GB, large enough.
 *** If we use 2 B to hold slotNum, it allows us to index (2^{16}) slots, which is 65535, large enough.
 *** Therefore, 5 B can represent new RID. It is also distinguishable from "regular" records, as it is smaller.
 
 NOTE: Three types of RID were involved:
 1. rid : taken as input
 2. actRid : the actual rid indicating where the record is located
 3. newRid : the new rid indicating where the updated record is located
 */
RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle,
                                        const vector<Attribute> &recordDescriptor,
                                        const void *data,
                                        const RID &rid) {
    
    // check fileHandler
    if (fileHandleNotExists(fileHandle) || recordDescriptorNotExists(recordDescriptor)) {
        return -1;
    }
    // check page
    if (pageNumInvalid(fileHandle, rid.pageNum)) {
        return -1;
    }
    // read page in
    void * buffer = malloc(PAGE_SIZE);
    fileHandle.readPage(rid.pageNum, buffer);
    // check slot
    if (slotNumInvalid(buffer, rid.slotNum)) {
        return -1;
    }
    // check if the record still there
    if (recordDeleted(buffer, rid)) {
        return -1;
    }
    // prepare upd record
    short updRecLen;
    void * updRecord = decodeMetaFrom(data, recordDescriptor, updRecLen);
    
    // recursively looking for actual RID and load up actualRecLen
    RID actRid;
    short actRecLen;
    getRecordRecursive(fileHandle, rid, actRid, actRecLen);
    // reload buffer
    fileHandle.readPage(actRid.pageNum, buffer);

    deleteRecordAndRearrange(buffer, actRid);
    // freeSpaceOfs after the rearranging the space
    short freeSpaceOffset = getFreeOffset(buffer);
    short freeSpaceAmount = checkForSpace(fileHandle, actRid.pageNum);
    
    if (freeSpaceAmount >= updRecLen) {
        
        // append the updRecord and put new offset/length at the old slot
        insertIntoPageHelper(buffer, updRecord, freeSpaceOffset, updRecLen, actRid.slotNum);
        
        // insertIntoNewPage() AND insertIntoPage() functions handle flush() operation already.
        fileHandle.writePage(actRid.pageNum, buffer);
    }
    else {
        int nxtAvaiPage = findNextAvaiPage(fileHandle, updRecLen);
        
        RID newRid;
        if (nxtAvaiPage == PAGENUM_UNAVAILABLE) {
            // inside the function, an empty page has been initialized and filled in with records as well as all other info, and flushed to disk.
            insertIntoNewPage(fileHandle, updRecord, newRid, updRecLen);
        }
        else {
            insertIntoPage(fileHandle, nxtAvaiPage, updRecord, newRid, updRecLen);
        }
        // Since not enough space for updRecord, simply put a 5-byte Beacon there, which points to where the updRecord is located.
        Beacon beacon = compressed(newRid);
        putBeaconIntoBuffer(buffer, actRid.slotNum, freeSpaceOffset, beacon);
        
//        insertIntoPageHelper(buffer, & beacon, freeSpaceOffset, (short) BEACON_SIZE, actRid.slotNum);
        
        // if the updated record's size exceeds what the current page could provide, the above code finds another page to store the record. Now, don't forget to flush the current page so that all the current buffer modifications take place.
        fileHandle.writePage(actRid.pageNum, buffer);
    }
    
    free(updRecord);
    free(buffer);
    
    return 0;
}

short fieldLenToMetaLen(const unsigned long & fieldLen) {
    return (short) sizeof(short) * fieldLen;
}
short getAttrOfsFromDecodedRec(const int & i,
                               const void * record,
                               const unsigned long & fieldLen) {
    if (i >= fieldLen || i < 0) {
        throw("Incorrect field index.");
    }
    if (i == 0) {
        short metaLen = fieldLenToMetaLen(fieldLen);
        return metaLen;
    }
    else {
        short offset;
        memcpy(&offset, (char*)record + (i - 1) * sizeof(short), sizeof(short));
        return offset;
    }
}
short getAttrLenFromDecodedRec(const int & i,
                               const void * record,
                               const unsigned long & fieldLen) {
    if (i >= fieldLen || i < 0) {
        throw("Incorrect field index.");
    }
    auto thisOfs = *(short*)((char*)record + i * sizeof(short));
    if (thisOfs == ATTR_NULL_FLAG) {
        return 0;
    }
    if (i == 0) {
        short metaLen = fieldLenToMetaLen(fieldLen);
        auto secRecOfs = *(short*)record;
        return (secRecOfs - metaLen);
    }
    else {
        auto prevOfs = *(short*)((char*)record + (i - 1) * sizeof(short));
        return (thisOfs - prevOfs);
    }
}

RC readAttributeNo(const int i, const void * record, const unsigned long & fieldLen, void * data) {
    short offset = getAttrOfsFromDecodedRec(i, record, fieldLen);
    short length = getAttrLenFromDecodedRec(i, record, fieldLen);
    unsigned char nullIndicator;
    if (length == 0) {
        // the attr value is null;
        nullIndicator = 0x80;
        memcpy(data, & nullIndicator, 1);
        return -1;
    }
    else {
        nullIndicator = 0x0;
        // for any attr, the corresponding null indicator length is always 1.
        memcpy(data, & nullIndicator, 1);
        memcpy((char*)data + 1, (char*)record + offset, (size_t)length);
        return 0;
    }
    
}

RC RecordBasedFileManager::readAttribute(FileHandle &fileHandle,
                                         const vector<Attribute> &recordDescriptor,
                                         const RID &rid,
                                         const string &attributeName,
                                         void *data) {
    // 1. readRecord() checks everything, including fileHandle, recordDescriptor, rid and if the record still exists.
    // 2. readRecord() loads up the below ptr with encoded record
    void * encodedRecord = malloc(PAGE_SIZE);
    if (readRecord(fileHandle, recordDescriptor, rid, encodedRecord) == -1) {
        return -1;
    }
    // 0 is returned anyway after this line
    // because an attr will be fetched even it is a nullIndicator.
    short recordLen;
    void * record = decodeMetaFrom(encodedRecord, recordDescriptor, recordLen);
    const unsigned long fieldLen = recordDescriptor.size();
    
    int attrFound = -1;
    for(int i = 0; i < fieldLen; i++) {
        if (recordDescriptor[i].name == attributeName) {
            attrFound = readAttributeNo(i, record, fieldLen, data); // load data and return 0
        }
    }
    free(encodedRecord);
    free(record);
    return 0;
}


RC RecordBasedFileManager::scan(FileHandle &fileHandle,
                                const vector<Attribute> &recordDescriptor,
                                const string &conditionAttribute,
                                const CompOp compOp,
                                // comparision type such as "<" and "="
                                const void *value,
                                // used in the comparison
                                const vector<string> &attributeNames,
                                // a list of projected attributes
                                RBFM_ScanIterator &rbfm_ScanIterator)
{
    rbfm_ScanIterator.initialize(fileHandle, recordDescriptor, conditionAttribute, compOp, value, attributeNames);
    return 0;
}

// ------------------------------------------------------------

RC RBFM_ScanIterator::initialize(FileHandle &fileHandle,
                                 const vector<Attribute> &recordDescriptor,
                                 const string &conditionAttribute,
                                 const CompOp compOp,
                                 // comparision type such as "<" and "="
                                 const void *value,
                                 // used in the comparison
                                 const vector<string> &attributeNames)
                                 // a list of projected attributes
{
    // add checking statements ?
    // init public variables global to this class
    this->fileHandle = fileHandle;
    this->recordDescriptor = recordDescriptor;
    this->conditionAttribute = conditionAttribute;
    this->compOp = compOp;
    this->value = value;
    this->attributeNames = attributeNames;
    
    for(int i = 0; i < recordDescriptor.size(); i++) {
        this->attrMap[recordDescriptor[i].name] = i;
    }

    this->curtPageNum = 0;
    this->curtSlotNum = 0;
    return 0;
}


RC RBFM_ScanIterator::loadNxtRecOnSlot(RID & rid,
                                       void * decodedRec,
                                       const void * buffer,
                                       const vector<Attribute> & recordDescriptor)
{
    short totUsedSlotsNum = getTotalUsedSlotsNum(buffer);
    while (this->curtSlotNum < totUsedSlotsNum)
    {
        // update rid every iteration!
        rid.slotNum = this->curtSlotNum;
        if (recordDeleted(buffer, rid) || recordRelocated(buffer, rid)) {
            this->curtSlotNum++;
            continue;
        }
        short recLen;
        void * record = getRecordStraight(buffer, rid, recLen);
        // decodeRec = record is NOT going to do the job!
        memcpy(decodedRec, record, recLen);
        // fetch record successful! Now free the holder.
        free(record);
        
        this->curtSlotNum++;
        // advance to next slotNum if getNextRecord() gets called again.
        return 0;
    }
    // failed to fetch next record on this page
    return -1;
    
}

RC RBFM_ScanIterator::loadNxtRecOnPage(RID &rid,
                                       void * decodedRec,
                                       const vector<Attribute> & recordDescriptor )
{
    unsigned totalPageNum = this->fileHandle.getNumberOfPages();
    while (this->curtPageNum < totalPageNum)
    {
        void * buffer = malloc(PAGE_SIZE);
        this->fileHandle.readPage(this->curtPageNum, buffer);
        rid.pageNum = this->curtPageNum;
        // rid.slotNum should be filled while executing loadNxtRecOnSlot()
        
        RC rc = loadNxtRecOnSlot(rid, decodedRec, buffer, recordDescriptor);

        // didn't find a record on curt page
        if (rc == -1) {
            this->curtPageNum++;
            this->curtSlotNum = 0;
            continue;
        }
        // void * decodedRec has been loaded
        
        free(buffer);
        return 0;
    }
    // failed to load the next record because of EOF
    return -1;
}


RC loadDataFor(const int fieldIdx,
               void * projectRec,
               const void * decodedRec,
               short & proRecLen,
               const unsigned long & allAttrNum)
{
    short offset = getAttrOfsFromDecodedRec(fieldIdx, decodedRec, allAttrNum);
    short attrLen = getAttrLenFromDecodedRec(fieldIdx, decodedRec, allAttrNum);
    memcpy((char*)projectRec + proRecLen, (char*)decodedRec + offset, (size_t)attrLen);
    proRecLen += attrLen;
    return 0;
}

RC turnOnBit(void * targetMeta,
             const unsigned long & targetCounter)
{
    short byte = targetCounter / BITES_PER_BYTE;
    short bit = targetCounter % BITES_PER_BYTE;
    
    unsigned char targetMetaByte;
    memcpy(& targetMetaByte, (char*)targetMeta + byte, 1);
    
    auto mask = (unsigned char) (0x80 >> bit);
    
    targetMetaByte = (targetMetaByte | mask);
    memcpy((char*)targetMeta + byte, & targetMetaByte, 1);
    
    return 0;
}
RC loadMetaFor(const int & fieldIdx,
               void * targetMeta,
               const void * allMeta,
               const unsigned long & targetCounter)
{
    short byte = fieldIdx / BITES_PER_BYTE;
    unsigned char allMetaByte;
    memcpy(& allMetaByte, (char*)allMeta + byte, 1);
    
    short bit = fieldIdx % BITES_PER_BYTE;
    auto mask = (unsigned char) (0x80 >> bit);

    if ((allMetaByte & mask) == mask) {
        turnOnBit(targetMeta, targetCounter);
    }
    
    return 0;
}

bool compareInt(const void * attrData,
                const CompOp & compOp,
                const void * value)
{
    switch (compOp) {
        case EQ_OP:
            return (*(int*)attrData == *(int*)value);
        case LT_OP:
            return (*(int*)attrData < *(int*)value);
        case LE_OP:
            return (*(int*)attrData <= *(int*)value);
        case GT_OP:
            return (*(int*)attrData > *(int*)value);
        case GE_OP:
            return (*(int*)attrData >= *(int*)value);
        case NE_OP:
            return (*(int*)attrData != *(int*)value);
        case NO_OP: // just to get rid of warning, it shouldn't be screened in earlier phase
            return true;
        default:
            throw("Unrecognized type of CompOp found!");
            break;
    }
}
bool compareReal(const void * attrData,
                const CompOp & compOp,
                const void * value)
{
    switch (compOp) {
        case EQ_OP:
            return (*(float*)attrData == *(float*)value);
        case LT_OP:
            return (*(float*)attrData < *(float*)value);
        case LE_OP:
            return (*(float*)attrData <= *(float*)value);
        case GT_OP:
            return (*(float*)attrData > *(float*)value);
        case GE_OP:
            return (*(float*)attrData >= *(float*)value);
        case NE_OP:
            return (*(float*)attrData != *(float*)value);
        case NO_OP: // just to get rid of warning, it shouldn't be screened in earlier phase
            return true;
        default:
            throw("Unrecognized type of CompOp found!");
            break;
    }
}

string getStringFromMemStd(const void * data) {
    int strLen = *(int*)data;
    // no Varchar entry takes memory more than 1 page
    char strVal[PAGE_SIZE];
    memcpy(strVal, (char*)data + sizeof(int), (size_t) strLen);
    strVal[strLen] = '\0';
    return string(strVal);
}

bool compareVarChar(const void * attrData,
                const CompOp & compOp,
                const void * value)
{
    
    string attrStr = getStringFromMemStd(attrData);
    string valueStr = getStringFromMemStd(value);
    switch (compOp) {
        case EQ_OP:
            return (attrStr.compare(valueStr) == 0);
        case LT_OP:
            return (attrStr.compare(valueStr) < 0);
        case LE_OP:
            return (attrStr.compare(valueStr) <= 0);
        case GT_OP:
            return (attrStr.compare(valueStr) > 0);
        case GE_OP:
            return (attrStr.compare(valueStr) >= 0);
        case NE_OP:
            return (attrStr.compare(valueStr) != 0);
        case NO_OP: // just to get rid of warning, it shouldn't be screened in earlier phase
            return true;
        default:
            throw("Unrecognized type of CompOp found!");
            break;
    }
}
bool satisfyCondition(const void * decodedRec,
                      const vector<Attribute> & recordDescriptor,
                      const unordered_map<string, int> & attrMap,
                      const string & conditionAttribute,
                      const CompOp & compOp,
                      const void * value)
{
    if (conditionAttribute.compare(string("")) == 0) {
        // no condition specified
        return true;
    }
    if (compOp == NO_OP) {
        // no compOp specified
        return true;
    }
    if (value == NULL || value == nullptr) {
        // no comp value specified
        return true;
    }
    // access mode: attrMap[conditionAttribute] is not allowed when attrMap is const
    int fieldIdx = attrMap.at(conditionAttribute);
    void * attrData = malloc(PAGE_SIZE);
    RC rc = readAttributeNo(fieldIdx, decodedRec, recordDescriptor.size(), attrData);
    if (rc == -1) {
        // if attrData is a nullIndicator
        return (compOp == NO_OP);
    }
    // attrData -> [nullIndicator + data]
    bool rst;
    switch (recordDescriptor[fieldIdx].type) {
        case TypeInt:
            rst = compareInt((char*)attrData + 1, compOp, value);
            free(attrData);
            return rst;
        case TypeReal:
            rst = compareReal((char*)attrData + 1, compOp, value);
            free(attrData);
            return rst;
        case TypeVarChar:
            rst = compareVarChar((char*)attrData + 1, compOp, value);
            free(attrData);
            return rst;
        default:
            throw("Other type of attribute than TypeInt, TypeReal, TypeVarChar.");
            break;
    }
}

RC projectAttributeOnto(void * projectRec,
                        const void * decodedRec,
                        const vector<Attribute> & recordDescriptor,
                        const unordered_map<string, int> & attrMap,
                        const vector<string> & attributeNames)
{
    void * encodedRec = malloc(PAGE_SIZE);
    encodeMetaInto(encodedRec, decodedRec, recordDescriptor);
    // get its encoded version and put into allMeta
    const unsigned long allAttrNum = recordDescriptor.size();
    short a_bytes = ceil((double)allAttrNum / BITES_PER_BYTE);
    void * allMeta = malloc((size_t) a_bytes);
    memcpy(allMeta, encodedRec, (size_t) a_bytes);
    
    // unsigned char targetMeta[t_bytes] -> variable size object cannot be initialized : everything initialized like this sits on stack and their size must be figured out during compile time
    // on the other hand, you can use "new" to initialize a char array with variable size in heap
    const unsigned long targetAttrNum = attributeNames.size();
    short t_bytes = ceil((double)targetAttrNum / BITES_PER_BYTE);
    void * targetMeta = malloc((size_t) t_bytes);
    memset(targetMeta, 0, (size_t) t_bytes);
    
    short proRecLen = t_bytes; // where to start appending field data
    unsigned long targetCounter = 0;
    
    for(string attr : attributeNames) {
        if (attrMap.count(attr) == 0) {
            throw("sanity check");
        }
        int idx = attrMap.at(attr);
        loadMetaFor(idx, targetMeta, allMeta, targetCounter);
        loadDataFor(idx, projectRec, decodedRec, proRecLen, allAttrNum);
        targetCounter++;
    }

    memcpy(projectRec, targetMeta, t_bytes);
    
    free(encodedRec);
    free(allMeta);
    free(targetMeta);
    return 0;
}

// load up rid and data
RC RBFM_ScanIterator::getNextRecord(RID &rid,
                                    void *data)
{
    // get some space
    void * decodedRec = malloc(PAGE_SIZE);
    do {
        // fetch the next decoded record
        RC rc1 = loadNxtRecOnPage(rid, decodedRec, this->recordDescriptor);
        if (rc1 == -1) {
            return RBFM_EOF;
        }
    // check if the attr satisfy the select criteria
    } while (!satisfyCondition(decodedRec,
                               this->recordDescriptor,
                               this->attrMap,
                               this->conditionAttribute,
                               this->compOp,
                               this->value));
    
    // project required attribute to data
    projectAttributeOnto(data,
                         decodedRec,
                         this->recordDescriptor,
                         this->attrMap,
                         this->attributeNames);
    return 0;
};






