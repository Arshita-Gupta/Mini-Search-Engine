

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



