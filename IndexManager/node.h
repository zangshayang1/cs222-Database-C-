#ifndef _node_h_
#define _node_h_

#include "../Utils/utils.h"

const short IDX_NEXT_NODE_PAGENUM = 4092; //   [92 + 0000]
const short IDX_THIS_NODE_PAGENUM = 4088; //     [88 + 0000]
const short IDX_NODE_TYPE_INFO_OFS = 4084; //   [84 + 0000]
const short IDX_FREE_SPACE_INFO_OFS = 4082; //  [82 + 00]
const short IDX_KEY_TYPE_INFO_OFS = 4078; //    [80 + 00000]
const short IDX_INFO_LEFT_BOUND_OFS = 4078; //  [80]

const PageNum NO_MORE_PAGE = pow(2, 32) - 1;
const short NO_TUPLE_OFS = pow(2, 16) - 1;

const PageNum ROOT_PAGE = 0;
const short FIRST_TUPLE_OFS = 0;

using namespace std;

/*
 * forward declaration
 */
//class Node;
//class IndexNode;
//class Tuple;
class LeafTuple;
class BranchTuple;

/*
 * class definition
 */

class Tuple
{
public:
    // default constructor
    Tuple() {};
    ~Tuple() {};
    
    string strKey;
    int intKey;
    float fltKey;
    
    RC setKeyPtr(const void * keyPtr);
    const void * getKeyPtr();
    AttrType getKeyType();
    short getLength();
    
    bool operator < (const Tuple & t);
    bool operator > (const Tuple & t);
    bool operator <= (const Tuple & t);
    bool operator >= (const Tuple & t);
    bool operator == (const Tuple & t);
    bool operator != (const Tuple & t);
    
protected:
    int compare(const Tuple & t);
    
    AttrType _keyType;
    const void * _keyPtr;
    short _length = 0;
    UtilsManager * _utils;
};

class LeafTuple : public Tuple
{
public:
    LeafTuple() : Tuple() {};
    ~LeafTuple() {};
    
    LeafTuple * next = nullptr;
    
    LeafTuple(const void * key,
              const AttrType & keyType,
              const RID & rid);
    
    LeafTuple(const void * buffer,
              const short & tupleOfs,
              const AttrType & keyType);
    
    bool exactMatch(LeafTuple & leafTuple);
    
    RID getRid();
    
protected:
    short _ridOfs = 0;
    RID _rid; // -> PageNum & SlotNum
private:
};

class BranchTuple : public Tuple
{
public:
    BranchTuple() : Tuple() {};
    ~BranchTuple() {};
    
    BranchTuple(void * data,
                const short & tupleOfs,
                const AttrType & keyType);
    
    BranchTuple(const void * key,
                const AttrType & keyType,
                const PageNum & left,
                const PageNum & right);
    
    BranchTuple * next = nullptr;
    
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


class Node
{
public:
    Node() {};
    ~Node() {};
    
    Node(void * data);
    
protected:
    void * _buffer = malloc(PAGE_SIZE);
    // void * _buffer; seems give it a NULL address WTF?
};

class IndexNode : public Node
{
public:
    // default
    IndexNode() {};
    ~IndexNode() {};
    
    // constructor
    IndexNode(void * data);

    // free ofs
    RC setFreeSpaceOfs(const short & freeSpaceOfs);
    short getFreeSpaceOfs() const;
    short getFreeSpaceAmount() const;
    // type
    RC setThisNodeType(const NodeType & nodeType);
    NodeType getThisNodeType() const;
    // _thisPage
    RC setThisPageNum(const PageNum & pageNum);
    PageNum getThisPageNum() const;
    // _nextPage
    RC setNextPageNum(const PageNum & pageNum);
    PageNum getNextPageNum() const;
    // key type
    RC setKeyType(const AttrType & keyType);
    AttrType getKeyType() const;
    // get first tuple
    LeafTuple getFirstLeafTuple();
    BranchTuple getFirstBranchTuple();
    
    RC initializeEmptyNode();
    RC initialize();
    RC clearAll();
    
    RC rollinToBuffer(LeafTuple & head);
    RC rolloutOfBuffer(LeafTuple & head);
    RC rollinToBuffer(BranchTuple & head);
    RC rolloutOfBuffer(BranchTuple & head);
    
    RC linearSearchBranchTupleForChild(const void * key, const AttrType & keyType, PageNum & nextPage);
    
    RC linearSearchLeafTupleForKey(const bool & lowKeyInclusive,
                                   LeafTuple & lowerBoundTup,
                                   const AttrType & keyType,
                                   LeafTuple & head);
    
    // buffer
    void * getBufferPtr();
    
protected:
    short _freeSpaceOfs;
    NodeType _nodeType;
    PageNum _nextPage = NULL;
    AttrType _keyType;
    PageNum _thisPage = NULL;
    
    RC _setFreeSpaceOfs(const short & freeSpaceOfs);
    RC _setThisNodeType(const NodeType & nodeType);
    RC _setThisPageNum(const PageNum & thisPage);
    RC _setNextPageNum(const PageNum & nextPage);
    RC _setKeyType(const AttrType & keyType);
    
};



#endif
