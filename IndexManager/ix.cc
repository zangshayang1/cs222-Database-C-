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
        root.linearSearchBranchTupleForChild(key, keyType, nextPage);
        
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
            // bubUpBtup has the same key as newChild (rightChild)
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
    // burn updates to disk
    ixFileHandle.writePage(root.getThisPageNum(), root.getBufferPtr());
    
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
        newRoot.setThisPageNum(ixFileHandle.getNumberOfPages());
        // burn newRoot into new page
        ixFileHandle.appendPage(newRoot.getBufferPtr());
        root = newRoot;
        rootptr = & root;
    }
    return 0;
}

/*
 * --------------------------------------------------------------------
 */

RC IndexManager::_deleteFromLeafTupleList(LeafTuple & head,
                                          const AttrType & keyType,
                                          const void * key,
                                          const RID & rid)
{
    LeafTuple curtKey = LeafTuple(key, keyType, rid);
    LeafTuple * prevptr = nullptr;
    LeafTuple * headptr = & head;
    while (headptr->next != nullptr) {
        if (curtKey.exactMatch(* headptr)) {
            break;
        }
        prevptr = headptr;
        headptr = headptr->next;
    }

    if (prevptr == nullptr && headptr->next == nullptr) {
        // only 1 leafTuple in this list
        head.setKeyPtr(nullptr);
    }
    else if (prevptr == nullptr) {
        // the 1st leafTuple is what we want to delete
        head = * head.next;
    }
    else if (head.next == nullptr) {
        // run through the list without finding what we want to delete
        return -1;
    }
    else {
        // found it
        prevptr->next = headptr->next;
    }
    return 0;
}

RC IndexManager::_deleteFromLeaf(IndexNode & node,
                                 const AttrType & keyType,
                                 const void * key,
                                 const RID & rid)
{
    if (node.getFreeSpaceOfs() == 0) {
        // page is empty
        return -1;
    }
    LeafTuple head;
    node.rolloutOfBuffer(head);
    RC rc = _deleteFromLeafTupleList(head, keyType, key, rid);
    if (rc == -1) {
        // didn't find what we are looking for
        // no change needed
        return -1;
    }
    // roll tuplelist back to the page
    node.rollinToBuffer(head);
    return 0;
    
}

RC IndexManager::_deleteEntryHelper(IXFileHandle & ixFileHandle,
                                    const PageNum & pageNum,
                                    const AttrType & keyType,
                                    const void * key,
                                    const RID & rid)
{
    void * buffer = malloc(PAGE_SIZE);
    ixFileHandle.readPage(pageNum, buffer);
    IndexNode node = IndexNode(buffer);
    node.initialize();
    
    if (node.getThisNodeType() == Leaf) {
        RC rc = _deleteFromLeaf(node, keyType, key, rid);
        if (rc == -1) {
            // didn't even find, delete should fail
            return -1;
        }
        // burn deletion onto disk
        ixFileHandle.writePage(node.getThisPageNum(), node.getBufferPtr());
        return 0;
    }
    PageNum childPage;
    node.linearSearchBranchTupleForChild(key, keyType, childPage);
    _deleteEntryHelper(ixFileHandle, childPage, keyType, key, rid);
    return 0;
}
RC IndexManager::deleteEntry(IXFileHandle &ixFileHandle,
                             const Attribute &attribute,
                             const void *key,
                             const RID &rid)
{
    PageNum rootPage = root.getThisPageNum();
    
    return _deleteEntryHelper(ixFileHandle,
                              rootPage,
                              attribute.type,
                              key,
                              rid);
}



/*
 * --------------------------------------------------------------------
 */

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

/*
 * --------------------------------------------------------------------
 */

RC IndexManager::scan(IXFileHandle &ixFileHandle,
                      const Attribute &attribute,
                      const void * lowKey,
                      const void * highKey,
                      bool lowKeyInclusive,
                      bool highKeyInclusive,
                      IX_ScanIterator &ix_ScanIterator)
{
    ix_ScanIterator.initialize(root.getThisPageNum(),
                               ixFileHandle,
                               attribute.type,
                               lowKey,
                               highKey,
                               lowKeyInclusive,
                               highKeyInclusive);
    return 0;
}

/*
 * --------------------------------------------------------------------
 */

RC IX_ScanIterator::_putScanIteratorCursors()
{
    if (_nodeCurs.getThisNodeType() == Leaf) {
        LeafTuple head;
        _nodeCurs.linearSearchLeafTupleForKey(_lowKeyInclusive,
                                              _lowerBoundTup,
                                              _keyType,
                                              head);
        _leafTupCurs = head;
        _pageCurs = _nodeCurs.getThisPageNum();
        return 0;
    }
    // _nodeCurs.getThisNodeType() == Branch
    PageNum childpage;
    _nodeCurs.linearSearchBranchTupleForChild(_lowKey, _keyType, childpage);
    void * buffer = malloc(PAGE_SIZE);
    _ixFileHandle.readPage(childpage, buffer);
    _nodeCurs = IndexNode(buffer);
    _nodeCurs.initialize();
    _putScanIteratorCursors();
    return 0;
}

LeafTuple IX_ScanIterator::_lowestBoundTup(AttrType & keyType)
{
    void * key = nullptr;
    int minInt = INT_MIN;
    string minStr = "";
    switch (keyType) {
        case TypeInt:
            key = & minInt;
            break;
        case TypeReal:
            key = & minInt;
        case TypeVarChar:
            key = & minStr;
        default:
            cout << "IX_ScanIterator::_lowestBoundTup(): ERR." << endl;
            break;
    }
    RID rid;
    rid.pageNum = -1;
    rid.slotNum = -1;
    return LeafTuple(key, keyType, rid);
}

LeafTuple IX_ScanIterator::_highestBoundTup(AttrType & keyType)
{
    void * key = nullptr;
    int maxInt = INT_MAX;
    
    char maxCharArr[PAGE_SIZE];
    for (int i = 0; i < PAGE_SIZE - 1; i++) {
        maxCharArr[i] = '~'; // ascii value 126
    }
    maxCharArr[PAGE_SIZE - 1] = '\0';
    string maxStr = string(maxCharArr);
    
    switch (keyType) {
        case TypeInt:
            key = & maxInt;
            break;
        case TypeReal:
            key = & maxInt;
        case TypeVarChar:
            key = & maxStr;
        default:
            cout << "IX_ScanIterator::_highestBoundTup(): ERR." << endl;
            break;
    }
    RID rid;
    rid.pageNum = -1;
    rid.slotNum = -1;
    return LeafTuple(key, keyType, rid);
}

RC IX_ScanIterator::initialize(const PageNum & rootPageNum,
                               IXFileHandle & ixFileHandle,
                               const AttrType & keyType,
                               const void * lowKey,
                               const void * highKey,
                               bool lowKeyInclusive,
                               bool highKeyInclusive)
{
    void * buffer = malloc(PAGE_SIZE);
    ixFileHandle.readPage(rootPageNum, buffer);
    IndexNode root = IndexNode(buffer);
    root.initialize();
    _nodeCurs = root;
    _pageCurs = _nodeCurs.getThisPageNum();
    
    _ixFileHandle = ixFileHandle;
    _keyType = keyType;
    _lowKey = lowKey;
    _highKey = highKey;
    _lowKeyInclusive = lowKeyInclusive;
    _highKeyInclusive = highKeyInclusive;
    
    // deal with cases where _lowKey is NULL, _highKey is NULL
    if (_lowKey == NULL) {
        _lowerBoundTup = _lowestBoundTup(_keyType);
    }
    else {
        RID rid1;
        _lowerBoundTup = LeafTuple(_lowKey, keyType, rid1);
    }
    if (_highKey == NULL) {
        _higherBoundTup = _highestBoundTup(_keyType);
    }
    else {
        RID rid2;
        _higherBoundTup = LeafTuple(_highKey, keyType, rid2);
    }
    return _putScanIteratorCursors();
}

RC IX_ScanIterator::getNextEntry(RID &rid, void *key)
{
    if (_ended) {
        return -1;
    }
    
    if (_leafTupCurs > _higherBoundTup) {
        // no more eligible leafTuple
        return -1;
    }
    if (_leafTupCurs == _higherBoundTup && !_highKeyInclusive) {
        // exclusive
        return -1;
    }
    
    rid = _leafTupCurs.getRid();
    switch (_keyType) {
        case TypeInt:
            key = & _leafTupCurs.intKey;
            break;
        case TypeReal:
            key = & _leafTupCurs.fltKey;
            break;
        case TypeVarChar:
            key = & _leafTupCurs.strKey;
            break;
        default:
            break;
    }
    
    if (_leafTupCurs.next != nullptr) {
        _leafTupCurs = * _leafTupCurs.next;
    }
    // done traversing the current page
    else if (_nodeCurs.getNextPageNum() != NO_MORE_PAGE) {
        _pageCurs = _nodeCurs.getNextPageNum();
        void * buffer = malloc(PAGE_SIZE);
        _ixFileHandle.readPage(_pageCurs, buffer);
        _nodeCurs = IndexNode(buffer);
        _nodeCurs.initialize();
        _nodeCurs.rolloutOfBuffer(_leafTupCurs);
    }
    // done traversing the whole tree
    else {
        // mark that the _leafTupCurs is the last one
        _ended = true;
    }
    return 0;
}

RC IX_ScanIterator::close()
{
    return 0;
}


/*
 * --------------------------------------------------------------------
 */

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

