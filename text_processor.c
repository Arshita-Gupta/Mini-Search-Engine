#include<stdio.h>
#include<string.h>
#include<ctype.h>

char* preprocessText(const char* text){
    if(!text) return NULL;
    int len = strlen(text);
    char* processed = (char*)malloc((len+1)*sizeof(char));
    strcpy(processed,text);

    processed=lowerCase(processed);
    processed=removePunc(processed);
}

char* lowerCase(char* s){
    int i=0;
    while(s[i]!='\0'){
        s[i]=tolower(s[i]);
        i++;
    }
    return s;
}

char* removePunc(char* s){
    for(int i=0;s[i];i++){
        if(isalnum(s[i]) || s[i]==' '){
            //eat 5star,do nothing ^)
        }
        else{
            s[i]=' ';
        }
    }
    return s;
}

int isvalid(const char* word){
    if(!word) return 0;
    int i=0;
    while(word[i]!='\0'){
        if(isalpha(word[i]))
        return 1;
        i++;
    }
    return 0;
}

int tokenizeText(const char* text,int maxtoken){
    if(!text)return 0;

    char* temp = (char*)malloc((strlen(text)+1)*sizeof(char));
    strcpy(temp,text);

    int tokencount = 0;
    char* token = strtok(temp," \n\t\r");

    while(token && tokencount<maxtoken){
        if(isvalid(token)){
            strcpy(token[tokencount],token);
            tokencount++;
        }
        token = strtok(NULL," \n\t\r");
    }
    free(temp);
    return tokencount;
}