
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define HASH_SIZE 10007
#define MAX_WORD_LEN 100
#define MAX_FILENAME 256

// Posting list node for inverted index
typedef struct PostingNode {
    int fileId;
    int frequency;
    struct PostingNode* next;
} PostingNode;

// Word entry in hash table
typedef struct WordEntry {
    char word[MAX_WORD_LEN];
    PostingNode* postings;
    struct WordEntry* next;
} WordEntry;

// Hash table structure
typedef struct HashTable {
    WordEntry* buckets[HASH_SIZE];
    int size;
} HashTable;


// same as structure(linked list we create a ADT where we initalize)
HashTable* createHashTable()
{
    HashTable * table = malloc(sizeof(HashTable));
    if(!table) 
    return NULL;

    for( int i =0 ; i< HASH_SIZE; i++)
    {
        table->buckets[i]=NULL;  // because there must be some thing store already so we are intalize with NULL
    }
    table->size=0;
    return table;
}

// converting word in number by djb2 alogorithm   hash ( where its follow the rule 5381)
unsigned int hash(const char* word) {
    unsigned int hashValue = 5381;
    int c;
    
    while((c = *word++)) {
        hashValue = ((hashValue << 5) + hashValue) + c; // hash * 33 + c
    }
    
    return hashValue % HASH_SIZE;
}

void insertWord(HashTable * table,const char word ,int fileid)
{

}


void freeHashTable(HashTable* table) {
    if(!table) return;
    
    for(int i = 0; i < HASH_SIZE; i++) {
        WordEntry* entry = table->buckets[i];
        while(entry) {
            WordEntry* nextEntry = entry->next;
            
            PostingNode* posting = entry->postings;
            while(posting) {
                PostingNode* nextPosting = posting->next;
                free(posting);
                posting = nextPosting;
            }
            
            free(entry);
            entry = nextEntry;
        }
    }
    
    free(table);
}