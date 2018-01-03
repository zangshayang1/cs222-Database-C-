# UCI-cs222 Database Implementation 

![alt text](DB_struct.png)


## Xcode ENV
cs222-database.xcodeproj dir contains IDE-related files.

## Page-oriented File Manager and File Handler

FileManager implements:
1. createFile (plus relevant stats file that keeps track of the times of file being read/written/appended.)
2. destroyFile
3. openFile (abstract file related info into a fileHandle that deals with read/write/append behaviors.)
4. closeFile

FileHanle implements:
1. readPage
2. writePage
3. appendPage
4. getNumberOfPages
5. collectCounterValues (the times of the file being read/written/appended.)

## Record-based File Manager

It implements an abstraction one layer higher than page-oriented file manager.
1. insertRecord
2. deleteRecord
3. readRecord
4. printRecord
5. updateRecord (simply delete and insert and update slot info)
6. scan operation

Records come in stored in a page, marked with a starting offset and a length. The offset and the length are stored using 2 bytes for each in directory section of a page. The directory section of a page has as many slots as the records it holds, plus two additional slots for free space info and total number of slots. The rest of a page is used to store actual records. 

A record could consist of integer/float/string fields with each string field having a 4-byte leading integer indicating the length of the string. When a record comes in, it is in "encoded form" with the only difference being that: it has a metadata part, with enough number of bits indicating the nullibility of each field.  

## Relation Manager 

It implements typical DB behaviors such as insertion/deletion/update/scan, essentially an abstraction based on record-based file manager.  

It also implements DB initialization such as creating catalog tables, essentially two tables:
1. TABLE for storing info for each tables created (name, id, mode, etc).
2. COLUMN for storing columns info for each tables created (name, type, length, etc). 

The above two tables are either created or cached from disk every time the DB is restarted. 

## B+tree-based Index Manager and page-oriented Node Manager

There are 3 layers of abstraction here, from high to low:
1. B+tree abstraction: the whole index for some attribute of some table is a B+tree, whose basis is internal tree node and external tree node.
2. Node abstraction: both internal tree node and external tree node are 1-page storage, labeled differently and ofc storing different types of info.
3. Tuple abstraction: the whole tree is oriented by keys, keys from the actual records. Each key in external tree node should correspond to a pageNum/slotNum pair pointing to the specific position of the actual record. Each key in internal tree node should consist of a key, a left child pointer and a right child pointer. 

Therefore, I defined LeafTuple and BranchTuple abstraction and developed tuple behaviors such as compare/match/getChild/getRid/next/length, as well as node behavior such as nodeType/pageNum/freeSpace/rollinToBuffer/rolloutOfBuffer etc. Ofc, IndexManager has standard B+tree behavior such as create/open/close/destroy/insertion/lazyDeletion/print/scan etc.

## Utils
Define common util functions and global constants.

## Main Test Dir
cs222-database dir contains public/private test cases for FileManager/RelationManager/IndexManager.
 
