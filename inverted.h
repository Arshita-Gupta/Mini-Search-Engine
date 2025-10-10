#ifndef INVERTED_H
#define INVERTED_H

#include "hash.h"

typedef struct FileInfo {
    int id;
    char filename[MAX_FILENAME];
    int totalWords;
    int uniqueWords;
    struct FileInfo* next;
} FileInfo;

// Search result structure
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
