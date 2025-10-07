#ifndef TEXT_PROCESSOR_H
#define TEXT_PROCESSOR_H

#include <stdio.h>
#include <stdlib.h>

#define MAX_WORD_LEN 100

char* preprocessText(const char* text);
int tokenizeText(const char* text, char tokens[][MAX_WORD_LEN], int maxtoken);
int isvalid(const char* word);

#endif // TEXT_PROCESSOR_H
