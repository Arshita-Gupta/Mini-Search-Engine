#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "search_engine.h"
#include "text_processor.h"

// It will create and initialize search engine
SearchEngine* createSearchEngine() {
    SearchEngine* engine = (SearchEngine*)malloc(sizeof(SearchEngine));
    if (!engine) {
        fprintf(stderr, "Error: Memory allocation failed for SearchEngine.\n");
        return NULL;
    }

    engine->index = createHashTable();
    if (!engine->index) {
        fprintf(stderr, "Error: Failed to create hash table.\n");
        free(engine);
        return NULL;
    }

    engine->files = NULL;
    engine->numFiles = 0;
    return engine;
}

// It will build inverted index from given files
void indexFiles(SearchEngine* engine, char** filenames, int numFiles) {
    if (!engine || !filenames || numFiles <= 0) {
        fprintf(stderr, "Warning: Invalid input to indexFiles().\n");
        return;
    }

    engine->numFiles = buildInvertedIndex(engine->index, &engine->files, filenames, numFiles);

    if (engine->numFiles == 0) {
        fprintf(stderr, "Warning: No files were indexed. Check file paths or content.\n");
    }
}

// It will perform search for given query string
SearchResult* performSearch(SearchEngine* engine, const char* query) {
    if (!engine || !engine->index || !query || strlen(query) == 0) {
        fprintf(stderr, "Warning: Invalid search input.\n");
        return NULL;
    }

    return searchQuery(engine->index, engine->files, query);
}

// It will print engine statistics
void printEngineStats(SearchEngine* engine) {
    if (!engine) {
        fprintf(stderr, "Error: SearchEngine not initialized.\n");
        return;
    }

    printf("\n=== Search Engine Statistics ===\n");
    printf("Total indexed files: %d\n", engine->numFiles);

    if (engine->index)
        printf("Hash table size: %d entries\n", engine->index->size);
    else
        printf("Hash table not initialized.\n");

    int totalWords = 0;
    FileInfo* file = engine->files;
    while (file) {
        totalWords += file->totalWords;
        file = file->next;
    }
    printf("Total words indexed: %d\n", totalWords);
    printf("================================\n\n");
}

// It will free all the allocated memory
void freeSearchEngine(SearchEngine* engine) {
    if (!engine) return;

    if (engine->index) freeHashTable(engine->index);
    if (engine->files) freeFileList(engine->files);
    free(engine);
}