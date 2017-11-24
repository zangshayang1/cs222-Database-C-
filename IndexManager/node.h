#ifndef _node_h_
#define _node_h_

#include "../Utils/utils.h"

const short IDX_NEXT_NODE_INFO_OFS = 4092; // [92 + 0000]
const short IDX_NODE_TYPE_INFO_OFS = 4088; // [88 + 0000]
const short IDX_FREE_SPACE_INFO_OFS = 4086; // [86 + 00]
const short IDX_KEY_TYPE_INFO_OFS = 4084;
const short IDX_INFO_LEFT_BOUND_OFS = 4084; // [86]

const PageNum NO_MORE_PAGE = pow(2, 32) - 1;
const short NO_TUPLE_OFS = pow(2, 16) - 1;
const TupleID NO_MORE_TUPLE = {
    .pageNum = NO_MORE_PAGE,
    .tupleOfs = NO_TUPLE_OFS
    };
const PageNum ROOT_PAGE = 0;
const short FIRST_TUPLE_OFS = 0;

using namespace std;

/*
 * class declaration
 */
class Node;
class IndexNode;
class Tuple;
class LeafTuple;
class BranchTuple;

/*
 * class definition
 */
class Node
{
public:
    Node(void * data);
    ~Node();
protected:
    void * _buffer = malloc(PAGE_SIZE);
};

class IndexNode : public Node
{
public:
    // constructor
    IndexNode(void * data);
    ~IndexNode();
    // free ofs
    RC setFreeSpaceOfs(const short & freeSpaceOfs);
    short getFreeSpaceOfs();
    short getFreeSpaceAmount();
    // type
    RC setThisNodeType(NodeType & nodeType);
    NodeType getThisNodeType();
    // next node
    RC setNextNode(PageNum next);
    PageNum getNextNode();
    // key type
    RC setKeyType(AttrType keyType);
    AttrType getKeyType();
    // get first tuple
    LeafTuple getFirstLeafTuple();
    BranchTuple getFirstBranchTuple();
    // set tuple
    RC setLeafTupleAt(const short & tupleOfs, LeafTuple & leafTuple);
    RC setBranchTupleAt(const short & tupleOfs, BranchTuple & branchTuple);
    // buffer
    void * getBufferPtr();
    
protected:
    short _freeSpaceOfs;
    NodeType _nodeType;
    PageNum _nextPage = NULL;
    AttrType _keyType;
    
};

class Tuple
{
public:
    string strKey = NULL;
    int intKey = NULL;
    float fltKey = NULL;
    
    // default constructor
    Tuple();
    ~Tuple();
    
    const void * getKeyPtr();
    
    TupleID getNextTupleID();
    RC setNextTupleID(TupleID & tid);
    
    short getLength();
    
protected:
    short _nextTupleOfs = 0;
    TupleID _nextTupleID;
    const void * _keyPtr;
    short _length = 0;
    UtilsManager * _utils;
};

class LeafTuple : public Tuple
{
public:
    LeafTuple(); // so that I can declare like this: LeafTuple curt;
    ~LeafTuple();
    
    LeafTuple(const void * key,
              const AttrType & keyType,
              const RID & rid);
    
    LeafTuple(const void * buffer,
              const short & tupleOfs,
              const AttrType & keyType);
    
    RID getRid();
    
protected:
    short _ridOfs = 0;
    RID _rid; // -> PageNum & SlotNum
private:
    RC _setRid();
    RID _getRid();
};

class BranchTuple : public Tuple
{
public:
    BranchTuple(void * data,
                const short & tupleOfs,
                const AttrType & keyType);
    ~BranchTuple();
    
    PageNum getLeftChild();
    RC setLeftChild(PageNum left);
    
    PageNum getRightChild();
    RC setRightChild(PageNum right);
protected:
    short _leftOfs = 0;
    short _rightOfs = 0;
    PageNum _leftChild;
    PageNum _rightChild;
};


#endif
