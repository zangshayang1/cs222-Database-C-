#ifndef _rm_h_
#define _rm_h_


#include "../FileManager/pfm.h"
#include "../FileManager/rbfm.h"

using namespace std;

#define RM_EOF (-1)  // end of a scan operator

typedef struct {
    int tid;
    string tableName;
    string fileName;
    int tableMode;
    RID tRid;
} Table;

typedef struct {
    int tid;
    string columnName;
    AttrType columnType;
    AttrLength columnLength;
    int columnPosition;
    int columnMode;
    RID cRid;
} Column;

// RM_ScanIterator is an iteratr to go through tuples
class RM_ScanIterator {
public:
  RM_ScanIterator() {};
  ~RM_ScanIterator() {};

    RC initialize(RBFM_ScanIterator rbfmsi) {
        this->_rbfmsi = rbfmsi;
        return 0;
    };

  // "data" follows the same format as RelationManager::insertTuple()
  RC getNextTuple(RID &rid, void *data) {
      if (_rbfmsi.getNextRecord(rid, data) == RBFM_EOF) {
          return RM_EOF;
      }
      return 0;
  };
  RC close() {
      _rbfmsi.close();
      return -1; };

private:
    RBFM_ScanIterator _rbfmsi;
    
};


// Relation Manager
class RelationManager
{
public:
  static RelationManager* instance();

  RC createCatalog();

  RC deleteCatalog();

  RC createTable(const string &tableName, const vector<Attribute> &attrs);

  RC deleteTable(const string &tableName);

  RC getAttributes(const string &tableName, vector<Attribute> &attrs);

  RC insertTuple(const string &tableName, const void *data, RID &rid);

  RC deleteTuple(const string &tableName, const RID &rid);

  RC updateTuple(const string &tableName, const void *data, const RID &rid);

  RC readTuple(const string &tableName, const RID &rid, void *data);

  // Print a tuple that is passed to this utility method.
  // The format is the same as printRecord().
  RC printTuple(const vector<Attribute> &attrs, const void *data);

  RC readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data);

  // Scan returns an iterator to allow the caller to go through the results one by one.
  // Do not store entire results in the scan iterator.
  RC scan(const string &tableName,
      const string &conditionAttribute,
      const CompOp compOp,                  // comparison type such as "<" and "="
      const void *value,                    // used in the comparison
      const vector<string> &attributeNames, // a list of projected attributes
      RM_ScanIterator &rm_ScanIterator);

// Extra credit work (10 points)
public:
    RC addAttribute(const string &tableName, const Attribute &attr);

    RC dropAttribute(const string &tableName, const string &attributeName);
    
    short getTotalTableNum();

protected:
  RelationManager();
  ~RelationManager();

private:
    FileHandle tableHandle;
    FileHandle columnHandle;
    RecordBasedFileManager *_rbf_manager;
    unordered_map<string, Table> TABLEMAP;
    unordered_map<int, vector<Column>> COLUMNSMAP;
    
    static RelationManager* _rm;
    UtilsManager * _utils;
    
    RC _loadTABLE(FileHandle & tableHandle);
    RC _loadCOLUMN(FileHandle & columnHandle);
};

#endif
