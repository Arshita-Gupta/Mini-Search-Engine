#ifndef INVERTED_H
#define INVERTED_H

#include "hash.h"   // keep this — your types come from here

//#ifndef MAX_FILENAME
//#define MAX_FILENAME 260   // fallback so this header is self-contained
//#endif

typedef struct FileInfo {
    int id;
    char filename[MAX_FILENAME];
    int totalWords;
    int uniqueWords;
    struct FileInfo* next;
} FileInfo;

typedef struct SearchResult {
    int fileId;
    char filename[MAX_FILENAME];
    float score;
    struct SearchResult* next;
} SearchResult;

FileInfo* createFileInfo(int id, const char* filename);
void addFileInfo(FileInfo** head, FileInfo* newFile);
int buildInvertedIndex(HashTable* index, FileInfo** files, char** filename, int numFiles);
SearchResult* searchQuery(HashTable* index, FileInfo* files, const char* query);
void printSearchResults(SearchResult* results);
void freeFileList(FileInfo* files);
void freeSearchResults(SearchResult* results);

#endif // INVERTED_H
