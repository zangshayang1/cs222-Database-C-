#include "node.h"

Node::Node(void * data)
{
    memcpy(_buffer, data, PAGE_SIZE);
}

/*
 * --------------------------------------------------------------------
 */

// explicitly inherit constructor from Node()
IndexNode::IndexNode(void * data)
{
    memcpy(_buffer, data, PAGE_SIZE); // is done by the header
}


RC IndexNode::initializeEmptyNode()
{
    memset(_buffer, EMPTY_BYTE, PAGE_SIZE);
    return 0;
}
RC IndexNode::initialize()
{
    _nodeType = *(NodeType*)((char*)_buffer + IDX_NODE_TYPE_INFO_OFS);
    _freeSpaceOfs = *(short*)((char*)_buffer + IDX_FREE_SPACE_INFO_OFS);
    _thisPage = *(PageNum*)((char*)_buffer + IDX_THIS_NODE_PAGENUM);
    _nextPage = *(PageNum*)((char*)_buffer + IDX_NEXT_NODE_PAGENUM);
    _keyType = (AttrType) *(int*)((char*)_buffer + IDX_KEY_TYPE_INFO_OFS);
    return 0;
}

RC IndexNode::_setFreeSpaceOfs(const short & freeSpaceOfs)
{
    memcpy((char*)_buffer + IDX_FREE_SPACE_INFO_OFS, & freeSpaceOfs, sizeof(short));
    _freeSpaceOfs = freeSpaceOfs;
    return 0;
}
RC IndexNode::_setThisNodeType(const NodeType & nodeType)
{
    memcpy((char*)_buffer + IDX_NODE_TYPE_INFO_OFS, & nodeType, sizeof(NodeType));
    _nodeType = nodeType;
    return 0;
}

RC IndexNode::_setThisPageNum(const PageNum & thisPage)
{
    memcpy((char*)_buffer + IDX_THIS_NODE_PAGENUM, & thisPage, sizeof(PageNum));
    _thisPage = thisPage;
    return 0;
}

RC IndexNode::_setNextPageNum(const PageNum & nextPage)
{
    memcpy((char*)_buffer + IDX_NEXT_NODE_PAGENUM, & nextPage, sizeof(PageNum));
    _nextPage = nextPage;
    return 0;
}
RC IndexNode::_setKeyType(const AttrType & keyType)
{
    memcpy((char*)_buffer + IDX_KEY_TYPE_INFO_OFS, & keyType, sizeof(int));
    _keyType = keyType;
    return 0;
}


RC IndexNode::setFreeSpaceOfs(const short & freeSpaceOfs) {
    return _setFreeSpaceOfs(freeSpaceOfs);
}
short IndexNode::getFreeSpaceOfs() const
{
    return _freeSpaceOfs;
}
short IndexNode::getFreeSpaceAmount() const
{
    return IDX_INFO_LEFT_BOUND_OFS - getFreeSpaceOfs();
}
RC IndexNode::setThisNodeType(const NodeType & nodeType)
{
    return _setThisNodeType(nodeType);
}
NodeType IndexNode::getThisNodeType() const
{
    return _nodeType;
}
RC IndexNode::setNextPageNum(const PageNum & nextPage)
{
    return _setNextPageNum(nextPage);
}
PageNum IndexNode::getNextPageNum() const
{
    return _nextPage;
}
RC IndexNode::setThisPageNum(const PageNum & pageNum)
{
    return _setThisPageNum(pageNum);
}
PageNum IndexNode::getThisPageNum() const
{
    return _thisPage;
}
RC IndexNode::setKeyType(const AttrType & keyType)
{
    return _setKeyType(keyType);
}
AttrType IndexNode::getKeyType() const
{
    return _keyType;
}

void * IndexNode::getBufferPtr()
{
    return _buffer;
}

RC IndexNode::_rollinToBufferLeafHelper(LeafTuple * t,
                                        void * bufferOfs)
{
    assert(t->getKeyType() == _keyType
           && "IndexNode:_rollinToBufferLeafHelper(): ERR.");
    
    short ofs;
    switch (t->getKeyType()) {
        case TypeInt:
            memcpy(bufferOfs, & t->intKey, sizeof(int));
            ofs = sizeof(int);
            break;
        case TypeReal:
            memcpy(bufferOfs, & t->fltKey, sizeof(float));
            ofs = sizeof(float);
            break;
        default:
            // c_str()
            memcpy(bufferOfs, t->strKey.c_str(), (size_t)t->strKey.length());
            ofs = t->strKey.length();
            break;
    }
    PageNum pageNum = t->getRid().pageNum;
    SlotNum slotNum = t->getRid().slotNum;
    memcpy((char*)bufferOfs + ofs, & pageNum, sizeof(PageNum));
    memcpy((char*)bufferOfs + ofs + sizeof(PageNum), & slotNum, sizeof(slotNum));
    return 0;
}

RC IndexNode::rollinToBuffer(LeafTuple * headptr)
{
    short increment = FIRST_TUPLE_OFS;
    
    // clear current _buffer of the page
    memset(_buffer, EMPTY_BYTE, IDX_INFO_LEFT_BOUND_OFS);
    
    while (headptr != nullptr && headptr->getKeyPtr() != nullptr) {
        assert(increment <= IDX_INFO_LEFT_BOUND_OFS &&
               "IndexNode::rollinToBuffer(LeafTupe) ERROR.");

        _rollinToBufferLeafHelper(headptr, (char*)_buffer + increment);
        increment += headptr->getLength();

//        if (headptr->getKeyPtr() != nullptr) {
//            _rollinToBufferLeafHelper(headptr, (char*)_buffer + increment);
//            increment += headptr->getLength();
//        }
        
        headptr = headptr->next;
    }
    _setFreeSpaceOfs(increment);
    return 0;
}

RC IndexNode::_rollinToBufferBranchHelper(BranchTuple* t,
                                          void* bufferOfs)
{
    assert(t->getKeyType() == _keyType
           && "IndexNode:_rollinToBufferBranchHelper(): ERR.");
    
    short ofs;
    switch (t->getKeyType()) {
        case TypeInt:
            memcpy(bufferOfs, & t->intKey, sizeof(int));
            ofs = sizeof(int);
            break;
        case TypeReal:
            memcpy(bufferOfs, & t->fltKey, sizeof(float));
            ofs = sizeof(float);
            break;
        case TypeVarChar:
            // c_str()
            memcpy(bufferOfs, t->strKey.c_str(), (size_t)t->strKey.length());
            ofs = t->strKey.length();
            break;
        default:
            cout << "IndexNode::_rollinToBufferBranchHelper() : ERR."<< endl;
            break;
    }
    PageNum leftChild = t->getLeftChild();
    PageNum rightChild = t->getRightChild();
    memcpy((char*)bufferOfs + ofs, & leftChild, sizeof(PageNum));
    memcpy((char*)bufferOfs + ofs + sizeof(PageNum), & rightChild, sizeof(PageNum));
    return 0;
}
RC IndexNode::rollinToBuffer(BranchTuple* headptr)
{
    short increment = FIRST_TUPLE_OFS;
    while (headptr != nullptr) {
        assert(increment <= IDX_INFO_LEFT_BOUND_OFS &&
               "IndexNode::rollinToBuffer(BranchTuple) ERROR.");
        _rollinToBufferBranchHelper(headptr, (char*)_buffer + increment);
        increment += headptr->getLength();
        headptr = headptr->next;
    }
    _setFreeSpaceOfs(increment);
    return 0;
}

RC IndexNode::rolloutOfBuffer(LeafTuple* & headptr)
{
    headptr = new LeafTuple(_buffer, FIRST_TUPLE_OFS, _keyType);
    LeafTuple * curs = headptr;
    LeafTuple * nextptr;
    short increment = curs->getLength();
    while (increment < _freeSpaceOfs) {
        nextptr = new LeafTuple(_buffer, increment, _keyType);
        curs->next = nextptr;
        curs = curs->next;
        increment += curs->getLength();
    }
    return 0;
}

RC IndexNode::rolloutOfBuffer(BranchTuple* &headptr)
{
    headptr = new BranchTuple(_buffer, FIRST_TUPLE_OFS, _keyType);
    BranchTuple* curs = headptr;
    BranchTuple* nextptr;
    short increment = headptr->getLength();
    while (increment < _freeSpaceOfs) {
        nextptr = new BranchTuple(_buffer, increment, _keyType);
        curs->next = nextptr;
        curs = curs->next;
        increment += curs->getLength();
    }
    return 0;
}

RC IndexNode::linearSearchBranchTupleForChild(const void * key,
                                              const AttrType & keyType,
                                              PageNum & nextPage)
{
    assert(_nodeType == Branch &&
           "IndexNode::_linearSearchForLeftChildOf(): ERROR.");
    
    BranchTuple* headptr;
    rolloutOfBuffer(headptr);
    
    if (key == nullptr) {
        nextPage = headptr->getLeftChild();
        return 0;
    }
    
    BranchTuple t = BranchTuple(key, keyType, 0, 0);
    
    BranchTuple* prevptr = nullptr;
    // put headptr to the first branchTuple that is larger than t
    // return the prevptr->rightChild()
    while (headptr != nullptr && *headptr <= t) {
        prevptr = headptr;
        headptr = headptr->next;
    }

    if (prevptr == nullptr) {
        // the first branchTuple is larger than t
        nextPage = headptr->getLeftChild();
    }
//    else if (headptr == nullptr) {
//        // the last branchTuple is smaller than t
//        nextPage = prevptr->getRightChild();
//    }
    else {
        nextPage = prevptr->getRightChild();
    }
    return 0;
}

RC IndexNode::linearSearchLeafTupleForKey(const bool & lowKeyInclusive,
                                          LeafTuple & lowerBoundTup,
                                          const AttrType &keyType,
                                          LeafTuple* &headptr)
{
    // search for the 1st leafTuple with a key that is >= than the given key
    
    rolloutOfBuffer(headptr);
    while (headptr->next != nullptr) {
        if (* headptr < lowerBoundTup) {
            headptr = headptr->next;
        }
        else if (* headptr == lowerBoundTup && !lowKeyInclusive) {
            headptr = headptr->next;
        }
        else {
            // found
            break;
        }
    }
    
    // within this node, there must be a leafTuple that is >= than the given key so if it comes to the end where head->next == nullptr, then the current head is supposed to be what we are looking for
    
    assert(* headptr >= lowerBoundTup &&
           "IndexNode::linearSearchLeafTupleForKey() : ERROR.");
    
    // could it be possible that it is not found in this Page?
    return 0;
}

RC IndexNode::clearAll()
{
    memset(getBufferPtr(), EMPTY_BYTE, IDX_INFO_LEFT_BOUND_OFS);
    _setFreeSpaceOfs(0);
    return 0;
}
/*
* --------------------------------------------------------------------
*/
RC Tuple::setKeyPtr(const void * keyPtr)
{
    _keyPtr = keyPtr;
    return 0;
}

AttrType Tuple::getKeyType()
{
    return _keyType;
}
const void * Tuple::getKeyPtr()
{
    return _keyPtr;
}
short Tuple::getLength() {
    return _length;
}

int Tuple::compare(const Tuple & t)
{
    switch (_keyType) {
        case TypeInt:
            if (intKey > t.intKey) {return 1;}
            if (intKey < t.intKey) {return -1;}
            if (intKey == t.intKey) {return 0;}
        case TypeReal:
            if (fltKey > t.fltKey) {return 1;}
            if (fltKey < t.fltKey) {return -1;}
            if (fltKey == t.fltKey) {return 0;}
        case TypeVarChar:
            if (strKey > t.strKey) {return 1;}
            if (strKey < t.strKey) {return -1;}
            if (strKey == t.strKey) {return 0;}
        default:
            cout << "Tuple:compare() : ERROR." << endl;
            return -1;
    }
}
bool Tuple::operator < (const Tuple & t) {return compare(t) < 0;}
bool Tuple::operator > (const Tuple & t) {return compare(t) > 0;}
bool Tuple::operator <= (const Tuple & t) {return compare(t) <= 0;}
bool Tuple::operator >= (const Tuple & t) {return compare(t) >= 0;}
bool Tuple::operator == (const Tuple & t) {return compare(t) == 0;}
bool Tuple::operator != (const Tuple & t) {return compare(t) != 0;}

/*
 * --------------------------------------------------------------------
 */

// LeafTuple Constructor 1: given key, keyType and rid;
LeafTuple::LeafTuple(const void * key,
                     const AttrType & keyType,
                     const RID & rid)
{
    _keyPtr = key;
    _rid = rid;
    _keyType = keyType;
    switch (_keyType) {
        case TypeInt:
            intKey = *(int*)_keyPtr;
            _ridOfs = sizeof(int);
            break;
        case TypeReal:
            fltKey = *(float*)_keyPtr;
            _ridOfs = sizeof(float);
            break;
        case TypeVarChar:
            strKey = _utils->getStringFrom(_keyPtr, 0);
            _ridOfs = sizeof(int) + strKey.length();
            break;
        default:
            break;
    }
    _length = _ridOfs + sizeof(PageNum) + sizeof(SlotNum);
}

// LeafTuple Constructor 2: init from page buffer
LeafTuple::LeafTuple(const void * buffer,
                     const short & tupleOfs,
                     const AttrType & keyType)
{
    _keyPtr = (char*)buffer + tupleOfs;
    _keyType = keyType;
    switch (keyType) {
        case TypeInt:
            intKey = *(int*)_keyPtr;
            _ridOfs = sizeof(int);
            break;
        case TypeReal:
            fltKey = *(float*)_keyPtr;
            _ridOfs = sizeof(float);
            break;
        case TypeVarChar:
            strKey = _utils->getStringFrom(_keyPtr, 0);
            _ridOfs = sizeof(int) + strKey.length();
            break;
        default:
            break;
    }
    _rid.pageNum = (PageNum) *(unsigned*)((char*)_keyPtr + _ridOfs);
    _rid.slotNum = (SlotNum) *(unsigned*)((char*)_keyPtr + _ridOfs + sizeof(PageNum));
    _length = _ridOfs + sizeof(PageNum) + sizeof(SlotNum);
}

RID LeafTuple::getRid()
{
    return _rid;
}

bool LeafTuple::exactMatch(LeafTuple & leafTuple)
{
    bool keyMatch;
    switch (_keyType) {
        case TypeInt:
            keyMatch = (intKey == leafTuple.intKey);
            break;
        case TypeReal:
            keyMatch = (fltKey == leafTuple.fltKey);
            break;
        case TypeVarChar:
            keyMatch = (strKey == leafTuple.strKey);
            break;
        default:
            break;
    }
    if (!keyMatch) {
        return false;
    }
    if (_rid.pageNum == leafTuple.getRid().pageNum &&
        _rid.slotNum == leafTuple.getRid().slotNum) {
        return true;
    }
    else {
        return false;
    }
}
/*
 * --------------------------------------------------------------------
 */

// BranchTuple Constructor 1: given key, keyType, left and right;
BranchTuple::BranchTuple(const void * key,
                         const AttrType & keyType,
                         const PageNum & left,
                         const PageNum & right)
{
    _keyPtr = key;
    _leftChild = left;
    _rightChild = right;
    _keyType = keyType;
    switch (_keyType) {
        case TypeInt:
            intKey = *(int*)_keyPtr;
            _leftOfs = sizeof(int);
            break;
        case TypeReal:
            fltKey = *(float*)_keyPtr;
            _leftOfs = sizeof(float);
            break;
        case TypeVarChar:
            strKey = _utils->getStringFrom(_keyPtr, 0);
            _leftOfs = sizeof(int) + strKey.length();
            break;
        default:
            cout << "BranchTuple::BranchTuple() : ERR." << endl;
            break;
    }
    _rightOfs = _leftOfs + sizeof(PageNum);
    _length = _rightOfs + sizeof(PageNum);
}

// BranchTuple Constructor 2: init from page buffer
BranchTuple::BranchTuple(void * data,
                         const short & tupleOfs,
                         const AttrType & keyType)
{
    _keyPtr = (char*)data + tupleOfs;
    _keyType = keyType;
    switch (_keyType) {
        case TypeInt:
            intKey = *(int*)_keyPtr;
            _leftOfs = sizeof(int);
            break;
        case TypeReal:
            fltKey = *(float*)_keyPtr;
            _leftOfs = sizeof(float);
            break;
        case TypeVarChar:
            strKey = _utils->getStringFrom(_keyPtr, 0);
            _leftOfs = sizeof(int) + strKey.length();
            break;
        default:
            break;
    }
    _rightOfs = _leftOfs + sizeof(PageNum); // PageNum _left;
    _leftChild = *(PageNum*)((char*)_keyPtr + _leftOfs);
    _rightChild = *(PageNum*)((char*)_keyPtr + _rightOfs);
    _length = _rightOfs + sizeof(PageNum);
}
PageNum BranchTuple::getLeftChild()
{
    return _leftChild;
}
PageNum BranchTuple::getRightChild()
{
    return _rightChild;
}
RC BranchTuple::setLeftChild(PageNum left)
{
    memcpy((char*)_keyPtr + _leftOfs, & left, sizeof(PageNum));
    return 0;
}
RC BranchTuple::setRightChild(PageNum right)
{
    memcpy((char*)_keyPtr + _rightOfs, & right, sizeof(PageNum));
    return 0;
}
