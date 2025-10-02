#include "hash.c"

// this is file informarion structure 
typedef struct FileInfo {
    int id;
    char filename[MAX_FILENAME];
    int totalWords;
    int uniqueWords;
    struct FileInfo* next;
} FileInfo;


FileInfo *createFileInfo(int id, const char *filename)
{
    FileInfo *file = malloc(sizeof(FileInfo));
    file->id = id;
    strcpy(file->filename, filename);
    file->totalWords=0;
    file->uniqueWords=0;
    file->next=NULL;
    return file ;
}

void addFileInfo(FileInfo** head, FileInfo* newFile) {
    newFile->next = *head;
    *head = newFile;
}



void freeFileList(FileInfo* files) {
    while(files) {
        FileInfo* next = files->next;
        free(files);
        files = next;
    }
}


