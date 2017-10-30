#include <iostream>
#include "rm.h"
#include "../FileManager/rbfm.h"
#include "../FileManager/pfm.h"

const string INIT_TABLE_NAME = "TABLE";
const string INIT_COLUMN_NAME = "COLUMN";
const string DAT_FILE_SUFFIX = ".dat";
const int SYSTEM = -1;
const int USER = 1;



/* ---------------------------------------------------------------------------------------
 General
 
 Utils
 
 Defined
 
 Below
 --------------------------------------------------------------------------------------- */
Table constructTable(const int & tid,
                     const string & tableName,
                     const string & fileName,
                     const int & tableMode)
{
    Table tbl;
    tbl.tid = tid;
    tbl.tableName = tableName;
    tbl.fileName = fileName;
    tbl.tableMode = tableMode;
    return tbl;
}

Column constructColumn(const int & tid,
                       const string & columnName,
                       const AttrType & columnType,
                       const AttrLength & columnLength,
                       const int & columnPosition,
                       const int & columnMode)
{
    Column clm;
    clm.tid = tid;
    clm.columnName = columnName;
    clm.columnType = columnType;
    clm.columnLength = columnLength;
    clm.columnPosition = columnPosition;
    clm.columnMode = columnMode;
    return clm;
}

Attribute constructAttribute(const string & name,
                             const AttrType & type,
                             const AttrLength & length)
// typedef enum { TypeInt = 0, TypeReal, TypeVarChar } AttrType;
{
    Attribute attr;
    attr.name = name;
    attr.type = type;
    attr.length = length;
    
    return attr;
}

int checkOwnership(Table tbl)
{
    return tbl.tableMode;
}

vector<Attribute> prepareTableDescriptor()
{
    vector<Attribute> tableDescriptor;
    tableDescriptor.push_back(constructAttribute("table-id", TypeInt, 4));
    tableDescriptor.push_back(constructAttribute("table-name", TypeVarChar, 50));
    tableDescriptor.push_back(constructAttribute("file-name", TypeVarChar, 50));
    tableDescriptor.push_back(constructAttribute("table-mode", TypeInt, 4));
    return tableDescriptor;
}

vector<Attribute> prepareColumnDescriptor()
{
    vector<Attribute> columnDescriptor;
    columnDescriptor.push_back(constructAttribute("table-id", TypeInt, 4));
    columnDescriptor.push_back(constructAttribute("column-name", TypeVarChar, 50));
    columnDescriptor.push_back(constructAttribute("column-type", TypeInt, 4));
    columnDescriptor.push_back(constructAttribute("column-length", TypeInt, 4));
    columnDescriptor.push_back(constructAttribute("column-position", TypeInt, 4));
    columnDescriptor.push_back(constructAttribute("column-mode", TypeInt, 4));
    return columnDescriptor;
}

RC prepareRecForTable(const int & tid,
                      const string & tname,
                      const string & fname,
                      const int  & tmode,
                      void * buffer) // init 0
{
    const auto tnameLen = (short) tname.length();
    const auto fnameLen = (short) fname.length();
    
    short recLen = 0;
    
    const unsigned char nullindicator = 0; // not a thing should be NULL
    memcpy((char*)buffer + recLen, & nullindicator, 1);
    recLen += 1;
    
    memcpy((char*)buffer + recLen, & tid, 4);
    recLen += 4;
    
    memcpy((char*)buffer + recLen, & tnameLen, 4);
    recLen += 4;
    
    memcpy((char*)buffer + recLen, & tname, tnameLen);
    recLen += tnameLen;
    
    memcpy((char*)buffer + recLen, & fnameLen, 4);
    recLen += 4;
    
    memcpy((char*)buffer + recLen, & fname, fnameLen);
    recLen += fnameLen;
    
    memcpy((char*)buffer + recLen, & tmode, 4);
    recLen += 4;
    
    return 0;
}


RC prepareRecForColumn(const int & tid,
                       const string & cname,
                       const int & ctype,
                       const int & clength,
                       const int & cposition,
                       const int & cmode,
                       void * buffer)
{
    const auto cnameLen = (short)cname.length();
    
    short recLen = 0;
    
    const unsigned char nullindicator = 0; // not a thing should be NULL
    memcpy((char*)buffer + recLen, & nullindicator, 1);
    recLen += 1;
    
    memcpy((char*)buffer + recLen, & tid, 4);
    recLen += 4;
    
    memcpy((char*)buffer + recLen, & cnameLen, 4);
    recLen += 4;
    
    memcpy((char*)buffer + recLen, & cname, cnameLen);
    recLen += cnameLen;
    
    memcpy((char*)buffer + recLen, & ctype, 4);
    recLen += 4;
    
    memcpy((char*)buffer + recLen, & clength, 4);
    recLen += 4;
    
    memcpy((char*)buffer + recLen, & clength, 4);
    recLen += 4;
    
    memcpy((char*)buffer + recLen, & cmode, 4);
    recLen += 4;

    return 0;
}

/* ---------------------------------------------------------------------------------------
 General
 
 Utils
 
 Defined
 
 Above
 --------------------------------------------------------------------------------------- */

RC RelationManager::_loadTABLE(FileHandle & tableHandle)
{
    void * pageBuffer = malloc(PAGE_SIZE);
    void * data = malloc(PAGE_SIZE);
    
    Table tbl;
    RID tRid;
    vector<Attribute> tableDescriptor = prepareTableDescriptor();
    
    PageNum totPages = tableHandle.getNumberOfPages();
    for(PageNum page = 0; page < totPages; page++) {
        if (tableHandle.readPage(page, pageBuffer) == -1) {
            break;
        }
        
        short totSlots = _rbf_manager->getTotalUsedSlotsNum(pageBuffer);
        for (SlotNum slot = 0; slot < totSlots; slot++) {
            tRid.pageNum = page; // install pageNum and slotNum to tRid
            tRid.slotNum = slot;
            if (_rbf_manager->readRecord(tableHandle, tableDescriptor, tRid, data) == -1) {
                break;
            }
            for (int attrIdx = 0; attrIdx < 4; attrIdx++) {
                short attrLen = tableDescriptor[attrIdx].length;
                void * attr = malloc(attrLen);
                _rbf_manager->readAttribute(tableHandle, tableDescriptor, tRid, tableDescriptor[attrIdx].name, attr);
                switch (attrIdx) {
                    case 0:
                        memcpy(& tbl.tid, attr, attrLen);
                        break;
                    case 1:
                        memcpy(& tbl.tableName, attr, attrLen);
                        break;
                    case 2:
                        memcpy(& tbl.fileName, attr, attrLen);
                        break;
                    case 3:
                        memcpy(& tbl.tableMode, attr, attrLen);
                        break;
                    default:
                        break;
                }
                free(attr);
            }
            TABLEMAP[tbl.tableName] = tbl;
        }
    }
    free(pageBuffer);
    free(data);
    return 0;
}

RC RelationManager::_loadCOLUMN(FileHandle & columnHandle)
{
    void * pageBuffer = malloc(PAGE_SIZE);
    void * data = malloc(PAGE_SIZE);
    
    Column clm;
    RID cRid;
    vector<Attribute> columnDescriptor = prepareColumnDescriptor();
    
    PageNum totPages = columnHandle.getNumberOfPages();
    for(PageNum page = 0; page < totPages; page++) {
        if (columnHandle.readPage(page, pageBuffer) == -1) {
            break;
        }
        
        short totSlots = _rbf_manager->getTotalUsedSlotsNum(pageBuffer);
        for (SlotNum slot = 0; slot < totSlots; slot++) {
            cRid.pageNum = page; // install pageNum and slotNum to tRid
            cRid.slotNum = slot;
            if (_rbf_manager->readRecord(columnHandle, columnDescriptor, cRid, data) == -1) {
                break;
            }
            
            for (int attrIdx = 0; attrIdx < 6; attrIdx++) {
                short attrLen = columnDescriptor[attrIdx].length;
                void * attr = malloc(attrLen);
                _rbf_manager->readAttribute(columnHandle, columnDescriptor, cRid, columnDescriptor[attrIdx].name, attr);
                switch (attrIdx) {
                    case 0:
                        memcpy(& clm.tid, attr, attrLen);
                        break;
                    case 1:
                        memcpy(& clm.columnName, attr, attrLen);
                        break;
                    case 2:
                        memcpy(& clm.columnType, attr, attrLen);
                        break;
                    case 3:
                        memcpy(& clm.columnLength, attr, attrLen);
                        break;
                    case 4:
                        memcpy(& clm.columnPosition, attr, attrLen);
                        break;
                    case 5:
                        memcpy(& clm.columnMode, attr, attrLen);
                        break;
                    default:
                        break;
                }
                free(attr);
            }
            COLUMNSMAP[clm.tid][clm.columnName] = clm;
        }
    }
    free(pageBuffer);
    free(data);
    
    return 0;
}

RelationManager* RelationManager::_rm = nullptr;

RelationManager* RelationManager::instance()
{
    static RelationManager * _rm = new RelationManager();
    return _rm;
}

RelationManager::RelationManager()
{
    _rbf_manager = RecordBasedFileManager::instance();
    
    if (_rbf_manager->fileExists(INIT_TABLE_NAME + DAT_FILE_SUFFIX) && _rbf_manager->fileExists(INIT_COLUMN_NAME + DAT_FILE_SUFFIX)) {
        
        _rbf_manager->openFile(INIT_TABLE_NAME + DAT_FILE_SUFFIX, tableHandle);
        TABLEMAP.clear(); // global
        _loadTABLE(tableHandle);
        _rbf_manager->closeFile(tableHandle);
    
        _rbf_manager->openFile(INIT_COLUMN_NAME + DAT_FILE_SUFFIX, columnHandle);
        COLUMNSMAP.clear(); // global
        _loadCOLUMN(columnHandle);
        _rbf_manager->closeFile(columnHandle);
    }
    // else : simply initialization
}

RelationManager::~RelationManager()
{
    delete &_rm;
    TABLEMAP.clear();
    COLUMNSMAP.clear();
}


RC RelationManager::createCatalog()
{
    if (_rbf_manager->fileExists(INIT_TABLE_NAME + DAT_FILE_SUFFIX) || _rbf_manager->fileExists(INIT_COLUMN_NAME + DAT_FILE_SUFFIX)) {
        cout << "Catalog TABLE.dat and COLUMN.dat already exists." << endl;
        return -1;
    }
    
    _rbf_manager->createFile(INIT_TABLE_NAME + DAT_FILE_SUFFIX);
    
    void * buffer = malloc(PAGE_SIZE);
    
    _rbf_manager->openFile(INIT_TABLE_NAME + DAT_FILE_SUFFIX, tableHandle);
    // init a rec for TABLE in TABLE
    vector<Attribute> tableDescriptor = prepareTableDescriptor();
    
    prepareRecForTable(1,
                       INIT_TABLE_NAME,
                       INIT_TABLE_NAME + DAT_FILE_SUFFIX,
                       SYSTEM,
                       buffer);
    RID ttRid; // tt: TABLE in TABLE
    _rbf_manager->insertRecord(tableHandle, tableDescriptor, buffer, ttRid);
    
    // init a rec for COLUMN in TABLE
    prepareRecForTable(2,
                       INIT_COLUMN_NAME,
                       INIT_COLUMN_NAME + DAT_FILE_SUFFIX,
                       SYSTEM,
                       buffer);
    RID ctRid; // ct: COLUMN in TABLE
    _rbf_manager->insertRecord(tableHandle, tableDescriptor, buffer, ctRid);
    
    _rbf_manager->closeFile(tableHandle); // done with TABLE
    
    
    _rbf_manager->createFile(INIT_COLUMN_NAME + DAT_FILE_SUFFIX);
    
    _rbf_manager->openFile(INIT_COLUMN_NAME + DAT_FILE_SUFFIX, columnHandle);
    
    vector<Attribute> columnDescriptor = prepareColumnDescriptor();
    
    // insert records for TABLE in COLUMN, whose number == # of TABLE attributes
    for(int i = 0; i < tableDescriptor.size(); i++) {
        prepareRecForColumn(1,
                            tableDescriptor[i].name,
                            tableDescriptor[i].type,
                            tableDescriptor[i].length,
                            i+1,
                            SYSTEM,
                            buffer);
        RID tcRid; // tc: TABLE in COLUMN
        _rbf_manager->insertRecord(columnHandle, columnDescriptor, buffer, tcRid);
    }
    
    // insert records for COLUMN in COLUMN, whose number == # of COLUMN attrs
    for(int i = 0; i < columnDescriptor.size(); i++) {
        prepareRecForColumn(2,
                            columnDescriptor[i].name,
                            columnDescriptor[i].type,
                            columnDescriptor[i].length,
                            i+1,
                            SYSTEM,
                            buffer);
        RID ccRid;
        _rbf_manager->insertRecord(columnHandle, columnDescriptor, buffer, ccRid);
    }
    free(buffer);
    _rbf_manager->closeFile(columnHandle);
    
    return 0;
}

RC RelationManager::deleteCatalog()
{
    if (!_rbf_manager->fileExists(INIT_TABLE_NAME + DAT_FILE_SUFFIX) || !_rbf_manager->fileExists(INIT_COLUMN_NAME + DAT_FILE_SUFFIX)) {
        cout << "Catalog TABLE.dat and COLUMN.dat don't exist." << endl;
        return -1;
    }
    _rbf_manager->destroyFile(INIT_TABLE_NAME + DAT_FILE_SUFFIX);
    _rbf_manager->destroyFile(INIT_COLUMN_NAME + DAT_FILE_SUFFIX);
    
    TABLEMAP.clear();
    COLUMNSMAP.clear();
    
    return 0;
    
}

short RelationManager::getTotalTableNum()
{
    return TABLEMAP.size();
}

RC RelationManager::createTable(const string &tableName,
                                const vector<Attribute> &attrs)
{
    // check existence of the corresponding file 'cause there shouldn't be.
    if (_rbf_manager->fileExists(tableName + DAT_FILE_SUFFIX)) {
        cout << "The table already exists." << endl;
        return -1;
    }
    _rbf_manager->createFile(tableName + DAT_FILE_SUFFIX);
    
    // check existence of the primitive two files: TABLE and COLUMN
    if (!_rbf_manager->fileExists(INIT_TABLE_NAME + DAT_FILE_SUFFIX) || !_rbf_manager->fileExists(INIT_COLUMN_NAME + DAT_FILE_SUFFIX)) {
        cout << "Catalog TABLE.dat and COLUMN.dat don't exist." << endl;
        return -1;
    }

    void * buffer = malloc(PAGE_SIZE);
    
    // insert a record in TABLE
    _rbf_manager->openFile(INIT_TABLE_NAME + DAT_FILE_SUFFIX, tableHandle);
    
    vector<Attribute> tableDescriptor = prepareTableDescriptor();
    int tid = getTotalTableNum() + 1; // TABLEMAP.size()
    prepareRecForTable(tid, tableName, tableName + DAT_FILE_SUFFIX, USER, buffer);
    RID tRid;
    
    _rbf_manager->insertRecord(tableHandle, tableDescriptor, buffer, tRid);
    
    TABLEMAP[tableName] = constructTable(tid, tableName, tableName + DAT_FILE_SUFFIX, USER);
    
    _rbf_manager->closeFile(tableHandle);
    
    // insert a bunch of records in COLUMN
    _rbf_manager->openFile(INIT_COLUMN_NAME + DAT_FILE_SUFFIX, columnHandle);

    vector<Attribute> columnDescriptor = prepareColumnDescriptor();
    
    for(int i = 0; i < attrs.size(); i++) {
        prepareRecForColumn(tid, attrs[i].name, attrs[i].type, attrs[i].length, i+1, USER, buffer);
        RID cRid;
        _rbf_manager->insertRecord(columnHandle, columnDescriptor, buffer, cRid);
        COLUMNSMAP[tid][attrs[i].name] = constructColumn(tid, attrs[i].name, attrs[i].type, attrs[i].length, i+1, USER);
    }
    
    _rbf_manager->closeFile(columnHandle);
    
    return 0;
}

RC RelationManager::deleteTable(const string &tableName)
{
    if (!_rbf_manager->fileExists(tableName + DAT_FILE_SUFFIX)) {
        return -1;
    }
    _rbf_manager->destroyFile(tableName + DAT_FILE_SUFFIX);
    return 0;
}

RC RelationManager::getAttributes(const string &tableName, vector<Attribute> &attrs)
{
    if (!_rbf_manager->fileExists(tableName + DAT_FILE_SUFFIX)) {
        return -1;
    }
    // get attributes about a table from COLUMNSMAP
    int tid = TABLEMAP[tableName].tid;
    
    for (auto it = COLUMNSMAP[tid].begin(); it != COLUMNSMAP[tid].end(); ++it) {
        Column clm = it->second;
        attrs.push_back(constructAttribute(clm.columnName, clm.columnType, clm.columnLength));
    }
    return 0;
}

RC RelationManager::insertTuple(const string &tableName, const void *data, RID &rid)
{
    if (!_rbf_manager->fileExists(tableName + DAT_FILE_SUFFIX)) {
        return -1;
    }
    if (checkOwnership(TABLEMAP[tableName]) == SYSTEM) {
        return -1;
    }
    
    FileHandle fileHandle;
    _rbf_manager->openFile(tableName + DAT_FILE_SUFFIX, fileHandle);
    
    vector<Attribute> tupleDescriptor;
    getAttributes(tableName, tupleDescriptor);
    
    _rbf_manager->insertRecord(fileHandle, tupleDescriptor, data, rid);
    
    _rbf_manager->closeFile(fileHandle);
    
    return 0;
}

RC RelationManager::deleteTuple(const string &tableName, const RID &rid)
{
    if (!_rbf_manager->fileExists(tableName + DAT_FILE_SUFFIX)) {
        return -1;
    }
    if (checkOwnership(TABLEMAP[tableName]) == SYSTEM) {
        return -1;
    }
    
    FileHandle fileHandle;
    _rbf_manager->openFile(tableName + DAT_FILE_SUFFIX, fileHandle);
    
    vector<Attribute> tupleDescriptor;
    getAttributes(tableName, tupleDescriptor);
    
    _rbf_manager->deleteRecord(fileHandle, tupleDescriptor, rid);
    
    _rbf_manager->closeFile(fileHandle);
    
    return 0;
}

RC RelationManager::updateTuple(const string &tableName, const void *data, const RID &rid)
{
    if (!_rbf_manager->fileExists(tableName + DAT_FILE_SUFFIX)) {
        return -1;
    }
    if (checkOwnership(TABLEMAP[tableName]) == SYSTEM) {
        return -1;
    }
    
    FileHandle fileHandle;
    _rbf_manager->openFile(tableName + DAT_FILE_SUFFIX, fileHandle);
    
    vector<Attribute> tupleDescriptor;
    getAttributes(tableName, tupleDescriptor);
    
    _rbf_manager->updateRecord(fileHandle, tupleDescriptor, data, rid);
    
    _rbf_manager->closeFile(fileHandle);
    
    return 0;
}

RC RelationManager::readTuple(const string &tableName, const RID &rid, void *data)
{
    if (!_rbf_manager->fileExists(tableName + DAT_FILE_SUFFIX)) {
        return -1;
    }
    // no ownership check required
    
    FileHandle fileHandle;
    _rbf_manager->openFile(tableName + DAT_FILE_SUFFIX, fileHandle);
    
    vector<Attribute> tupleDescriptor;
    getAttributes(tableName, tupleDescriptor);
    
    _rbf_manager->readRecord(fileHandle, tupleDescriptor, rid, data);
    
    _rbf_manager->closeFile(fileHandle);
    
    return 0;
}

RC RelationManager::printTuple(const vector<Attribute> &attrs, const void *data)
{
    _rbf_manager->printRecord(attrs, data);
    return 0;
}

RC RelationManager::readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data)
{
    if (!_rbf_manager->fileExists(tableName + DAT_FILE_SUFFIX)) {
        return -1;
    }
    // no ownership check required
    
    FileHandle fileHandle;
    _rbf_manager->openFile(tableName + DAT_FILE_SUFFIX, fileHandle);
    
    vector<Attribute> tupleDescriptor;
    getAttributes(tableName, tupleDescriptor);
    
    _rbf_manager->readAttribute(fileHandle, tupleDescriptor, rid, attributeName, data);
    
    _rbf_manager->closeFile(fileHandle);
    
    return 0;
}

RC RelationManager::scan(const string &tableName,
                         const string &conditionAttribute,
                         const CompOp compOp,
                         const void *value,
                         const vector<string> &attributeNames,
                         RM_ScanIterator &rm_ScanIterator)
{
    if (!_rbf_manager->fileExists(tableName + DAT_FILE_SUFFIX)) {
        return -1;
    }
    // no ownership check required
    
    FileHandle fileHandle;
    _rbf_manager->openFile(tableName + DAT_FILE_SUFFIX, fileHandle);
    
    vector<Attribute> tupleDescriptor;
    getAttributes(tableName, tupleDescriptor);
    
    RBFM_ScanIterator rbfmsi = RBFM_ScanIterator();
    
    _rbf_manager->scan(fileHandle, tupleDescriptor, conditionAttribute, compOp, value, attributeNames, rbfmsi);
    // -> which finishes the following initialization : rbfmsi.initialize(fileHandle, tupleDescriptor, conditionAttribute, compOp, value, attributeNames);
    
    RM_ScanIterator rm_scanIterator = RM_ScanIterator(rbfmsi);
    
    _rbf_manager->closeFile(fileHandle);
    
    return 0;
}

// Extra credit work
RC RelationManager::dropAttribute(const string &tableName, const string &attributeName)
{
    return -1;
}

// Extra credit work
RC RelationManager::addAttribute(const string &tableName, const Attribute &attr)
{
    return -1;
}


