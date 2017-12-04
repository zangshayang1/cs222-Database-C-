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
    if (ixFileHandle.pFile != nullptr) {
        return -1;
    }
    // read binary and update
    ixFileHandle.pFile = fopen(fileName.c_str(), "rb+");
    ixFileHandle.fileName = fileName;
    
    if (ixFileHandle.pFile == nullptr) {
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

/*
 * --------------------------------------------------------------------
 */

RC IndexManager::_createNewNode(const NodeType & nodeType,
                                const AttrType & keyType,
                                IndexNode & newNode)
{
    void * buffer = malloc(PAGE_SIZE);
    newNode = IndexNode(buffer); // memcpy of the buffer
    free(buffer);
    newNode.initializeEmptyNode();
    newNode.setKeyType(keyType);
    newNode.setNextPageNum(NO_MORE_PAGE);
    newNode.setFreeSpaceOfs(0);
    newNode.setThisNodeType(nodeType);
    
    return 0;
}

RC IndexManager::_initializeBplusRoot(const NodeType & nodeType,
                                      const AttrType & keyType,
                                      IndexNode & root)
{
    _createNewNode(nodeType, keyType, root);
    return 0;
}


RC IndexManager::_insertIntoLeafTupleList(IndexNode & leaf,
                                          LeafTuple & inserted,
                                          LeafTuple & head)
{
    leaf.rolloutOfBuffer(head);
    
    LeafTuple * prevptr = nullptr;
    LeafTuple * headptr = & head;
    while (headptr != nullptr && *headptr <= inserted)
    {
        prevptr = headptr;
        headptr = headptr->next;
    }
    if (prevptr == nullptr) {
        inserted.next = headptr;
        head = inserted;
    }
    else if (headptr == nullptr) {
        headptr->next = & inserted;
    }
    else {
        prevptr->next = & inserted;
        inserted.next = headptr;
    }
    return 0;
}

RC IndexManager::_insertIntoBranchTupleList(IndexNode & branch,
                                            BranchTuple & inserted,
                                            BranchTuple & head)
{
    branch.rolloutOfBuffer(head);
    
    BranchTuple * prevptr = nullptr;
    BranchTuple * headptr = & head;
    while (headptr != nullptr && *headptr <= inserted)
    {
        prevptr = headptr;
        headptr = headptr->next;
    }
    if (prevptr == nullptr) {
        inserted.next = headptr;
        head = inserted;
    }
    else if (headptr == nullptr) {
        headptr->next = & inserted;
    }
    else {
        prevptr->next = & inserted;
        inserted.next = headptr;
    }
    return 0;
}

RC IndexManager::_splitLeafTupleList(LeafTuple & first, LeafTuple & second)
{
    LeafTuple * fast = & first;
    LeafTuple * slow = & first;
    while (fast != nullptr && fast->next != nullptr) {
        fast = fast->next->next;
        slow = slow->next;
    }
    second = *slow->next;
    slow->next = nullptr;
    
    return 0;
}
RC IndexManager::_splitBranchTupleList(BranchTuple & first, BranchTuple & second)
{
    BranchTuple * fast = & first;
    BranchTuple * slow = & first;
    while (fast != nullptr && fast->next != nullptr) {
        fast = fast->next->next;
        slow = slow->next;
    }
    second = *slow->next;
    slow->next = nullptr;
    
    return 0;
}

RC IndexManager::_insertIntoLeaf(IndexNode & leaf,
                                 IndexNode * newChildPtr,
                                 IXFileHandle & ixFileHandle,
                                 const AttrType & keyType,
                                 const void * key,
                                 const RID & rid)
{
    LeafTuple inserted = LeafTuple(key, keyType, rid);
    
    short freeSpaceAmount = leaf.getFreeSpaceAmount(); // curt space
    
    // EMPTY PAGE
    if (freeSpaceAmount == IDX_INFO_LEFT_BOUND_OFS) {
        leaf.rollinToBuffer(inserted);
        ixFileHandle.writePage(leaf.getThisPageNum(), leaf.getBufferPtr());
        return 0;
    }
    
    LeafTuple first;
    _insertIntoLeafTupleList(leaf, inserted, first);
    
    // ENOUGH SPACE
    if (freeSpaceAmount >= inserted.getLength()) {
        leaf.rollinToBuffer(first);
        ixFileHandle.writePage(leaf.getThisPageNum(), leaf.getBufferPtr());
        return 0;
    }
    
    // NOT ENOUGH
    LeafTuple second;
    _splitLeafTupleList(first, second);
    IndexNode sibling;
    _createNewNode(Leaf, keyType, sibling);
    leaf.rollinToBuffer(first);
    sibling.rollinToBuffer(second);
    
    ixFileHandle.appendPage(sibling.getBufferPtr());
    PageNum newPageNum = ixFileHandle.getNumberOfPages() - 1;
    sibling.setThisPageNum(newPageNum);
    leaf.setNextPageNum(newPageNum);
    newChildPtr = & sibling;
    
    return 0;
}

RC IndexManager::_insertIntoBplusTree(IndexNode & root,
                                      IndexNode * newChildPtr,
                                      IXFileHandle & ixFileHandle,
                                      const AttrType & keyType,
                                      const void * key,
                                      const RID & rid) {
    // search should be implicitly done by the function itself recursively
    // that's how we can keep track of parent node naturally
    
    // sanity check
    assert(root.getKeyType() == keyType &&
           "IndexManager::_insertIntoBplusTree() : The keyType should be the same for entire tree.");
    
    if (root.getThisNodeType() == Leaf) {
        _insertIntoLeaf(root,
                        newChildPtr,
                        ixFileHandle,
                        keyType,
                        key,
                        rid);
    }
    else {
        // root is a branchNode
        PageNum nextPage;
        root.linearSearchForChildOf(key, keyType, nextPage);
        
        void * buffer = malloc(PAGE_SIZE);
        ixFileHandle.readPage(nextPage, buffer);
        IndexNode node = IndexNode(buffer);
        node.initialize();
        
        _insertIntoBplusTree(node,
                             newChildPtr,
                             ixFileHandle,
                             keyType,
                             key,
                             rid);
        if (newChildPtr != nullptr) {
            BranchTuple bubUpBtup = BranchTuple(newChildPtr->getBufferPtr(),
                                                newChildPtr->getKeyType(),
                                                node.getThisPageNum(),
                                                newChildPtr->getThisPageNum());
            // get current free space amount
            short freeSpaceAmount = root.getFreeSpaceAmount();
            // insert bubUpBtup into branch tuple linkedlist
            BranchTuple first;
            _insertIntoBranchTupleList(root, bubUpBtup, first);
            
            if (freeSpaceAmount > bubUpBtup.getLength()) {
                root.rollinToBuffer(first);
                ixFileHandle.writePage(root.getThisPageNum(), root.getBufferPtr());
            }
            else {
                BranchTuple second;
                _splitBranchTupleList(first, second);
                IndexNode sibling;
                _createNewNode(Branch, keyType, sibling);
                root.rollinToBuffer(first);
                sibling.rollinToBuffer(second);
                
                ixFileHandle.appendPage(sibling.getBufferPtr());
                PageNum newPageNum = ixFileHandle.getNumberOfPages() - 1;
                sibling.setThisPageNum(newPageNum);
                root.setNextPageNum(newPageNum);
                newChildPtr = & sibling; // ptr to the 1st key
            }
        }
    }
    return 0;
}

RC IndexManager::insertEntry(IXFileHandle &ixFileHandle,
                             const Attribute &attribute,
                             const void *key,
                             const RID &rid)
{
    if (rootptr == nullptr) {
        assert(ixFileHandle.getNumberOfPages() == 0 &&
               "IndexManager::insertEntry() : ERROR");
        _initializeBplusRoot(Leaf, attribute.type, root);
        ixFileHandle.appendPage(root.getBufferPtr());
        root.setThisPageNum(ixFileHandle.getNumberOfPages() - 1);
        rootptr = & root;
    }
    IndexNode * newChildPtr = nullptr;
    _insertIntoBplusTree(root,
                         newChildPtr,
                         ixFileHandle,
                         attribute.type,
                         key,
                         rid);
    // root hasn't been modified since the last change
    if (newChildPtr != nullptr) {
        // a new root is needed, expand in height
        IndexNode newRoot;
        _initializeBplusRoot(Branch, attribute.type, newRoot);
        BranchTuple bubUpBtup = BranchTuple(newChildPtr->getBufferPtr(),
                                                      newChildPtr->getKeyType(),
                                                      root.getThisPageNum(),
                                                      newChildPtr->getThisPageNum());
        
        newRoot.rollinToBuffer(bubUpBtup);
        ixFileHandle.appendPage(newRoot.getBufferPtr());
        newRoot.setThisPageNum(ixFileHandle.getNumberOfPages() - 1);
        root = newRoot;
    }
    return 0;
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
void IndexManager::_printLeafTuple(LeafTuple & leafTuple,
                                   const AttrType & keyType)
const {
    switch (keyType) {
        case TypeInt:
            cout << leafTuple.intKey;
            break;
        case TypeReal:
            cout << leafTuple.fltKey;
            break;
        case TypeVarChar:
            cout << leafTuple.strKey;
            break;
        default:
            break;
    }
    cout << " : ";
    RID rid = leafTuple.getRid();
    cout << "(" << rid.pageNum << "," << rid.slotNum << ")";
    return ;
}
void IndexManager::_printLeaf(IndexNode & node,
                              const AttrType & keyType)
const {
    LeafTuple head;
    
    node.rolloutOfBuffer(head);
    LeafTuple * headptr = & head;
    
    cout << "{KEYS: [";
    while (headptr->next != nullptr) {
        _printLeafTuple(*headptr, keyType);
        cout << ", ";
        headptr = headptr->next;
    }
    _printLeafTuple(*headptr, keyType);
    cout << "]}";
    return ;
    
}
void IndexManager::_printBranchTuple(BranchTuple & branchTuple,
                                     const AttrType & keyType)
const {
    switch (keyType) {
        case TypeInt:
            cout << branchTuple.intKey;
            break;
        case TypeReal:
            cout << branchTuple.fltKey;
            break;
        case TypeVarChar:
            cout << branchTuple.strKey;
            break;
        default:
            break;
    }
}
void IndexManager::_printBtreeHelper(PageNum & pageNum,
                                     IXFileHandle & ixFileHandle,
                                     const AttrType & keyType)
const {
    void * buffer = malloc(PAGE_SIZE);
    ixFileHandle.readPage(pageNum, buffer);
    IndexNode node = IndexNode(buffer);
    node.initialize();
    
    if (node.getThisNodeType() == Leaf) {
        _printLeaf(node, keyType);
        return ;
    }
    BranchTuple head;
    node.rolloutOfBuffer(head);
    BranchTuple * headptr = & head;
    
    cout << "{KEYS: [";
    vector<PageNum> children;
    while (headptr->next != nullptr) {
        _printBranchTuple(*headptr, keyType);
        cout << ",";
        PageNum left = headptr->getLeftChild();
        children.push_back(left);
        headptr = headptr->next;
    }
    _printBranchTuple(*headptr, keyType);
    children.push_back(headptr->getRightChild());
    cout << "]," << endl;
    cout << "CHILDRENS: [";
    for(PageNum child : children) {
        _printBtreeHelper(child, ixFileHandle, keyType);
    }
    cout << "]}" << endl;
}

void IndexManager::printBtree(IXFileHandle &ixFileHandle,
                              const Attribute &attribute)
const {
    PageNum rootPage = root.getThisPageNum();
    _printBtreeHelper(rootPage, ixFileHandle, attribute.type);
    return ;
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

