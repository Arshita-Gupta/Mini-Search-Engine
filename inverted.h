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

FileInfo* createFileInfo(int id, const char* filename);
void addFileInfo(FileInfo** head, FileInfo* newFile);
int buildInvertedIndex(HashTable* index, FileInfo** files, char** filename, int numFiles);
void freeFileList(FileInfo* files);

#endif // INVERTED_H
