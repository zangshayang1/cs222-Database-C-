#include "node.h"

Node::Node(void * data)
{
    memcpy(_buffer, data, PAGE_SIZE);
}
Node::~Node()
{
    free(_buffer);
    
}

IndexNode::IndexNode(void * data) : Node(data)
{
    memcpy(_buffer, data, PAGE_SIZE);
    _freeSpaceOfs = *(short*)((char*)_buffer + IDX_FREE_SPACE_INFO_OFS);
    _nodeType = *(NodeType*)((char*)_buffer + IDX_NODE_TYPE_INFO_OFS);
    _nextPage = *(PageNum*)((char*)_buffer + IDX_NEXT_NODE_INFO_OFS);
    _keyType = *(AttrType*)((char*)_buffer + IDX_KEY_TYPE_INFO_OFS);
}
IndexNode::~IndexNode() {
    free(_buffer);
}

RC IndexNode::setFreeSpaceOfs(const short & freeSpaceOfs)
{
    memcpy((char*)_buffer + IDX_FREE_SPACE_INFO_OFS, & freeSpaceOfs, sizeof(short));
    return 0;
}
short IndexNode::getFreeSpaceOfs()
{
    return _freeSpaceOfs;
}
short IndexNode::getFreeSpaceAmount()
{
    return IDX_INFO_LEFT_BOUND_OFS - getFreeSpaceOfs();
}

RC IndexNode::setThisNodeType(NodeType & nodeType)
{
    memcpy((char*)_buffer + IDX_NODE_TYPE_INFO_OFS, & nodeType, sizeof(NodeType));
    return 0;
}
NodeType IndexNode::getThisNodeType()
{
    return _nodeType;
}
RC IndexNode::setNextNode(PageNum nextPage)
{
    memcpy((char*)_buffer + IDX_NEXT_NODE_INFO_OFS, & nextPage, sizeof(PageNum));
    _nextPage = nextPage;
    return 0;
}
PageNum IndexNode::getNextNode()
{
    return _nextPage;
}
RC IndexNode::setKeyType(AttrType keyType)
{
    memcpy((char*)_buffer + IDX_KEY_TYPE_INFO_OFS, & keyType, sizeof(short));
    return 0;
}
AttrType IndexNode::getKeyType()
{
    return _keyType;
}
RC IndexNode::setLeafTupleAt(const short & tupleOfs, LeafTuple & leafTuple)
{
    assert(_nodeType == Leaf && "IndexNode::setLeafTupleAt() : this node is not leaf.");
    memcpy((char*)_buffer + tupleOfs, leafTuple.getKeyPtr(), leafTuple.getLength());
    return 0;
}
RC IndexNode::setBranchTupleAt(const short & tupleOfs, BranchTuple & branchTuple)
{
    assert(_nodeType == Branch && "IndexNode::setLeafTupleAt() : this node is not branch.");
    memcpy((char*)_buffer + tupleOfs, branchTuple.getKeyPtr(), branchTuple.getLength());
    return 0;
}

LeafTuple IndexNode::getFirstLeafTuple()
{
    assert(_nodeType == Leaf && "IndexNode::getFirstLeafTuple() : this node is not Leaf");
    return LeafTuple(_buffer, FIRST_TUPLE_OFS, _keyType);
}
BranchTuple IndexNode::getFirstBranchTuple()
{
    assert(_nodeType == Branch && "IndexNode::getFirstLeafTuple() : this node type is not Leaf");
    return BranchTuple(_buffer, FIRST_TUPLE_OFS, _keyType);
}

void * IndexNode::getBufferPtr()
{
    return _buffer;
}

// default constructor
Tuple::Tuple() {};

Tuple::~Tuple() {};

TupleID Tuple::getNextTupleID()
{
    return _nextTupleID;
}
RC Tuple::setNextTupleID(TupleID & tid)
{
    memcpy((char*)_keyPtr + _nextTupleOfs, & tid.pageNum, sizeof(PageNum));
    memcpy((char*)_keyPtr + _nextTupleOfs + sizeof(PageNum), & tid.tupleOfs, sizeof(short));
    return 0;
}
const void * Tuple::getKeyPtr()
{
    return _keyPtr;
}
short Tuple::getLength() {
    return _length;
}

// LeafTuple Constructor 0: default
LeafTuple::LeafTuple(){};
LeafTuple::~LeafTuple(){};

// LeafTuple Constructor 1: init from key ptr
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
        case TypeVarChar:
            strKey = _utils->getStringFrom(_keyPtr, 0);
            _ridOfs = sizeof(int) + strKey.length();
        default:
            break;
    }
    _setRid();
    _nextTupleOfs = _ridOfs + sizeof(PageNum) + sizeof(SlotNum);
    TupleID tid;
    tid.pageNum = NO_MORE_TUPLE.pageNum;
    tid.tupleOfs = NO_MORE_TUPLE.tupleOfs;
    setNextTupleID(tid); // use of _nextTupleOfs
    _length = _nextTupleOfs + sizeof(PageNum) + sizeof(short);
}
RC LeafTuple::_setRid()
{ // this private func is only invoked in LeafTuple Constructor 1
    memcpy((char*)_keyPtr + _ridOfs, & _rid.pageNum, sizeof(PageNum));
    memcpy((char*)_keyPtr + _ridOfs + sizeof(PageNum), & _rid.slotNum, sizeof(SlotNum));
    return 0;
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
        case TypeVarChar:
            strKey = _utils->getStringFrom(_keyPtr, 0);
            _ridOfs = sizeof(int) + strKey.length();
        default:
            break;
    }
    _rid = _getRid();
    _nextTupleOfs = _ridOfs + sizeof(PageNum) + sizeof(SlotNum);
    TupleID _nextTupleID = getNextTupleID(); // use of _nextTupleOfs
    _length = _nextTupleOfs + sizeof(PageNum) + sizeof(short);
}
RID LeafTuple::_getRid()
{
    RID rid;
    rid.pageNum = *(PageNum*)((char*)_keyPtr + _ridOfs, sizeof(PageNum));
    rid.slotNum = *(SlotNum*)((char*)_keyPtr + _ridOfs + sizeof(PageNum), sizeof(SlotNum));
    return rid;
}
RID LeafTuple::getRid()
{
    return _rid;
}

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
    _nextTupleOfs = _leftOfs + 2 * sizeof(PageNum);
    _length = _nextTupleOfs + sizeof(PageNum) + sizeof(short);
    
    _leftChild = *(PageNum*)((char*)_keyPtr + _leftOfs);
    _rightChild = *(PageNum*)((char*)_keyPtr + _rightOfs);
    _nextTupleID = _utils->getTupleIdAt((char*)_keyPtr + _nextTupleOfs);
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








