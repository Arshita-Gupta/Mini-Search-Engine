#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "inverted.h"
#include "text_processor.h"

int main ()
{
    HashTable* index = createHashTable();
    FileInfo* files = NULL;

    char *filename[]={
        "files/AI_1.txt",
        "files/AI_2.txt",
        "files/AI_3.txt",
        "files/AI_4.txt",
        "files/AI_5.txt",
        "files/AI_6.txt",
        "files/AI_7.txt",
        "files/AI_8.txt",
        "files/AI_9.txt",
        "files/AI_10.txt"
    };

    int numsfile= 10;
    int indexedFiles = buildInvertedIndex(index, &files, filename, numsfile);
    printf("Successfully indexed %d files.\n\n", indexedFiles);

    char query[256];
    while(1) {
        printf("Enter search query (or 'quit' to exit): ");
        if(!fgets(query, sizeof(query), stdin)) {
            break;
        }
        
        // Remove newline
        query[strcspn(query, "\n")] = 0;
        
        if(strcmp(query, "quit") == 0) {
            break;
        }
        
        if(strlen(query) == 0) {
            continue;
        }
        
        printf("\nSearching for: '%s'\n", query);
        printf("----------------------------------------\n");
        
        SearchResult* results = searchQuery(index, files, query);
        
        if(results) {
            printSearchResults(results);
            freeSearchResults(results);
        } else {
            printf("No results found.\n");
        }
        
        printf("----------------------------------------\n\n");
    }

    
    freeHashTable(index);
    freeFileList(files);
    return 0;
}
