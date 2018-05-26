#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#define BLOCK_LOW(id, p, n)   ((id)*(n)/(p))
#define BLOCK_HIGH(id, p, n)  (BLOCK_LOW((id)+1, p, n)-1)
#define BLOCK_OWNER(index, p, n)  (((p)*((index)+1)-1)/(n))

char*** readFile(char *name,int *W,int *D,int *H,long *freq){
        //Debugging
        //printf("File name %s\n",name);
        FILE *f = fopen(name,"r");
        fscanf(f,"%d %d %d %ld",&*W,&*D,&*H,&*freq);
        //Debugging
        //printf("%d %d %d %ld\n",*W,*D,*H,*freq);
        char ***nodes = (char***)malloc(*W*sizeof(char**));
        for(int i = 0; i<*W; i++) {
                nodes[i] = (char**)malloc(*D*sizeof(char*));
                for(int j = 0; j<*D; j++) {
                        nodes[i][j] = (char*)malloc(*H*sizeof(char));
                        for(int k = 0; k<*H; k++) {
                                fscanf(f,"%c",&nodes[i][j][k]);
                        }
                }
        }
        return nodes;
}

int main(int argc, char **argv) {
        if(argc != 5) {
                printf("Check the number of arguments, must be 5!\n");
                return 0;
        }
        int W,D,H;
        int X,Y,Z;
        int block_number;
        long freq;

        //Read Room
        printf("Reading room file\n");
        char ***nodes = readFile(argv[1],&W,&D,&H,&freq);
        //Convert chars to int
        X = atoi(argv[2]);
        Y = atoi(argv[3]);
        Z = atoi(argv[4]);
        block_number = X*Y*Z;

        //Debugging
        //printf("%d\n", W*D*H );
        //printf("%d\n",block_number);

        //Create a folder
        struct stat st = {0};
        printf("Checking if Partitions folder exists\n" );
        if (stat("Partitions", &st) == -1) {
                printf("Creating Partitions folder\n");
                mkdir("Partitions", 0700);
        }
        printf("Starting partitioning the room\n" );
        //Partition the room
        for (int i=0; i < block_number; i++) {

                //Find index
                int x = (int) round((float)i / Y* Z);
                int y = (int) round((float) (i % Y*Z) / Z);
                int z = (i % Y*Z) % Z;
                int index = x*Y*Z + y*Z + z;
                int low = BLOCK_LOW(i,block_number,W*D*H);
                int high = BLOCK_HIGH(i,block_number,W*D*H);

                //Debugging
                //printf("Index %d\n", index);
                //printf("Low = %d High = %d\n",low,high );

                //Create file for partition index
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "Partitions/room_%d.dwm", index);
                FILE *f = fopen(buffer,"w");
                for(int q = 0; q<W; q++) {
                        for(int j = 0; j<D; j++) {
                                for(int k = 0; k<H; k++) {
                                        //Write to desired file
                                        int block_id = q + W*j + W*D*k;
                                        if(  block_id >= low && block_id<high) {
                                                fprintf(f, "%c", nodes[q][j][k]);
                                        }
                                }
                        }
                }
        }
        printf("Finished creating room partitions\n");
        return 0;
}
