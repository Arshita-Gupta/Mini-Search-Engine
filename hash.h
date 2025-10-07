#ifndef HASH_H
#define HASH_H

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

// Hash table API
HashTable* createHashTable(void);
unsigned int hash(const char* word);
void insertWord(HashTable* table, const char* word, int fileid);
void freeHashTable(HashTable* table);
WordEntry* findWord(HashTable* table, const char* word);

#endif // HASH_H
