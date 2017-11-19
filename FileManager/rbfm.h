#ifndef _rbfm_h_
#define _rbfm_h_


#include "pfm.h"
#include "../Utils/utils.h"

using namespace std;

// RBFM_ScanIterator is an iterator to go through records
// The way to use it is like the following:
//  RBFM_ScanIterator rbfmScanIterator;
//  rbfm.open(..., rbfmScanIterator);
//  while (rbfmScanIterator(rid, data) != RBFM_EOF) {
//    process the data;
//  }
//  rbfmScanIterator.close();

class RBFM_ScanIterator {
public:
    // declare public variables global to this class
    FileHandle fileHandle;
    vector<Attribute> recordDescriptor;
    string conditionAttribute;
    CompOp compOp;
    const void * value;
    vector<string> attributeNames;
    
    unordered_map<string, int> attrMap;
    RID rid;
    PageNum curtPageNum;
    SlotNum curtSlotNum;
    
    RBFM_ScanIterator() {};
    ~RBFM_ScanIterator() {};
    
    // Never keep the results in the memory. When getNextRecord() is called,
    // a satisfying record needs to be fetched from the file.
    // "data" follows the same format as RecordBasedFileManager::insertRecord().
    RC getNextRecord(RID &rid, void *data);
    RC close() {
        // TODO : fclose(fileHandle.pFile);
        return -1;
    };
    
    RC initialize(FileHandle &fileHandle,
                  const vector<Attribute> &recordDescriptor,
                  const string &conditionAttribute,
                  const CompOp compOp,
                  // comparision type such as "<" and "="
                  const void *value,
                  // used in the comparison
                  const vector<string> &attributeNames);
                  // a list of projected attributes
private:
    RC loadNxtRecOnPage(RID &rid, void * data, const vector<Attribute> & recordDescriptor);
    RC loadNxtRecOnSlot(RID &rid, void * data, const void * buffer, const vector<Attribute> & recordDescriptor);
    
};


class RecordBasedFileManager
{
public:
    static RecordBasedFileManager* instance();
    
    RC createFile(const string &fileName);
    
    RC destroyFile(const string &fileName);
    
    RC openFile(const string &fileName, FileHandle &fileHandle);
    
    RC closeFile(FileHandle &fileHandle);
    
    short getTotalUsedSlotsNum(const void * buffer);
    
    //  Format of the data passed into the function is the following:
    //  [n byte-null-indicators for y fields] [actual value for the first field] [actual value for the second field] ...
    //  1) For y fields, there is n-byte-null-indicators in the beginning of each record.
    //     The value n can be calculated as: ceil(y / 8). (e.g., 5 fields => ceil(5 / 8) = 1. 12 fields => ceil(12 / 8) = 2.)
    //     Each bit represents whether each field value is null or not.
    //     If k-th bit from the left is set to 1, k-th field value is null. We do not include anything in the actual data part.
    //     If k-th bit from the left is set to 0, k-th field contains non-null values.
    //     If there are more than 8 fields, then you need to find the corresponding byte first,
    //     then find a corresponding bit inside that byte.
    //  2) Actual data is a concatenation of values of the attributes.
    //  3) For Int and Real: use 4 bytes to store the value;
    //     For Varchar: use 4 bytes to store the length of characters, then store the actual characters.
    //  !!! The same format is used for updateRecord(), the returned data of readRecord(), and readAttribute().
    // For example, refer to the Q8 of Project 1 wiki page.
    RC insertRecord(FileHandle &fileHandle,
                    const vector<Attribute> &recordDescriptor,
                    const void *data,
                    RID &rid);
    
    RC readRecord(FileHandle &fileHandle,
                  const vector<Attribute> &recordDescriptor,
                  const RID &rid,
                  void *data);
    
    // This method will be mainly used for debugging/testing.
    // The format is as follows:
    // field1-name: field1-value  field2-name: field2-value ... \n
    // (e.g., age: 24  height: 6.1  salary: 9000
    //        age: NULL  height: 7.5  salary: 7500)
    RC printRecord(const vector<Attribute> &recordDescriptor, const void *data);
    
    /******************************************************************************************************************************************************************
     IMPORTANT, PLEASE READ: All methods below this comment (other than the constructor and destructor) are NOT required to be implemented for the part 1 of the project
     ******************************************************************************************************************************************************************/
    RC deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid);
    
    // Assume the RID does not change after an update
    RC updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid);
    
    RC readAttribute(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, const string &attributeName, void *data);
    
    // Scan returns an iterator to allow the caller to go through the results one by one.
    RC scan(FileHandle &fileHandle,
            const vector<Attribute> &recordDescriptor,
            const string &conditionAttribute,
            const CompOp compOp,                  // comparision type such as "<" and "="
            const void *value,                    // used in the comparison
            const vector<string> &attributeNames, // a list of projected attributes
            RBFM_ScanIterator &rbfm_ScanIterator);
    
    void * decodeMetaFrom(const void* data,
                          const vector<Attribute> & recordDescriptor,
                          short & recordLen);
    
//    RC encodeMetaInto(void * data,
//                      const void * record,
//                      const vector<Attribute> & recordDescriptor);
protected:
    RecordBasedFileManager();
    
    ~RecordBasedFileManager();
    
private:
    static RecordBasedFileManager *_rbf_manager;
    
    PagedFileManager *_pbf_manager;
    UtilsManager * _utils;

};

#endif /* rbfm_hpp */
