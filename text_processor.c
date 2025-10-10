#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_WORD_LEN 100

char* lowerCase(char* s){
    int i = 0;
    while (s[i] != '\0') {
        s[i] = tolower((unsigned char)s[i]);
        i++;
    }
    return s;
}

char* removePunc(char* s){
    for (int i = 0; s[i]; i++) {
        if (isalnum((unsigned char)s[i]) || s[i] == ' ') {
        } 
        else {
            s[i] = ' ';
        }
    }
    return s;
}

char* preprocessText(const char* text){
    if (!text) return NULL;
    int len = strlen(text);
    char* processed = (char*)malloc((len + 1) * sizeof(char));
    if (!processed) return NULL;
    strcpy(processed, text);

    lowerCase(processed);
    removePunc(processed);
    return processed;
}

int isvalid(const char* word){
    if (!word) return 0;
    int i = 0;
    while (word[i] != '\0'){
        if (isalpha((unsigned char)word[i]))
            return 1;
        i++;
    }
    return 0;
}

int tokenizeText(const char* text, char tokens[][MAX_WORD_LEN], int maxtoken){
    if (!text) return 0;

    char* temp = (char*)malloc((strlen(text) + 1) * sizeof(char));
    if (!temp) return 0;
    strcpy(temp, text);

    int tokencount = 0;
    char* token = strtok(temp, " \n\t\r");

    while (token && tokencount < maxtoken) {
        if (isvalid(token)){
            strncpy(tokens[tokencount], token, MAX_WORD_LEN - 1);
            tokens[tokencount][MAX_WORD_LEN - 1] = '\0';
            tokencount++;
        }
        token = strtok(NULL, " \n\t\r");
    }
    free(temp);
    return tokencount;
}