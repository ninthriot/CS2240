/*dbFIXER.c - Spencer Hunt CS2240 Assignment 4 - Revision 8
dbFIXER is a program which takes an argument at the command line which is a directory of database files which should be reconstructed into one file. This is done in parallel using the pthreads library.
Some terminal display helps the user see when each thread is created and when each thread begins sorting it's data. After sorting all the individual files, they are sent back using a pthread exit and a join in main.
The array of structs sent back from the thread is then placed in a two dimensional array of struct arrays. The arrays are then all sorted together by picking off the minimum value in the top of the arrays. When a minimum value is picked off, the corresponding struct is printed to a file called sorted.yay. The pointer to the most recent minimum value is incremented to point to the next value in that array. Integer "-10" in the di field is used to signify a null struct, and when sorting a -10 will not be considered in the minimum comparison. For the last value, all except one di value will be "-10" so we just find the outlier and print that struct which is our final one.
...
Revision 4: Added a checker to make sure the user specified a file.
Revision 5: Beautified Code and removed unused code, comments, and variables.
Revision 6: Further removed unused code, and changed makefile to use -Wall and -Werror.
Revision 7: Fixed the fileopen issue where the database could only be on the level of the program, now dbFIXER can read the files from any directory on the system! Woo-hoo!
Revision 8: All memory leaks have been patched using free()!
...
*/
#include <stdio.h>
#include <pthread.h>
#include "apue.h"
#include <dirent.h>
#include <stdlib.h>

     const int revision = 8;
     int mostLines;
     int t;
     int flag = 1;
     int numThreads = 0;
     int * threadNumLines;
     int z = 0;
     char tok[MAXLINE];
     char * dbpath = "./data";

     typedef struct {
     char * un;
     char * pw;
     char * bt;
     char * dn;
     int di;
     } DataBase;

int minNum(int a[], int n) {
    int c;
    int min;
    int index;
    min = a[0];
    index = 0;
    for (c = 1; c < n; c++) {
        if (a[c] < min && a[c] >= 0) {
            index = c;
            min = a[c];
        }
    }
    return index;
}

int compar(const void * a,
    const void * b) {
    DataBase * StructA = (DataBase * ) a;
    DataBase * StructB = (DataBase * ) b;
    return (StructA->di - StructB->di);
}

void * Sort(void * fn) {
    int numlines;  //number of lines in the file
    int j = 0;
    long sz;
    char * saveptr;//for strtok
    char * buf;    //for file
    char * buffer; //for strtok
    char a[7 + strlen((char * ) fn)]; //for file
    DataBase * users = malloc(1000 * sizeof(DataBase)); //Array of DataBase Structs

    printf("|\t\t\t |Thread %lu- Sorting file: %s       |\n",         pthread_self(), (char * ) fn);
    strcpy(a,dbpath);
    strcat(a,"/");
    strcat(a, (char * ) fn);
    FILE * frag = fopen(a, "r");
    fseek(frag, 0L, SEEK_END);
    sz = ftell(frag);
    rewind(frag);
    buf = calloc(1, sz + 1);
    strtok_r(buf, ",\n", & saveptr);
    fread(buf, sz, 1, frag);
    rewind(frag);

    while (1) {
            //Address 0x546516b is 0 bytes after a block of size 11 alloc'd... Think this is beacause buffer becomes NULL!
        if ((buffer = strtok_r(NULL, ",\n", & saveptr)) == NULL) {
            users[j].di = -10;
            break; //end of this file
        } else {
            users[j].un = malloc(sizeof(char) * strlen(buffer));
            memcpy(users[j].un,buffer,strlen(buffer));

            buffer = strtok_r(NULL, ",\n", & saveptr);
            users[j].pw = malloc(sizeof(char) * strlen(buffer));
            memcpy(users[j].pw,buffer,strlen(buffer));

            buffer = strtok_r(NULL, ",\n", & saveptr);
            users[j].bt = malloc(sizeof(char) * strlen(buffer));
            memcpy(users[j].bt,buffer,strlen(buffer));

            buffer = strtok_r(NULL, ",\n", & saveptr);
            users[j].dn = malloc(sizeof(char) * strlen(buffer));
            memcpy(users[j].dn,buffer,strlen(buffer));

            buffer = strtok_r(NULL, ",\n", & saveptr);
            users[j].di = atoi(buffer);
        }
        j++;
    }

    numlines = j;
    threadNumLines[z] = numlines;
    z++;
    fclose(frag);
    qsort(users, numlines, sizeof(DataBase), * compar);
    j = 0;
//PRINT SORTED DATA HERE FOR DEBUGGING
    free(buf);
    free(buffer);
    return(users);
}

int filework(char * path, char ** filenames) {
    int i = 0;
    DIR * d;
    struct dirent * dir;
    if ((d = opendir(path)) == NULL) err_sys("Error opening directory");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name,"..") == 0 || strcmp(dir->d_name,".") == 0) continue;
            filenames[i] = malloc(strlen(dir->d_name) + 1);
            strcpy(filenames[i], (void * ) dir->d_name);
            i++;
            numThreads++;
        }
        closedir(d);
    }

    return (0);
}

int main(int argc, char * argv[]) {
    char ** filenames = malloc(sizeof(char * ) * 30);
    int min;
    int w = 0;
    int * compare;
    FILE * finalFile;
    DataBase ** userDataPtr;

    if (argc != 2) {
        printf("Error! No database directory specified!\n");
        return 0;
    }

    dbpath = argv[1];
    DataBase ** userData; //2d array x is thread and y is struct data
    system("clear");
    printf("|------dbFIXER-v%u----|\n", revision);
    printf("Welcome to dbFIXER v%d!\nDB Path: %s\n", revision, dbpath);

    printf("-------------------------------------------------------------------------\n");

    filework(dbpath, filenames);

    threadNumLines = malloc(sizeof(int) * numThreads);
    userData = malloc(sizeof(DataBase * ) * numThreads);
    userDataPtr = malloc(sizeof(DataBase *) * numThreads);
    pthread_t threads[numThreads];

    for (t = 0; t < numThreads; t++) {
        printf("|Main- Creating thread: %u|                                              |\n", t);
        pthread_create( & threads[t], NULL, Sort, (void * ) filenames[t]);
    }

    for (t = 0; t < numThreads; t++) {
        pthread_join(threads[t], (void * ) &userData[t]);
    }

    memcpy(userDataPtr,userData,sizeof(DataBase *) * numThreads);

    printf("-------------------------------------------------------------------------\n");


    for (w=0; w < numThreads; w++) {
        mostLines += threadNumLines[w];
    }

    compare = malloc(numThreads * sizeof(int));

    finalFile = fopen("sorted.yay", "w");

    for (w = 0; w < mostLines; w++) {
        for (t = 0; t < numThreads; t++) {
            memcpy( &compare[t], &userData[t][0].di, sizeof(int));
            //printf("%d ",compare[t]); useful for debugging
        }

        min = minNum(compare, numThreads);

        if (w == mostLines - 1) {
            for (t = 0; t < numThreads; t++) {
                if (compare[t] != -10) min = t;
            }
        }

        //Valgrind reports invalid read size of 1... weird stuff, not too worried about it though.
        fprintf(finalFile, "%s,%s,%s,%s,%i\n", userData[min][0].un, userData[min][0].pw, userData[min][0].bt, userData[min][0].dn, userData[min][0].di);
        (userData[min])++;
    }

    for (t = 0; t < numThreads; t++) {
        free(filenames[t]);
        for(w=0;userDataPtr[t][w].di != -10;w++){
            free(userDataPtr[t][w].un);
            free(userDataPtr[t][w].pw);
            free(userDataPtr[t][w].bt);
            free(userDataPtr[t][w].dn);
        }
       free(userDataPtr[t]);
    }

    free(filenames);
    free(threadNumLines);
    free(userData);
    free(userDataPtr);
    free(compare);
    printf("\n");
    fclose(finalFile);
    return(0);
}
