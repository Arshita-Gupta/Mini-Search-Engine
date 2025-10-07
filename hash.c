#include "hash.h"

// Types are declared in hash.h

// same as structure(linked list we create a ADT where we initalize)
HashTable *createHashTable()
{
    HashTable *table = malloc(sizeof(HashTable));
    if (!table)
        return NULL;

    for (int i = 0; i < HASH_SIZE; i++)
    {
        table->buckets[i] = NULL; // because there must be some thing store already so we are intalize with NULL
    }
    table->size = 0;
    return table;
}

// converting word in number by djb2 alogorithm   hash ( where its follow the rule 5381)
unsigned int hash(const char *word)
{
    unsigned int hashValue = 5381;
    int c;

    while ((c = *word++))
    {
        hashValue = ((hashValue << 5) + hashValue) + c; // hash * 33 + c
    }

    return hashValue % HASH_SIZE;
}

void insertWord(HashTable *table, const char *word, int fileid)
{
    if (!hash || !word)
        return;

    int index = hash(word);
    WordEntry *entry = table->buckets[index];
    while (entry && strcmp(entry->word, word) != 0)
    {
        entry = entry->next;
    }
    if (!entry)
    {
        // creating a new word entry
        entry = malloc(sizeof(WordEntry));
        strcpy(entry->word, word);
        entry->postings = NULL;
        entry->next = table->buckets[index];  // using concept of insert at beginning
        table->buckets[index] = entry;
        table->size++;
    }
    PostingNode *posting = entry->postings;
    while (posting && posting->fileId != fileid)     // basically we are checking for the file is there any file for this 
    {
        posting = posting->next;
    }

    if (posting)
    {
        // increasing frequency
        posting->frequency++;
    }
    else
    {
        // new positing created
        posting = malloc(sizeof(PostingNode));
        posting->fileId = fileid;
        posting->frequency = 1;
        posting->next = entry->postings;
        entry->postings = posting;
    }
}

void freeHashTable(HashTable *table)
{
    if (!table)
        return;

    for (int i = 0; i < HASH_SIZE; i++)
    {
        WordEntry *entry = table->buckets[i];
        while (entry)
        {
            WordEntry *nextEntry = entry->next;

            PostingNode *posting = entry->postings;
            while (posting)
            {
                PostingNode *nextPosting = posting->next;
                free(posting);
                posting = nextPosting;
            }

            free(entry);
            entry = nextEntry;
        }
    }

    free(table);
}

WordEntry* findWord(HashTable* table, const char* word) {             // its  a helper for search funcition aas give particular index 
    if(!table || !word) return NULL;
    
    unsigned int index = hash(word);
    WordEntry* entry = table->buckets[index];
    
    while(entry && strcmp(entry->word, word) != 0) {
        entry = entry->next;
    }
    
    return entry;
}

// int main() 
// {
//     // Create a hash table
//     HashTable *table = createHashTable();
//     if (!table)
//     {
//         printf("Failed to create hash table.\n");
//         return 1;
//     }

//     // Insert words into the hash table
//     insertWord(table, "example", 1);
//     insertWord(table, "test", 2);
//     insertWord(table, "example", 1);
//     insertWord(table, "data", 3);

//     // Check if words are inserted correctly
//     char ch[10];
//     fgets(ch, 10, stdin);
//     ch[strcspn(ch, "\n")] = '\0';  
//     unsigned int index = hash(ch); // Pass the array directly
//     WordEntry *entry = table->buckets[index];

//     printf("Testing hash table:\n");
//     while (entry)
//     {
//         printf("Word: %s\n", entry->word);
//         PostingNode *posting = entry->postings;
//         while (posting)
//         {
//             printf("  File ID: %d, Frequency: %d\n", posting->fileId, posting->frequency);
//             posting = posting->next;
//         }
//         entry = entry->next;
//     }

//     // Free the hash table
//     freeHashTable(table);

//     return 0;
// }
