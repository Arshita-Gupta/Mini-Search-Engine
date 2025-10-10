#include "hash.h"
#include "text_processor.h"
#include "inverted.h"

FileInfo *createFileInfo(int id, const char *filename)
{
    FileInfo *file = malloc(sizeof(FileInfo));
    if (!file) return NULL;
    file->id = id;
    strncpy(file->filename, filename, MAX_FILENAME - 1);
    file->filename[MAX_FILENAME - 1] = '\0';
    file->totalWords = 0;
    file->uniqueWords = 0;
    file->next = NULL;
    return file;
}

void addFileInfo(FileInfo** head, FileInfo* newFile) {
    newFile->next = *head;
    *head = newFile;
}

int buildInvertedIndex(HashTable * index , FileInfo **files, char** filename , int numFiles)
{
    if (!index || !files || !filename || numFiles <= 0) return 0;

    int succesfullindex = 0;
    for ( int i = 0; i < numFiles; i++)
    {
        FILE *file = fopen(filename[i], "r");
        if (!file) {
            printf("Error: Cannot open file %s\n", filename[i]);
            continue;
        }

        FileInfo *fileInfo = createFileInfo(i + 1, filename[i]);
        if (!fileInfo) {
            fclose(file);
            continue;
        }

        char buffer[4096];
        char *processed = NULL;
        char tokens[1024][MAX_WORD_LEN];
        int totalWords = 0;

        // read file line by line
        while (fgets(buffer, sizeof(buffer), file)) {
            processed = preprocessText(buffer);
            if (!processed) continue;

            int n = tokenizeText(processed, tokens, 1024);
            for (int t = 0; t < n; t++) {
                insertWord(index, tokens[t], fileInfo->id);
                totalWords++;
            }

            free(processed);
            processed = NULL;
        }

        fileInfo->totalWords = totalWords;
        // uniqueWords can be computed later; for now leave as 0
        addFileInfo(files, fileInfo);
        succesfullindex++;
        fclose(file);
    }

    return succesfullindex;
}

void freeFileList(FileInfo* files) {
    while(files) {
        FileInfo* next = files->next;
        free(files);
        files = next;
    }
}


SearchResult* searchQuery(HashTable* index, FileInfo* files, const char* query) {
    char tokens[50][MAX_WORD_LEN];
    char* processedQuery = preprocessText(query);
    int tokenCount = tokenizeText(processedQuery, tokens, 50);
    
    SearchResult* results = NULL;
    
    // Simple search: find files containing any query term
    for(int i = 0; i < tokenCount; i++) {
        WordEntry* wordEntry = findWord(index, tokens[i]);
        if(wordEntry) {
            PostingNode* posting = wordEntry->postings;
            while(posting) {
                // Check if this file is already in results
                SearchResult* existing = results;
                int found = 0;
                while(existing) {
                    if(existing->fileId == posting->fileId) {
                        existing->score += posting->frequency;
                        found = 1;
                        break;
                    }
                    existing = existing->next;
                }
                
                if(!found) {
                    SearchResult* newResult = malloc(sizeof(SearchResult));
                    newResult->fileId = posting->fileId;
                    
                    // Find filename
                    FileInfo* fileInfo = files;
                    while(fileInfo && fileInfo->id != posting->fileId) {
                        fileInfo = fileInfo->next;
                    }
                    
                    if(fileInfo) {
                        strcpy(newResult->filename, fileInfo->filename);
                    }
                    
                    newResult->score = posting->frequency;
                    newResult->next = results;
                    results = newResult;
                }
                
                posting = posting->next;
            }
        }
    }
    
    free(processedQuery);
    return results;
}


void printSearchResults(SearchResult* results) {
    if(!results) {
        printf("No results found.\n");
        return;
    }
    
    printf("Search Results:\n");
    SearchResult* current = results;
    int rank = 1;
    
    while(current) {
        printf("%d. %s (Score: %.1f)\n", rank++, current->filename, current->score);
        current = current->next;
    }
}

void freeSearchResults(SearchResult* results) {
    while(results) {
        SearchResult* next = results->next;
        free(results);
        results = next;
    }
}