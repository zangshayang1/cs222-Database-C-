#include "utils.h"

/*
 * UtilsManager doesn't have an explicitly defined constructor
 * When we call new UtilsManager(), the implicit constructor will be invoked.
 * We intend to make it a globally unique instance
 * So we define a instance() method, and the following is the only place this method will be invoked.
 */

UtilsManager* UtilsManager::_utils_manager = 0;

UtilsManager* UtilsManager::instance()
{
    if (!_utils_manager) {
        _utils_manager = new UtilsManager();
    }
    return _utils_manager;
}

bool UtilsManager::fileExists(const string &filename)
{
    struct stat file_stat;
    if (stat(filename.c_str(), &file_stat) == 0) {
        return true;
    }
    return false;
}

string UtilsManager::statFileNameOf(const string & filename)
{
    string suffix = ".stat";
    string prefix = ".";
    return (prefix + filename + suffix);
}

void UtilsManager::print_char(const unsigned char oneChar) {
    unsigned char mask;
    cout << '[';
    for(int i = 0; i < 8; i++) {
        mask = (0x80 >> i);
        if ((oneChar & mask) == mask) {
            cout << '1';
        }
        else {
            cout << '0';
        }
    }
    cout << ']' << endl;
}
void UtilsManager::print_bytes(void *object, size_t size)
{
    // This is for C++; in C just drop the static_cast<>() and assign.
    //    const unsigned char * const bytes = static_cast<const unsigned char *>(object);
    unsigned char data[size];
    memcpy(data, object, size);
    for (int i = 0; i < size; i++) {
        cout << "char " << i << endl;
        print_char(data[i]);
    }
}


// this function returns string content from the given data chunk and offset
// NOTE that the first 4bytes make an int indicator of the strLen
string UtilsManager::getStringFrom(const void * data,
                                   const short & offset) {
    int strLen;
    memcpy(&strLen, (char*)data + offset, sizeof(int));
    // no Varchar entry takes memory more than 1 page
    char strVal[PAGE_SIZE];
    memcpy(strVal, (char*)data + offset + sizeof(int), (size_t) strLen);
    strVal[strLen] = '\0';
    return string(strVal);
}

void UtilsManager::printDecoded(const vector<Attribute> &recordDescriptor,
                                const void *decodedRec)
{
    string output;
    string tab = "\t";
    string newline = "\n";
    string nullstr = "NULL";
    
    auto fieldNum = (short) recordDescriptor.size();
    
    short thisFieldDataOfs = fieldNum * sizeof(short);
    for(int i = 0; i < recordDescriptor.size(); i++) {
        output += recordDescriptor[i].name;
        output += tab;
        
        auto nextFieldDataOfs = *(short*)((char*)decodedRec + i * sizeof(short));
        if (nextFieldDataOfs == ATTR_NULL_FLAG) {
            /*
             * if this field is NULL (encoded by nextFieldDataOfs == ATTR_NULL_FLAG)
             * then the current value of thisFieldDataOfs holds is the start of the next field.
             * if the next field is still NULL
             * thisFieldDataOfs still holds.
             */
            output += nullstr;
            output += newline;
            continue;
        }
        switch (recordDescriptor[i].type) {
            case TypeInt:
                int intVal;
                memcpy(&intVal, (char*)decodedRec + thisFieldDataOfs, sizeof(int));
                output += to_string(intVal);
                break;
            case TypeReal:
                float realVal;
                memcpy(&realVal, (char*)decodedRec + thisFieldDataOfs, sizeof(float));
                output += to_string(realVal);
                break;
            case TypeVarChar:
                string strVal = getStringFrom(decodedRec, thisFieldDataOfs);
                output += strVal;
                break;
        }
        output += newline;
        thisFieldDataOfs = nextFieldDataOfs;
    }
    cout << output << endl;
}

RC UtilsManager::readFileStatsFrom(const string & fileName,
                                   unsigned & readPageCounter,
                                   unsigned & writePageCounter,
                                   unsigned & appendPageCounter)
{
    FILE * fptr = fopen(statFileNameOf(fileName).c_str(), "rb+");
    void * buffer = malloc(sizeof(unsigned) * STAT_NUM);
    fread(buffer, sizeof(char), sizeof(unsigned) * STAT_NUM, fptr);
    memcpy(&readPageCounter, (char*)buffer + READ_PAGE_COUNTER_OFFSET, sizeof(unsigned));
    memcpy(&writePageCounter, (char*)buffer + WRITE_PAGE_COUNTER_OFFSET, sizeof(unsigned));
    memcpy(&appendPageCounter, (char*)buffer + APPEND_PAGE_COUNTER_OFFSET, sizeof(unsigned));
    free(buffer);
    fclose(fptr);
    
    return 0;
}

RC UtilsManager::writeFileStatsTo(const string & fileName,
                                  const unsigned & readPageCounter,
                                  const unsigned & writePageCounter,
                                  const unsigned & appendPageCounter)
{
    FILE * fptr = fopen(statFileNameOf(fileName).c_str(), "rb+");
    void * buffer = malloc(sizeof(unsigned) * STAT_NUM);
    memcpy((char*)buffer + READ_PAGE_COUNTER_OFFSET, & readPageCounter, sizeof(unsigned));
    memcpy((char*)buffer + WRITE_PAGE_COUNTER_OFFSET, & writePageCounter, sizeof(unsigned));
    memcpy((char*)buffer + APPEND_PAGE_COUNTER_OFFSET, & appendPageCounter, sizeof(unsigned));
    fwrite((char*)buffer, sizeof(char), sizeof(unsigned) * STAT_NUM, fptr);
    fflush(fptr);
    free(buffer);
    fclose(fptr);
    
    return 0;
}

RID UtilsManager::getRidAt(const void * data)
{
    RID rid;
    rid.pageNum = *(PageNum*)data;
    rid.slotNum = *(SlotNum*)((char*)data + sizeof(PageNum));
    return rid;
}
TupleID UtilsManager::getTupleIdAt(const void * data)
{
    TupleID tid;
    tid.pageNum = *(PageNum*)data;
    tid.tupleOfs = *(short*)((char*)data + sizeof(PageNum));
    return tid;
}
bool UtilsManager::sameTupleID(const TupleID & a, const TupleID & b)
{
    if (a.pageNum == b.pageNum && a.tupleOfs == b.tupleOfs) {
        return true;
    }
    else {
        return false;
    }
}

