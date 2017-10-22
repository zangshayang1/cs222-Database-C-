#include "rbfm.h"

#include "cmath"
#include <iostream>

const short ATTR_NULL_FLAG = -1;
const short SLOT_OFFSET_CLEAN = -2;
const short SLOT_RECLEN_CLEAN = -3;
const PageNum PAGENUM_UNAVAILABLE = -4;
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
// ---------------------------------------------------------------------------------------

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
            auto mask = (char) (0x80 >> j); // 1000 0000 >> j
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
    recordLen = metaLength + dataLength;
    void * record = malloc((size_t) recordLen);
    // void * record -> [meta1, meta2,..., data1, data2,...]
    memcpy(record, meta, (size_t) metaLength);
    // dataLength = n_bytes + actualData_length
    memcpy((char*)record + metaLength, (char*)data + n_bytes, (size_t) (dataLength - n_bytes));
    
    free(meta);
    return (char*)record;
}

/*
 This func checks the absolute amount of freespace on the page pointed by fileHandle + pageNum
 @input:
 fileHandle
 pageNum
 freeSpace -> itself is a pointer variable pointing to the address passed in, in our case, [11]
 */
short RecordBasedFileManager::checkForSpace(FileHandle & fileHandle,
                                            const PageNum & pageNum) {
    void* buffer = malloc(PAGE_SIZE);
    fileHandle.readPage(pageNum, buffer);
    short freeSpace;
    short totalSlots;
    memcpy(&freeSpace, (char*)buffer + FREE_SPACE_INFO_POS, sizeof(short));
    memcpy(&totalSlots, (char*)buffer + SLOT_NUM_INFO_POS, sizeof(short));
    
    short spaceLeftEmpty = SLOT_NUM_INFO_POS - sizeof(int) * totalSlots - freeSpace;
    
    free(buffer);
    
    return spaceLeftEmpty;
}

PageNum RecordBasedFileManager::findNextAvaiPage(FileHandle & fileHandle,
                                                 const short & recordLen) {
    PageNum totalPageNum = fileHandle.getNumberOfPages();
    
    // if no page was ever allocated
    if ( totalPageNum == 0) {
        return PAGENUM_UNAVAILABLE;
    }
    // page indexing starts by 0!!!!!
    PageNum i = 0;
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


RC RecordBasedFileManager::insertIntoNewPage(FileHandle & fileHandle,
                                             const void * record,
                                             RID &rid,
                                             const short & recordLen) {
    void * buffer = malloc(PAGE_SIZE);
    // without the following memset(), the entire chunk of memory will remain uninitialized
    // ERROR: Syscall param write(buf) points to uninitialised byte(s)
    memset(buffer, EMPTY_BYTE, PAGE_SIZE);
    
    // we are inserting the first record of this new page
    short totalSlots = 1;
    memcpy((char*)buffer + SLOT_NUM_INFO_POS, & totalSlots, sizeof(short));
    
    // next_slot_position is determined by SLOT_NUM_INFO_POS and totalSlots
    // init record_offset so that (char*)buffer + record_offset can be the address of the record
    // SLOT[record_offset, record_length] -> each takes 2 bytes
    short next_slot_pos = SLOT_NUM_INFO_POS - sizeof(int) * totalSlots; // 4092 - 4 * 1 = 4088
    short record_offset = 0;
    memcpy((char*)buffer + next_slot_pos, & record_offset, sizeof(short));
    memcpy((char*)buffer + next_slot_pos + sizeof(short), & recordLen, sizeof(short));
    
    // fill in record data
    memcpy((char*)buffer + record_offset, record, (size_t) recordLen);
    
    // fill in remaining freeSpace
    short freeSpace = recordLen;
    memcpy((char*)buffer + FREE_SPACE_INFO_POS, & freeSpace, sizeof(short));
    
    fileHandle.appendPage(buffer);
    rid.pageNum = fileHandle.getNumberOfPages() - 1; // indexing starts by 0
    rid.slotNum = (unsigned int) totalSlots; // indexing starts by 1
    
    free(buffer);
    return 0;
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

short getRecOffset(const void * data, const short & slotIdx) {
    short offset;
    memcpy(&offset, (char*)data + SLOT_NUM_INFO_POS - slotIdx * sizeof(int), sizeof(short));
    return offset;
}

short getRecLength(const void * data, const short & slotIdx) {
    short recLen;
    memcpy(&recLen, (char*)data + SLOT_NUM_INFO_POS - slotIdx * sizeof(int) + sizeof(short), sizeof(short));
    return recLen;
}

short nextAvaiSlot(const void * data, const short & totSlots) {
    short i = 1;
    while (i <= totSlots) {
        short recOffset = getRecOffset(data, i);
        short recLength = getRecLength(data, i);
        if (recOffset == SLOT_OFFSET_CLEAN && recLength == SLOT_RECLEN_CLEAN) {
            break;
        }
        i++;
    }
    return i;
}

RC putRecOffset(const void * data, const short & slotIdx, const short & offset) {
    memcpy((char*)data + SLOT_NUM_INFO_POS - slotIdx * sizeof(int), & offset, sizeof(short));
    return 0;
}

RC putRecLength(const void * data, const short & slotIdx, const short & length) {
    memcpy((char*)data + SLOT_NUM_INFO_POS - slotIdx * sizeof(int) + sizeof(short), & length, sizeof(short));
    return 0;
}

void* getRecordFrom(const void * data, const short & offset, const short & recLength) {
    void * record = malloc((size_t) recLength);
    memcpy(record, (char*)data + offset, (size_t) recLength);
    return record;
}

RC RecordBasedFileManager::insertIntoPage(FileHandle & fileHandle,
                                          const PageNum & next_avai_page,
                                          const void * record,
                                          RID & rid,
                                          const short & recordLen) {
    void * buffer = malloc(PAGE_SIZE);
    fileHandle.readPage(next_avai_page, buffer);
    
    short totalSlots = getTotalSlotsNum(buffer);
    short freeSpace = getFreeOffset(buffer);
    // fill in record
    short recOffset = freeSpace;
    memcpy((char*)buffer + recOffset, record, (size_t) recordLen);
    // adjust freeSpace
    freeSpace += recordLen;
    
    short next_slot_pos = nextAvaiSlot(buffer, totalSlots);
    
    totalSlots += (short) 1;
    
    // put all the above back
    putFreeOffset(buffer, freeSpace);
    putTotalSlotsNum(buffer, totalSlots);
    putRecOffset(buffer, next_slot_pos, recOffset);
    putRecLength(buffer, next_slot_pos, recordLen);
    
    fileHandle.writePage(next_avai_page, buffer);
    
    rid.pageNum = next_avai_page;
    rid.slotNum = next_slot_pos;
    // the idx of the current slot = sum(slots) as long as they are incrementing only
    
    free(buffer);
    return 0;
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle,
                                        const vector<Attribute> &recordDescriptor,
                                        const void *data,
                                        RID &rid) {
    if (fileHandle.pFile == nullptr || recordDescriptor.empty() || data == nullptr) {
        return -1;
    }
    
    // must first find recordLen and then find if a suitable page exists.
    short recordLen;
    // you cannot pass in a uninitialized pointer
    void* record = decodeMetaFrom(data, recordDescriptor, recordLen);
    
    PageNum next_avai_page = findNextAvaiPage(fileHandle, recordLen);
    if (next_avai_page == PAGENUM_UNAVAILABLE) {
        // if no page was ever allocated, meaning this is the first ever record insertion,
        // OR if no page was found to have enough capacity to hold this record
        // execute the following
        insertIntoNewPage(fileHandle, record, rid, recordLen);
    }
    else {
        insertIntoPage(fileHandle, next_avai_page, record, rid, recordLen);
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
            auto mask = (char) (0x80 >> j); // 1000 0000
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

bool recordExists(void * buffer, const RID & rid) {
    short recOffset = getRecOffset(buffer, (short)rid.slotNum);
    short recLength = getRecLength(buffer, (short)rid.slotNum);
    if (recOffset == SLOT_OFFSET_CLEAN && recLength == SLOT_RECLEN_CLEAN) {
        return false;
    }
    //    else if (recOffset == SLOT_OFFSET_CLEAN || recLength == SLOT_RECLEN_CLEAN) {
    //        throw("Slots info got messed up.");
    //    }
    else {
        return true;
    }
}
RC RecordBasedFileManager::readRecord(FileHandle &fileHandle,
                                      const vector<Attribute> &recordDescriptor,
                                      const RID &rid,
                                      void *data) {
    // check fileHandler
    if (fileHandle.pFile == nullptr || recordDescriptor.empty()) {
        return -1;
    }
    // check rid.pageNum
    unsigned total_page_num = fileHandle.getNumberOfPages();
    if (rid.pageNum > total_page_num) {
        return -1;
    }
    // check rid.slotNum
    void * buffer = malloc(PAGE_SIZE);
    fileHandle.readPage(rid.pageNum, buffer);
    
    short totalSlots = getTotalSlotsNum(buffer);
    if (rid.slotNum < 1 or rid.slotNum > totalSlots) {
        return -1;
    }
    
    if (!recordExists(buffer, rid)) {
        return -1;
    }
    
    // find record_offset and recordLen
    short record_offset = getRecOffset(buffer, (short) rid.slotNum);
    short recordLen = getRecLength(buffer, (short) rid.slotNum);
    // check if the record still exists
    
    // read data into record (another block of memory)
    void * record = getRecordFrom(buffer, record_offset, recordLen);
    
    encodeMetaInto(data, record, (short) recordDescriptor.size(), recordLen);
    
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
            auto mask = (char) (0x80 >> j); // 1000 0000 -> 0100 0000
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

short getSlotsLeftBound(const void * buffer,
                        const short & totSlots) {
    short slotIdx = 1;
    short counter = 0;
    while (counter < totSlots) {
        short recOffset = getRecOffset(buffer, slotIdx);
        short recLength = getRecLength(buffer, slotIdx);
        if ( recOffset == SLOT_OFFSET_CLEAN &&  recLength == SLOT_RECLEN_CLEAN) {
            // nullified slot, doesn't count towards totSlots
            slotIdx += 1; // move slotIdx left by 1
        }
        //        else if (recOffset == SLOT_OFFSET_CLEAN || recLength == SLOT_RECLEN_CLEAN) {
        //            // ERROR
        ////            cout << "UNEXPECTED!" << endl;
        //            throw("UNEXPECTED");
        //        }
        else {
            slotIdx += 1;
            counter += 1;
        }
    }
    return SLOT_NUM_INFO_POS - (slotIdx - 1) * sizeof(int);
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
    short i = 1;
    while (SLOT_NUM_INFO_POS - i * sizeof(int) >= slotLeftBound) {
        short curtRecOffset = getRecOffset(buffer, i);
        if (nxtRecOffset == curtRecOffset) {
            nxtRecLength = getRecLength(buffer, i);
            break;
        }
        i++;
    }
    // eligibility check : actually found nxtRecLength
    //    if (nxtRecLength == SLOT_RECLEN_CLEAN) {
    ////        cout << "Didn't find the slot storing record offset: " << nxtRecOffset << endl;
    //        throw("Didn't find the slot storing nxtRecOffset.");
    //    }
    // ready to move nxtRec
    memcpy((char*)buffer + breakPoint, (char*)buffer + nxtRecOffset, (size_t) nxtRecLength);
    short newBreakPoint = breakPoint + nxtRecLength;
    memset((char*)buffer + newBreakPoint, EMPTY_BYTE, (size_t)breakLength);
    // starting from newBreakPoint, the remaining length equals to the old breakLength
    // after all, breakLength is not gonna change throughout the process
    kickinRemainingRecords(buffer, newBreakPoint, breakLength, slotLeftBound);
    return 0;
}

RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle,
                                        const vector<Attribute> & recordDescriptor,
                                        const RID &rid) {
    if (fileHandle.pFile == nullptr || recordDescriptor.empty()) {
        return -1;
    }
    
    if (rid.pageNum >= fileHandle.getNumberOfPages() || rid.slotNum < 1 || rid.slotNum > PAGE_SIZE) {
        return -1;
    }
    // read page in
    void * buffer = malloc(PAGE_SIZE);
    fileHandle.readPage(rid.pageNum, buffer);
    
    // erase record in memory
    short recLength = getRecLength(buffer, (short)rid.slotNum);
    short recOffset = getRecOffset(buffer, (short)rid.slotNum);
    memset((char*)buffer + recOffset, EMPTY_BYTE, (size_t)recLength);
    
    // reset slot info, freeOffset, totalslotNum, etc
    putRecOffset(buffer, (short)rid.slotNum, SLOT_OFFSET_CLEAN);
    putRecLength(buffer, (short)rid.slotNum, SLOT_RECLEN_CLEAN);
    putTotalSlotsNum(buffer, getTotalSlotsNum(buffer) - (short)1);
    putFreeOffset(buffer, getFreeOffset(buffer) - recLength);
    
    // fill up the hole
    short slotLeftBound = getSlotsLeftBound(buffer, getTotalSlotsNum(buffer));
    kickinRemainingRecords(buffer, recOffset, recLength, slotLeftBound);
    
    fileHandle.writePage(rid.pageNum, buffer);
    
    free(buffer);
    return 0;
}

//RC updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid) {
//
//}
