#include <stdio.h>
#include <string.h>
#include "common.h"

void read_newline(char* message){
    char *delim = "\\n";
    char *p;
    char buff_temp[MAX];
    strcpy(buff_temp, message);
    p =  str_split(buff_temp, delim);
    while( p != NULL){
        printf("%s\n", p);
        p = str_split(NULL, delim);
    }
}

char* str_split(char* str, char* delim){
    static char* str_rest;

    if (str != NULL){
        str_rest = str;
    }
    if(str_rest == NULL){
        return str_rest;
    }
    char *end = strstr(str_rest, delim);
    if(end == NULL){
        char *temp = str_rest;
        str_rest = NULL;
        return temp;
    }

    char *temp = str_rest;
    *end = '\0';
    str_rest = end + strlen(delim);
    return temp;
}
