#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include "hash.h"
#include "inverted.h"

// Main Search Engine structure
typedef struct SearchEngine {
    HashTable* index;
    FileInfo* files;
    int numFiles;
} SearchEngine;

// Function Declarations
SearchEngine* createSearchEngine();
void indexFiles(SearchEngine* engine, char** filenames, int numFiles);
SearchResult* performSearch(SearchEngine* engine, const char* query);
void printEngineStats(SearchEngine* engine);
void freeSearchEngine(SearchEngine* engine);

#endif