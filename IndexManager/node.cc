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

RC IndexNode::rollinToBuffer(LeafTuple & head)
{
    LeafTuple * headptr = & head;
    short increment = FIRST_TUPLE_OFS;
    while (headptr != nullptr) {
        assert(increment <= IDX_INFO_LEFT_BOUND_OFS &&
               "IndexNode::rollinToBuffer() ERROR.");
        memcpy((char*)_buffer + increment,
               headptr->getKeyPtr(),
               headptr->getLength());
        increment += headptr->getLength();
        headptr = headptr->next;
    }
    _setFreeSpaceOfs(increment);
    return 0;
}
RC IndexNode::rolloutOfBuffer(LeafTuple & head)
{
    head = LeafTuple(_buffer, FIRST_TUPLE_OFS, _keyType);
    LeafTuple * headptr = & head;
    LeafTuple next;
    short increment = headptr->getLength();
    while (increment < _freeSpaceOfs) {
        next = LeafTuple(_buffer, increment, _keyType);
        headptr->next = & next;
        headptr = headptr->next;
        increment += headptr->getLength();
    }
    return 0;
}

RC IndexNode::rollinToBuffer(BranchTuple & head)
{
    BranchTuple * headptr = & head;
    short increment = FIRST_TUPLE_OFS;
    while (headptr != nullptr) {
        assert(increment <= IDX_INFO_LEFT_BOUND_OFS &&
               "IndexNode::rollinToBuffer() ERROR.");
        memcpy((char*)_buffer + increment,
               headptr->getKeyPtr(),
               headptr->getLength());
        increment += headptr->getLength();
        headptr = headptr->next;
    }
    _setFreeSpaceOfs(increment);
    return 0;
}

RC IndexNode::rolloutOfBuffer(BranchTuple & head)
{
    head = BranchTuple(_buffer, FIRST_TUPLE_OFS, _keyType);
    BranchTuple * headptr = & head;
    BranchTuple next;
    short increment = headptr->getLength();
    while (increment < _freeSpaceOfs) {
        next = BranchTuple(_buffer, increment, _keyType);
        headptr->next = & next;
        headptr = headptr->next;
        increment += headptr->getLength();
    }
    return 0;
}

RC IndexNode::linearSearchForChildOf(const void * key,
                                          const AttrType & keyType,
                                          PageNum & nextPage)
{
    assert(_nodeType == Branch &&
           "IndexNode::_linearSearchForLeftChildOf(): ERROR.");
    
    // TODO this should be LeafTuple RIGHT??? can I compare BranchTuple and LeafTuple?
    RID rid;
    LeafTuple t = LeafTuple(key, keyType, rid);
    
    BranchTuple head;
    rolloutOfBuffer(head);
    BranchTuple * prevptr = nullptr;
    BranchTuple * headptr = & head;
    while (headptr != nullptr && *headptr <= t) {
        prevptr = headptr;
        headptr = headptr->next;
    }
    if (prevptr == nullptr) {
        nextPage = headptr->getLeftChild();
    }
    else {
        nextPage = prevptr->getRightChild();
    }
    return 0;
}

/*
* --------------------------------------------------------------------
*/

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
            if (intKey < t.intKey) {return -1;}
            if (intKey > t.intKey) {return 1;}
            if (intKey == t.intKey) {return 0;}
        case TypeReal:
            if (fltKey < t.fltKey) {return -1;}
            if (fltKey > t.fltKey) {return 1;}
            if (fltKey == t.fltKey) {return 0;}
        case TypeVarChar:
            if (strKey < t.strKey) {return -1;}
            if (strKey > t.strKey) {return 1;}
            if (strKey == t.strKey) {return 0;}
        default:
            cout << "Tuple:compare() : ERROR." << endl;
            return 0;
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
    _length = _ridOfs + sizeof(PageNum) + sizeof(SlotNum);
}

// LeafTuple Constructor 2: init from page buffer
LeafTuple::LeafTuple(const void * buffer,
                     const short & tupleOfs,
                     const AttrType & keyType)
{
    _keyPtr = (char*)buffer + tupleOfs;
    
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
    switch (keyType) {
        case TypeInt:
            intKey = *(int*)_keyPtr;
            _leftOfs = sizeof(int);
            break;
        case TypeReal:
            fltKey = *(float*)_keyPtr;
            _leftOfs = sizeof(float);
        case TypeVarChar:
            strKey = _utils->getStringFrom(_keyPtr, 0);
            _leftOfs = sizeof(int) + strKey.length();
        default:
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
    switch (keyType) {
        case TypeInt:
            intKey = *(int*)_keyPtr;
            _leftOfs = sizeof(int);
            break;
        case TypeReal:
            fltKey = *(float*)_keyPtr;
            _leftOfs = sizeof(float);
        case TypeVarChar:
            strKey = _utils->getStringFrom(_keyPtr, 0);
            _leftOfs = sizeof(int) + strKey.length();
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








