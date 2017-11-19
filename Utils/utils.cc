#include "utils.h"



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
                const void *decodedRec) {
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
