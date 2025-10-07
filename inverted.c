#include "hash.c"
#include "text_processor.c"


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

int buildInvertedIndex(HashTable * index , FileInfo **files, char** filename , int numFiles)
{
    int succesfullindex = 0;
    for ( int i =0 ; i< numFiles; i++)
    {
        FILE *file=fopen(filename[i],"r");
        if(!file) {
            printf("Error: Cannot open file %s\n", filename[i]);
            continue;
        }
    FileInfo *fileInfo = createFileInfo(i,filename[i]);
    char buffer[1024];
    char tokens[500][MAX_WORD_LEN];
    printf("Processing file: %s\n", filename[i]);
    
    }
}

void freeFileList(FileInfo* files) {
    while(files) {
        FileInfo* next = files->next;
        free(files);
        files = next;
    }
}


