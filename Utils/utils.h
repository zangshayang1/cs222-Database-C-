#ifndef _utils_h_
#define _utils_h_

#include <sys/stat.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>

// 4096
#define FREE_SPACE_INFO_POS 4094
// a 2bytes (short int) would be sufficient to repr free space in a 4KB page
#define SLOT_NUM_INFO_POS 4092

#define RIGHT_MOST_SLOT_OFFSET 4088

#define BITES_PER_BYTE 8

#define BEACON_SIZE 5

#define PAGE_SIZE 4096

using namespace std;

typedef unsigned PageNum;
typedef unsigned SlotNum;
typedef int RC;
typedef char byte;
typedef unsigned char CompressedPageNum[3];
typedef unsigned char CompressedSlotNum[2];

const short ATTR_NULL_FLAG = -1;
const short SLOT_OFFSET_CLEAN = -2;
const short SLOT_RECLEN_CLEAN = -3;
const int PAGENUM_UNAVAILABLE = -4;
const int EMPTY_BYTE = -5;
// memset() takes int but fill the block using unsigned char interpretation

// Attribute
typedef enum { TypeInt = 0, TypeReal = 1, TypeVarChar = 2 } AttrType;

typedef unsigned AttrLength;

struct Attribute {
    string   name;     // attribute name
    AttrType type;     // attribute type
    AttrLength length; // attribute length
};

// Record ID
typedef struct
{
    PageNum pageNum;    // page number
    SlotNum slotNum;    // slot number in the page
} RID;

// Beacon (used to indicate where the record is really located. It is needed when an updateRecord operation cannot be done in its original page. This way, once a RID is initialized when a record is initially inserted, it doesn't change.)
typedef struct
{
    CompressedPageNum cpsdPageNum; // compressed page number
    CompressedSlotNum cpsdSlotNum; // compressed slot number
} Beacon;



// Comparison Operator (NOT needed for part 1 of the project)
typedef enum { EQ_OP = 0, // no condition// =
    LT_OP,      // <
    LE_OP,      // <=
    GT_OP,      // >
    GE_OP,      // >=
    NE_OP,      // !=
    NO_OP       // no condition
} CompOp;


/********************************************************************************
 The scan iterator is NOT required to be implemented for the part 1 of the project
 ********************************************************************************/

# define RBFM_EOF (-1)  // end of a scan operator

class UtilsManager
{
public:
    static UtilsManager * instance();
    bool fileExists(const string & filename);
    string statFileNameOf(const string & filename);
    void print_char(const unsigned char oneChar);
    void print_bytes(void *object, size_t size);
    string getStringFrom(const void * data,
                         const short & offset);
    void printDecoded(const vector<Attribute> &recordDescriptor,
                      const void *decodedRec);

    
private:
    static UtilsManager * _utils_manager;
};

#endif
