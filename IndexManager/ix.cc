#include "ix.h"

/*
 * WHY init this way?
 */
IndexManager* IndexManager::_index_manager = 0;

IndexManager* IndexManager::instance()
{
    if(!_index_manager)
        _index_manager = new IndexManager();

    return _index_manager;
}

IndexManager::IndexManager()
{
    _pfm = PagedFileManager::instance();
}

IndexManager::~IndexManager()
{
    delete _index_manager;
}

RC IndexManager::createFile(const string &fileName)
{
    return _pfm->createFile(fileName);
}

RC IndexManager::destroyFile(const string &fileName)
{
    return _pfm->destroyFile(fileName);
}

RC IndexManager::openFile(const string &fileName, IXFileHandle & ixFileHandle)
{
    
    if (!_utils->fileExists(fileName)) {
        return -1;
    }
    // read binary and update
    ixFileHandle.pFile = fopen(fileName.c_str(), "rb+");
    ixFileHandle.fileName = fileName;
    
    if (ixFileHandle.pFile == NULL) {
        return -1;
    }
    
    // read persistent counters from disk
    _utils->readFileStatsFrom(fileName,
                              ixFileHandle.readPageCounter,
                              ixFileHandle.writePageCounter,
                              ixFileHandle.appendPageCounter);
    return 0;
}

RC IndexManager::closeFile(IXFileHandle & ixFileHandle)
{
    if (ixFileHandle.pFile == NULL) {
        // file wasn't opened.
        return -1;
    }
    _utils->writeFileStatsTo(ixFileHandle.fileName,
                             ixFileHandle.readPageCounter,
                             ixFileHandle.writePageCounter,
                             ixFileHandle.appendPageCounter);
    
    fclose(ixFileHandle.pFile);
    return 0;
}

RC IndexManager::_createBplusRootWith(IXFileHandle & ixFileHandle,
                                      const AttrType & keyType,
                                      const void * key,
                                      const RID & rid)
{   PageNum pageNum; // placeholder only, it should also be assigned 0.
    RC rc = _createNewLeafWith(ixFileHandle, pageNum, keyType, key, rid);
    return rc;
}
RC IndexManager::_createNewLeafWith(IXFileHandle & ixFileHandle,
                                    PageNum & pageNum,
                                    const AttrType & keyType,
                                    const void * key,
                                    const RID & rid)
{
    void * buffer = malloc(PAGE_SIZE);
    memset(buffer, EMPTY_BYTE, PAGE_SIZE);
    IndexNode leaf = IndexNode(buffer);
    NodeType nodeType = Leaf;
    leaf.setThisNodeType(nodeType);
    leaf.setFreeSpaceOfs(0);
    leaf.setNextNode(NO_MORE_PAGE);
    leaf.setKeyType(keyType);
    LeafTuple first = LeafTuple(key, keyType, rid);
    leaf.setLeafTupleAt(FIRST_TUPLE_OFS, first);
    leaf.setFreeSpaceOfs(first.getLength());
    ixFileHandle.appendPage(leaf.getBufferPtr());
    pageNum = ixFileHandle.getNumberOfPages() - 1; // pageIdx starts from 0
    free(buffer);
    return 0;
}

RC IndexManager::_moveHalfLeafTuplesFrom(IXFileHandle & ixFileHandle,
                                         IndexNode & leafNode,
                                         const short & totCounter,
                                         const AttrType & keyType,
                                         PageNum & newPage)
{
    short halfCounter = totCounter / 2 + 1;
    // find the starting point for the larger half
    const void * pagePtr = leafNode.getBufferPtr(); // page ptr
    LeafTuple curt;
    TupleID tid = {.pageNum = 0, .tupleOfs = 0}; // .pageNum placeholder
    for(int i = 0; i < halfCounter; i++) {
        const short ofs = tid.tupleOfs;
        curt = LeafTuple(pagePtr, ofs, keyType); // (halfCounter)th
        tid = curt.getNextTupleID();
    }
    // set up a new page
    const short halfStartOfs = tid.tupleOfs;
    curt = LeafTuple(pagePtr, halfStartOfs, keyType);
    _createNewLeafWith(ixFileHandle, newPage, keyType, curt.getKeyPtr(), curt.getRid());
    void * buffer = malloc(PAGE_SIZE);
    ixFileHandle.readPage(newPage, buffer);
    IndexNode newLeafNode = IndexNode(buffer);
    TupleID oldTid = curt.getNextTupleID();
    // move the rest to the new page
    for(int i = halfCounter + 1; i < totCounter; i++) {
        const short ofs = tid.tupleOfs;
        curt = LeafTuple(pagePtr, ofs, keyType); // (halfCounter + 1)th
        short freeSpaceOfs = newLeafNode.getFreeSpaceOfs();
        short nextTupleSpaceOfs = freeSpaceOfs + curt.getLength();
        TupleID nextTid = {.pageNum = newPage, .tupleOfs = nextTupleSpaceOfs};
        curt.setNextTupleID(nextTid);
        newLeafNode.setLeafTupleAt(freeSpaceOfs, curt);
        newLeafNode.setFreeSpaceOfs(nextTupleSpaceOfs);
        oldTid = curt.getNextTupleID();
    }
    // erase larger half in old page
    TODO : This is wrong way to do so.
        
    memset((char*)pagePtr + halfStartOfs, EMPTY_BYTE, leafNode.getFreeSpaceAmount());
    ixFileHandle.writePage(newPage, newLeafNode.getBufferPtr());
    return 0;
}

RC IndexManager::_insertIntoLeafAt(IXFileHandle & ixFileHandle,
                                   const PageNum & pageNum,
                                   const AttrType & keyType,
                                   const void * key,
                                   const RID & rid,
                                   void * newChildEntry)
{
    void * buffer = malloc(PAGE_SIZE);
    ixFileHandle.readPage(pageNum, buffer);
    IndexNode leaf = IndexNode(buffer);
    
    // search the end tuple within the page
    short counter = 0;
    const void * pagePtr = leaf.getBufferPtr(); // page ptr
    LeafTuple curt;
    TupleID tid = {.pageNum = pageNum, .tupleOfs = 0};
    do {
        counter += 1;
        const short tupleOfs = tid.tupleOfs;
        curt = LeafTuple(pagePtr, tupleOfs, keyType);
        tid = curt.getNextTupleID();
    } while(!_utils->sameTupleID(tid, NO_MORE_TUPLE) &&
            (tid.pageNum == pageNum)
            );
    // found the end tuple while curt holds the previous LeafTuple
    if (_utils->sameTupleID(tid, NO_MORE_TUPLE)) {
        LeafTuple tuple = LeafTuple(key, keyType, rid); // set next to NO_MORE
        TupleID tupleID;
        if (leaf.getFreeSpaceAmount() >= tuple.getLength()) {
            tupleID = {
                .pageNum = pageNum,
                .tupleOfs = leaf.getFreeSpaceOfs()
            };
            leaf.setLeafTupleAt(tupleID.tupleOfs, tuple);
            leaf.setFreeSpaceOfs(tupleID.tupleOfs + tuple.getLength());
            newChildEntry = nullptr;
        }
        else {
// MOVE HALF OF leafTuples TO ANOTHER PAGE
            _moveHalfLeafTuplesFrom(counter)
//            PageNum newPageNum;
//            _createNewLeafWith(ixFileHandle, newPageNum, keyType, key, rid);
//            tupleID = {
//                .pageNum = newPageNum,
//                .tupleOfs = 0
//            };
        }
        curt.setNextTupleID(tupleID);
        ixFileHandle.writePage(pageNum, leaf.getBufferPtr());
        // write change to old page anyway
    }
    else {
    // didn't find the end tuple
        _insertIntoLeafAt(ixFileHandle, tid.pageNum, keyType, key, rid);
    }
    return 0;
}

RC IndexManager::_insertIntoBplusTree(IXFileHandle & ixFileHandle,
                                   const PageNum & pageNum,
                                   const AttrType & keyType,
                                   const void * key,
                                   const RID & rid) {
    void * buffer = malloc(PAGE_SIZE);
    ixFileHandle.readPage(pageNum, buffer);
    IndexNode root = IndexNode(buffer);
    if (root.getKeyType() != keyType) {
        cout << "The keyType should be the same for entire tree." << endl;
        return -1;
    }
    if (root.getThisNodeType() == Leaf) {
        _insertIntoLeafAt(ixFileHandle, pageNum, keyType, key, rid);
    }
    else {
        PageNum nextPage = _searchForNextPageWithin(ixFileHandle, pageNum, key);
        _insertIntoBplusTree(ixFileHandle,
                             nextPage,
                             keyType,
                             key,
                             rid);
    }
    return 0;
}

RC IndexManager::insertEntry(IXFileHandle &ixFileHandle,
                             const Attribute &attribute,
                             const void *key,
                             const RID &rid)
{
    PageNum totPageNum = ixFileHandle.getNumberOfPages();
    if (totPageNum == 0) {
        _createBplusRootWith(ixFileHandle, attribute.type, key, rid);
    }
    else {
        _insertIntoBplusTree();
    }
    return -1;
}

RC IndexManager::deleteEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
    return -1;
}


RC IndexManager::scan(IXFileHandle &ixfileHandle,
        const Attribute &attribute,
        const void      *lowKey,
        const void      *highKey,
        bool			lowKeyInclusive,
        bool        	highKeyInclusive,
        IX_ScanIterator &ix_ScanIterator)
{
    return -1;
}

void IndexManager::printBtree(IXFileHandle &ixfileHandle, const Attribute &attribute) const {
}

IX_ScanIterator::IX_ScanIterator()
{
}

IX_ScanIterator::~IX_ScanIterator()
{
}

RC IX_ScanIterator::getNextEntry(RID &rid, void *key)
{
    return -1;
}

RC IX_ScanIterator::close()
{
    return -1;
}


IXFileHandle::IXFileHandle()
{
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
    fileName = "";
    pFile = NULL;
}

IXFileHandle::~IXFileHandle()
{
}

