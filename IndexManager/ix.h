#ifndef _ix_h_
#define _ix_h_

#include "../Utils/utils.h"
#include "../FileManager/rbfm.h"
#include "node.h"


using namespace std;

class IX_ScanIterator;
class IXFileHandle;

class IndexManager {

public:
    
    static IndexManager* instance();
    IndexNode root;
    IndexNode * rootptr = nullptr;
    

    // Create an index file.
    RC createFile(const string &fileName);

    // Delete an index file.
    RC destroyFile(const string &fileName);

    // Open an index and return an ixfileHandle.
    RC openFile(const string &fileName, IXFileHandle &ixfileHandle);

    // Close an ixfileHandle for an index.
    RC closeFile(IXFileHandle &ixfileHandle);

    // Insert an entry into the given index that is indicated by the given ixfileHandle.
    RC insertEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid);

    // Delete an entry from the given index that is indicated by the given ixfileHandle.
    RC deleteEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid);

    // Initialize and IX_ScanIterator to support a range search
    RC scan(IXFileHandle &ixfileHandle,
            const Attribute &attribute,
            const void *lowKey,
            const void *highKey,
            bool lowKeyInclusive,
            bool highKeyInclusive,
            IX_ScanIterator &ix_ScanIterator);

    // Print the B+ tree in pre-order (in a JSON record format)
    void printBtree(IXFileHandle &ixfileHandle, const Attribute &attribute) const;

protected:
    IndexManager();
    ~IndexManager();

private:
    static IndexManager *_index_manager;
    PagedFileManager * _pfm;
    UtilsManager * _utils;
    
    bool _validIxFileHandle(const IXFileHandle & ixFileHandle) const;
    
    RC _createNewNode(const NodeType & nodeType,
                      const AttrType & keyType,
                      IndexNode* &newNode);
    
    RC _initializeBplusRoot(const NodeType & nodeType, const AttrType & keyType, IndexNode & root);
    
    RC _insertIntoLeafTupleList(IndexNode & leaf,
                                LeafTuple & inserted,
                                LeafTuple* &head);
    
    RC _insertIntoBranchTupleList(IndexNode & branch,
                                  BranchTuple & inserted,
                                  BranchTuple* &headptr);
    
    RC _splitLeafTupleList(LeafTuple * first, LeafTuple* &second);
    
    RC _splitBranchTupleList(BranchTuple* first, BranchTuple* &second);
    
    RC _insertIntoLeaf(IndexNode & leaf,
                       IndexNode* &newChildPtr,
                       IXFileHandle & ixFileHandle,
                       const AttrType & keyType,
                       const void * key,
                       const RID & rid);
    
    RC _insertIntoBplusTree(IndexNode & root,
                            IndexNode* &newChildPtr,
                            IXFileHandle & ixFileHandle,
                            const AttrType & keyType,
                            const void * key, const RID & rid);
    
    void _printBtreeHelper(PageNum & pageNum,
                         IXFileHandle & ixFileHandle,
                         const AttrType & keyType) const;
    void _printBranchTuple(BranchTuple & branchTuple,
                           const AttrType & keyType) const;
    void _printLeaf(IndexNode & node,
                    const AttrType & keyType) const;
    void _printLeafTuple(LeafTuple & leafTuple,
                         const AttrType & keyType) const;
    
    RC _deleteEntryHelper(IXFileHandle & ixFileHandle,
                          const PageNum & pageNum,
                          const AttrType & keyType,
                          const void * key,
                          const RID & rid);
    
    RC _deleteFromLeaf(IndexNode & node,
                       const AttrType & keyType,
                       const void * key,
                       const RID & rid);
    
    RC _deleteFromLeafTupleList(LeafTuple * headptr,
                                const AttrType & keyType,
                                const void * key,
                                const RID & rid);
    
    void _printLeafTupleList(LeafTuple* first,
                             const AttrType & keyType);
    
    void _printBranchTupleList(BranchTuple* first,
                               const AttrType &keyType);
    
};

class IXFileHandle : public FileHandle {
    public:

/*
 * Inherited from the FileHandle class

    unsigned readPageCounter;
    unsigned writePageCounter;
    unsigned appendPageCounter;
    string fileName;
    FILE * pFile;
 
    RC readPage(PageNum pageNum, void *data);
    RC writePage(PageNum pageNum, const void *data);
    RC appendPage(const void *data);
    RC collectCounterValues(unsigned &readPageCount,
                            unsigned &writePageCount,
                            unsigned &appendPageCount);
    unsigned getNumberOfPages();
*/
    // Constructor
    IXFileHandle();

    // Destructor
    ~IXFileHandle();
};



class IX_ScanIterator {
public:
    // Constructor
    IX_ScanIterator() {};
    
    // Destructor
    ~IX_ScanIterator() {};
    
    // Get next matching entry
    RC getNextEntry(RID &rid, void* key);
    
    // Terminate index scan
    RC close();
    
    RC initialize(const PageNum & pageNum,
                  IXFileHandle & ixFileHandle,
                  const AttrType & keyType,
                  const void * lowKey,
                  const void * highKey,
                  bool lowKeyInclusive,
                  bool highKeyInclusive);
protected:
    IXFileHandle _ixFileHandle;
    AttrType _keyType;
    const void * _lowKey;
    const void * _highKey;
    bool _lowKeyInclusive;
    bool _highKeyInclusive;
    LeafTuple _lowerBoundTup;
    LeafTuple _higherBoundTup;
    
    LeafTuple* _leafTupCurs = nullptr;
    PageNum _pageCurs;
    IndexNode _nodeCurs;
    bool _ended = false;
    
    RC _putScanIteratorCursors();
    
    LeafTuple _lowestBoundTup(AttrType & keyType);
    LeafTuple _highestBoundTup(AttrType & keyType);
};

#endif
