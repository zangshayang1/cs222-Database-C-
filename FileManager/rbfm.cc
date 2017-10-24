#include "rbfm.h"

#include "cmath"
#include <iostream>

const short ATTR_NULL_FLAG = -1;
const short SLOT_OFFSET_CLEAN = -2;
const short SLOT_RECLEN_CLEAN = -3;
const int PAGENUM_UNAVAILABLE = -4;
const int EMPTY_BYTE = -5;


RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = nullptr;

RecordBasedFileManager* RecordBasedFileManager::instance()
{
    if(!_rbf_manager)
    _rbf_manager = new RecordBasedFileManager();
    
    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager()
{
    _rbf_manager = nullptr;
    _pbf_manager = PagedFileManager::instance();
}

RecordBasedFileManager::~RecordBasedFileManager()
{
    delete _rbf_manager;
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
 Utils
 
 Defined
 
 Below
--------------------------------------------------------------------------------------- */

void print_char(const unsigned char oneChar) {
    unsigned char mask;
    cout << '[';
    for(int i = 0; i < 8; i++) {
        mask = (0x80 >> i);
        if ((oneChar & mask) == mask) {
            cout << '1';
        }
        else {
            cout << '0';
        }
    }
    cout << ']' << endl;
}
void print_bytes(void *object, size_t size)
{
    // This is for C++; in C just drop the static_cast<>() and assign.
    //    const unsigned char * const bytes = static_cast<const unsigned char *>(object);
    unsigned char data[size];
    memcpy(data, object, size);
    for (int i = 0; i < size; i++) {
        cout << "char " << i << endl;
        print_char(data[i]);
    }
}


// this function returns string content from the given data chunk and offset
// NOTE that the first 4bytes make an int indicator of the strLen
string getStringFrom(const void * data,
                     const short & offset) {
    int strLen;
    memcpy(&strLen, (char*)data + offset, sizeof(int));
    // no Varchar entry takes memory more than 1 page
    char strVal[PAGE_SIZE];
    memcpy(strVal, (char*)data + offset + sizeof(int), (size_t) strLen);
    strVal[strLen] = '\0';
    return string(strVal);
}

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
    memcpy(beacon.cpsdSlotNum, &rid.slotNum, 3);

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
    SlotNum slotIdx = 0;
    short counter = 0;
    while (counter < totSlots) {
        short recOffset = getRecOffset(buffer, slotIdx);
        short recLength = getRecLength(buffer, slotIdx);
        if ( recOffset == SLOT_OFFSET_CLEAN &&  recLength == SLOT_RECLEN_CLEAN) {
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
    return RIGHT_MOST_SLOT_OFFSET - (slotIdx - 1) * sizeof(int);
}

bool slotNumInvalid(const void * buffer, const SlotNum & slotNum) {
    short leftMostOffset = getSlotsLeftBound(buffer, getTotalSlotsNum(buffer));
    // SlotNum unsigned typed, no need to check < 0
    // slotNum index should never fall into the left side of leftMostOffset
    return (RIGHT_MOST_SLOT_OFFSET - slotNum * sizeof(int) < leftMostOffset);
}

bool recordDeleted(void * buffer, const RID & rid) {
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

void * getRecord(FileHandle & fileHandle,
                 const RID & rid,
                 RID & realRid,
                 short & realRecLen) {
    void * buffer = malloc(PAGE_SIZE);
    fileHandle.readPage(rid.pageNum, buffer);
    short recOfs = getRecOffset(buffer, rid.slotNum);
    short recLen = getRecLength(buffer, rid.slotNum);
    //    cout << "inside getRecord()" << endl;
    //    cout << "recOfs: " << recOfs << endl;
    //    cout << "recLen: " << recLen << endl;
    if (recordDeleted(buffer, rid)) {
        // missing piece
        void * record = nullptr;
        return record;
    }
    if (recLen > 5) {
        void * record = malloc(recLen);
        memcpy(record, (char*)buffer + recOfs, (size_t) recLen);
        realRid.pageNum = rid.pageNum;
        realRid.slotNum = rid.slotNum;
        realRecLen = recLen;
        free(buffer);
        return record;
    }
    else if (recLen == 5) {
        Beacon beacon;
        memcpy(& beacon, (char*)buffer + recOfs, 5);
        RID newRid = decompressed(beacon);
        free(buffer);
        return getRecord(fileHandle, newRid, realRid, realRecLen);
    }
    else {
        throw("There shouldn't be a record whose length < 5.");
    }
}

/* ---------------------------------------------------------------------------------------
 Utils
 
 Defined
 
 Above
 --------------------------------------------------------------------------------------- */

// This func checks the absolute amount of freespace on the page pointed by fileHandle + pageNum
// freeSpace -> itself is a pointer variable pointing to the address passed in, in our case, [11]
short checkForSpace(FileHandle & fileHandle,
                    const PageNum & pageNum) {
    void* buffer = malloc(PAGE_SIZE);
    fileHandle.readPage(pageNum, buffer);
    short freeSpaceOffset = getFreeOffset(buffer);
    short totalSlots = getTotalSlotsNum(buffer);
    
    short spaceLeftEmpty = SLOT_NUM_INFO_POS - sizeof(int) * totalSlots - freeSpaceOffset;
    
    free(buffer);
    
    return spaceLeftEmpty;
}


char* RecordBasedFileManager::decodeMetaFrom(const void* data,
                                             const vector<Attribute> & recordDescriptor,
                                             short & recordLen) {
    
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
                    dataLength += getStringFrom(data, dataLength).length();
                    dataLength += (short) sizeof(int);
                    break;
                default:
                    break;
            }
            // else "meta" memory records the position where this field of data ends in each 2bytes slot.
            memcpy((char*)meta + sizeof(short) * target_field_idx, &dataLength, sizeof(short));
            j++;
        }
        i++;
    }
    // now dataLength has been computed, record = [meta + data - n_byte], where dataLength = data - n_byte
    recordLen = metaLength + dataLength - n_bytes;
//    cout << "Inside of decodeMeta(): " << endl;
//    cout << "metaLength: " << metaLength << endl;
//    cout << "dataLength: " << dataLength << endl;
    void * record = malloc((size_t) recordLen);
    // void * record -> [meta1, meta2,..., data1, data2,...]
    memcpy(record, meta, (size_t) metaLength);
    // dataLength = n_bytes + actualData_length
    memcpy((char*)record + metaLength, (char*)data + n_bytes, (size_t) (dataLength - n_bytes));
    
    free(meta);
    return (char*)record;
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

RC RecordBasedFileManager::encodeMetaInto(void * data,
                                          const void * record,
                                          const short & fieldNum,
                                          const short & recordLen) {
    // split record into meta and actualData two parts
    short metaLen = sizeof(short) * fieldNum;
    void * actualData = malloc((size_t) (recordLen - metaLen));
    memcpy(actualData, (char*)record + metaLen, (size_t) (recordLen - metaLen));
    
    short i = 0;
    auto n_bytes = (short) ceil((float)fieldNum / BITES_PER_BYTE);
    void * encodedMeta = malloc((size_t) n_bytes);
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
            short thisField;
            memcpy(&thisField, (char*)record + target_field_idx * sizeof(short), sizeof(short));
            if (thisField == ATTR_NULL_FLAG) {
                // 0000 0000 | 1000 0000 -> 1000 0000
                // 1000 0000 | 0100 0000 -> 1100 0000
                thisByte = (thisByte | mask);
            }
            j++;
        }
        memcpy((char*)encodedMeta + i, &thisByte, sizeof(char));
        i++;
    }
    // fill in encoded meta
    memcpy(data, encodedMeta, (size_t) n_bytes);
    free(encodedMeta);
    // fill in actual data
    memcpy((char*)data + n_bytes, actualData, (size_t) (recordLen - metaLen));
    free(actualData);
    
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
    void * record = getRecord(fileHandle, rid, realRid, realRecLen);
    
    if (record == nullptr) {
        // after going into the page and recursively looking for the record, it found the record was deleted.
        return -1;
    }
    
    encodeMetaInto(data, record, (short) recordDescriptor.size(), realRecLen);
    
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
                    string str = getStringFrom(data, offset);
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
//        cout << "Didn't find the slot storing record offset: " << nxtRecOffset << endl;
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
    
    // fill up the hole
    short slotLeftBound = getSlotsLeftBound(buffer, getTotalSlotsNum(buffer));
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
    getRecord(fileHandle, rid, actRid, actRecLen);
    fileHandle.readPage(rid.pageNum, buffer); // reload buffer
    
    deleteRecordAndRearrange(buffer, actRid);
    
    fileHandle.writePage(actRid.pageNum, buffer);
    
    free(buffer);
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
    getRecord(fileHandle, rid, actRid, actRecLen);
    // reload buffer
    fileHandle.readPage(actRid.pageNum, buffer);

    deleteRecordAndRearrange(buffer, actRid);
    // freeSpaceOfs after the rearranging the space
    short freeSpaceOffset = getFreeOffset(buffer);
    short leftMostSlotOffset = getSlotsLeftBound(buffer, getTotalSlotsNum(buffer));
    short freeSpaceAmount = leftMostSlotOffset - freeSpaceOffset;

//    void * myBuffer = malloc(1024);
//    memcpy(myBuffer, (char*)buffer + 124, 1024);
//    void * myData = malloc(117);
//    encodeMetaInto(myData, myBuffer, 4, 1024);
////    readRecord(fileHandle, recordDescriptor, myRid, myBuffer);
//    printRecord(recordDescriptor, myData);
//    free(myBuffer);
//    free(myData);
    
    if (freeSpaceAmount >= updRecLen) {
        
//        cout << "CHECKING updRecord: " << endl;
//        void* myData = malloc(117);
//        encodeMetaInto(myData, updRecord, 4, 124);
//        printRecord(recordDescriptor, myData);
        
        // append the updRecord and put new offset/length at the old slot
        insertIntoPageHelper(buffer, updRecord, freeSpaceOffset, updRecLen, actRid.slotNum);
        
        // insertIntoNewPage() AND insertIntoPage() functions handle flush() operation already.
        fileHandle.writePage(actRid.pageNum, buffer);
        
//        cout << "CHECKING inserted record: " << endl;
//        void* myData = malloc(117);
//        encodeMetaInto(myData, (char*)buffer + 1148, 4, 124);
//        printRecord(recordDescriptor, myData);
        
    }
    else {
        RID newRid;
        int nxtAvaiPage = findNextAvaiPage(fileHandle, updRecLen);
        
        if (nxtAvaiPage == PAGENUM_UNAVAILABLE) {
            // inside the function, an empty page has been initialized and filled in with records as well as all other info, and flushed to disk.
            insertIntoNewPage(fileHandle, updRecord, newRid, updRecLen);
            
        }
        else {
            insertIntoPage(fileHandle, nxtAvaiPage, updRecord, newRid, updRecLen);
        }
        // Since not enough space for updRecord, simply put a 5-byte Beacon there, which points to where the updRecord is located.
        Beacon beacon = compressed(newRid);
        insertIntoPageHelper(buffer, & beacon, freeSpaceOffset, (short) 5, actRid.slotNum);
        // if the updated record's size exceeds what the current page could provide, the above code finds another page to store the record. Now, don't forget to flush the current page so that all the current buffer modifications take place.
        fileHandle.writePage(actRid.pageNum, buffer);
    }
    
    free(updRecord);
    free(buffer);
    
    return 0;
}


