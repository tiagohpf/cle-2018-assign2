#include <unistd.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wav.h"
#include <math.h>
#include <stddef.h>

//Defines
#define MASTER 0
#define BLOCK_LOW(id, p, n)   ((id)*(n)/(p))
#define BLOCK_HIGH(id, p, n)  (BLOCK_LOW((id)+1, p, n)-1)
#define BLOCK_OWNER(index, p, n)  (((p)*((index)+1)-1)/(n))
#define BLOCK_SIZE(id, p, n)  (BLOCK_LOW((id)+1, p, n)-BLOCK_LOW(id, p, n))


typedef struct {
        char c;
        double p;
        double pup,pdown,pleft,pright,pforward,pback;
        double mup,mdown,mleft,mright,mforward,mback;
}MESH;

MESH*** readFile(char *name,int *W,int *D,int *H,long *freq){
        //Debugging
        //printf("File name %s\n",name);
        FILE *f = fopen(name,"r");
        fscanf(f,"%d %d %d %ld",&*W,&*D,&*H,&*freq);
        //Debugging
        //printf("%d %d %d %ld\n",*W,*D,*H,*freq);
        MESH ***nodes = (MESH***)malloc(*W*sizeof(MESH**));
        for(int i = 0; i<*W; i++) {
                nodes[i] = (MESH**)malloc(*D*sizeof(MESH*));
                for(int j = 0; j<*D; j++) {
                        nodes[i][j] = (MESH*)malloc(*H*sizeof(MESH));
                        for(int k = 0; k<*H; k++) {
                                if(i == 0 && j == 0 && k == 0) {
                                        char x;
                                        fscanf(f,"%c",&x);
                                }
                                fscanf(f,"%c",&nodes[i][j][k].c);
                                nodes[i][j][k].p = 0;
                                nodes[i][j][k].pup = 0;
                                nodes[i][j][k].pdown = 0;
                                nodes[i][j][k].pleft = 0;
                                nodes[i][j][k].pright = 0;
                                nodes[i][j][k].pforward = 0;
                                nodes[i][j][k].pback = 0;
                                nodes[i][j][k].mup = 0;
                                nodes[i][j][k].mdown = 0;
                                nodes[i][j][k].mleft = 0;
                                nodes[i][j][k].mright = 0;
                                nodes[i][j][k].mforward = 0;
                                nodes[i][j][k].mback = 0;
                        }
                }
        }
        printf("Finished Reading room file!\n" );
        return nodes;
}

double get_absortion_coefficient(char label) {
        double coefficient;
        switch(label) {
        case 'A':
                coefficient = 0;
                break;
        case 'B':
                coefficient = 0.1;
                break;
        case 'C':
                coefficient = 0.2;
                break;
        case 'D':
                coefficient = 0.3;
                break;
        case 'E':
                coefficient = 0.4;
                break;
        case 'F':
                coefficient = 0.5;
                break;
        case 'G':
                coefficient = 0.6;
                break;
        case 'H':
                coefficient = 0.7;
                break;
        case 'I':
                coefficient = 0.8;
                break;
        case 'J':
                coefficient = 0.9;
                break;
        case '1':
                coefficient = 0.91;
                break;
        case '2':
                coefficient = 0.92;
                break;
        case '3':
                coefficient = 0.93;
                break;
        case '4':
                coefficient = 0.94;
                break;
        case '5':
                coefficient = 0.95;
                break;
        case '6':
                coefficient = 0.96;
                break;
        case '7':
                coefficient = 0.97;
                break;
        case '8':
                coefficient = 0.98;
                break;
        case '9':
                coefficient = 0.99;
                break;
        case 'Z':
                coefficient = 1;
                break;
        }
        return coefficient;
}


int main(int argc, char **argv) {

        MPI_Init(&argc, &argv);

        //Vars
        int totalProcesses, process_id;
        int W,D,H;
        long freq;
        double coef;
        long num_samples,size_of_each_sample;
        int value = 0;
        int process_to_send = -1;
        int process_to_recieve = -1;
        MESH ***nodes, ***new_nodes;
        MESH *nodes_per_process;
        unsigned char buffer4[4];
        unsigned char buffer2[2];
        FILE *input,*output;
        struct HEADER header;
        char wavheader[100];
        //Read from terminal later
        int X=2,Y=2,Z=2;
        int cordx,cordy,cordz;
        int small_X,small_Y,small_Z;
        int block_number;
        int size;
        int count;
        MPI_Comm_size(MPI_COMM_WORLD, &totalProcesses);
        //Create Data type for structure MESH to send and recieve with MPI
        int nitems=14;
        int blocklengths[14] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1};
        MPI_Datatype types[14] = {MPI_BYTE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE};
        MPI_Aint offsets[14];
        offsets[0] =offsetof(MESH,c);
        offsets[1] =offsetof(MESH,p);
        offsets[2] =offsetof(MESH,pup);
        offsets[3] =offsetof(MESH,pdown);
        offsets[4] =offsetof(MESH,pleft);
        offsets[5] =offsetof(MESH,pright);
        offsets[6] =offsetof(MESH,pforward);
        offsets[7] =offsetof(MESH,pback);
        offsets[8] =offsetof(MESH,mup);
        offsets[9] =offsetof(MESH,mdown);
        offsets[10] =offsetof(MESH,mleft);
        offsets[11] =offsetof(MESH,mright);
        offsets[12] =offsetof(MESH,mforward);
        offsets[13] =offsetof(MESH,mback);
        MPI_Datatype MPI_MESH;
        MPI_Type_create_struct(nitems, blocklengths, offsets, types, &MPI_MESH);
        MPI_Type_commit(&MPI_MESH);

        MPI_Comm_rank(MPI_COMM_WORLD, &process_id);
        if(process_id == MASTER) {
                nodes= readFile(argv[1],&W,&D,&H,&freq);
                size = W*D*H;
                block_number = X*Y*Z;
                small_X = W / X;
                small_Y = D / Y;
                small_Z = H / Z;

                if(totalProcesses != block_number) {
                        printf("Number of processes and blocks do not match!\n" );
                        return -1;
                }
                MPI_Bcast (&freq, 1, MPI_LONG, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&size, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&X, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&Y, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&Z, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&small_X, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&small_Y, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&small_Z, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                cordx=0,cordy=0,cordz=0;
                int rcordx,rcordy,rcordz;
                for (int i=0; i < block_number; i++) {

                        printf("Id = %d ", i);
                        printf("Coords %d %d %d Id %d\n",cordx,cordy,cordz,cordx*Y*Z+cordy*Z+cordz );
                        int block_size = BLOCK_SIZE(i,block_number,size);

                        nodes_per_process = (MESH*)malloc((block_size+1) * sizeof(MESH));
                        count = 0;
                        for(int q = 0; q<W; q++) {
                                for(int j = 0; j<D; j++) {
                                        for(int k = 0; k<H; k++) {
                                                int block_id = q + W*j + W*D*k;
                                                //printf("%d\n",BLOCK_OWNER(block_id,block_number,size) );
                                                if(BLOCK_OWNER(block_id,block_number,size) == i) {
                                                        nodes_per_process[count] = nodes[q][j][k];
                                                        count++;
                                                }
                                        }
                                }
                        }
                        // for (int q = 0; q <block_size; q++) {
                        //         if(nodes_per_process[q].c == 'S') {
                        //                 printf("YES\n");
                        //         }
                        // }
                        // printf("\n");

                        //Set arrays for processing and distribute work
                        if(i == MASTER) {
                                rcordx = cordx;
                                rcordy = cordy;
                                rcordz = cordz;
                                new_nodes = (MESH***)malloc(small_X*sizeof(MESH**));
                                int value = 0;
                                for(int i = 0; i<small_X; i++) {
                                        new_nodes[i] = (MESH**)malloc(small_Y*sizeof(MESH*));
                                        for(int j = 0; j<small_Y; j++) {
                                                new_nodes[i][j] = (MESH*)malloc(small_Z*sizeof(MESH));
                                                for(int k = 0; k<small_Z; k++) {
                                                        new_nodes[i][j][k] = nodes_per_process[value];
                                                        value++;
                                                }
                                        }
                                }
                        }
                        else{
                                MPI_Send(&count, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                                MPI_Send(nodes_per_process, count, MPI_MESH, i, 0, MPI_COMM_WORLD);
                                MPI_Send(&cordx,1,MPI_INT,i,0,MPI_COMM_WORLD);
                                MPI_Send(&cordy,1,MPI_INT,i,0,MPI_COMM_WORLD);
                                MPI_Send(&cordz,1,MPI_INT,i,0,MPI_COMM_WORLD);
                                //Send to other processes
                        }
                        if(cordz == Z-1) {
                                cordz = 0;
                                if(cordy == Y-1) {
                                        cordy = 0;
                                        cordx++;
                                }
                                else{
                                        cordy++;
                                }
                        }
                        else{
                                cordz++;
                        }
                }

                //Do My work
                //Each process does it's work, this bit is equal to the above on master since all processes do the same thing
                printf("Id %d - %d %d %d\n",process_id,rcordx,rcordy,rcordz);
                //Every process reads header
                input = fopen(argv[2],"rb");
                //Read wav file and store header in a string.
                fread(header.riff, sizeof(header.riff), 1, input);
                strcpy(wavheader, (char*)header.riff);
                fread(buffer4, sizeof(buffer4), 1, input);
                strcat(wavheader,(char*)buffer4);
                header.overall_size  = buffer4[0] |
                                       (buffer4[1]<<8) |
                                       (buffer4[2]<<16) |
                                       (buffer4[3]<<24);
                fread(header.wave, sizeof(header.wave), 1, input);
                strcat(wavheader,(char*)header.wave);
                fread(header.fmt_chunk_marker, sizeof(header.fmt_chunk_marker), 1, input);
                strcat(wavheader,(char*)header.fmt_chunk_marker);
                fread(buffer4, sizeof(buffer4), 1, input);
                strcat(wavheader,(char*)buffer4);
                header.length_of_fmt = buffer4[0] |
                                       (buffer4[1] << 8) |
                                       (buffer4[2] << 16) |
                                       (buffer4[3] << 24);

                fread(buffer2, sizeof(buffer2), 1, input);
                strcat(wavheader,(char*)buffer2);
                header.format_type = buffer2[0] | (buffer2[1] << 8);
                fread(buffer2, sizeof(buffer2), 1, input);
                strcat(wavheader,(char*)buffer2);
                header.channels = buffer2[0] | (buffer2[1] << 8);
                fread(buffer4, sizeof(buffer4), 1, input);
                strcat(wavheader,(char*)buffer4);
                header.sample_rate = buffer4[0] |
                                     (buffer4[1] << 8) |
                                     (buffer4[2] << 16) |
                                     (buffer4[3] << 24);
                if(header.sample_rate != freq) {
                        printf("Frequencies don't match!\n" );
                        return -1;
                }
                fread(buffer4, sizeof(buffer4), 1, input);
                strcat(wavheader,(char*)buffer4);
                header.byterate  = buffer4[0] |
                                   (buffer4[1] << 8) |
                                   (buffer4[2] << 16) |
                                   (buffer4[3] << 24);
                fread(buffer2, sizeof(buffer2), 1, input);
                strcat(wavheader,(char*)buffer2);
                header.block_align = buffer2[0] |
                                     (buffer2[1] << 8);
                fread(buffer2, sizeof(buffer2), 1, input);
                strcat(wavheader,(char*)buffer2);
                header.bits_per_sample = buffer2[0] |
                                         (buffer2[1] << 8);
                fread(header.data_chunk_header, sizeof(header.data_chunk_header), 1, input);
                strcat(wavheader,(char*)header.data_chunk_header);

                fread(buffer4, sizeof(buffer4), 1, input);
                strcat(wavheader,(char*)buffer4);
                header.data_size = buffer4[0] |
                                   (buffer4[1] << 8) |
                                   (buffer4[2] << 16) |
                                   (buffer4[3] << 24 );

                num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
                size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
                char data_buffer[size_of_each_sample];

                for(int x = 0; x<small_X; x++) {
                        for(int y = 0; y<small_Y; y++) {
                                for(int z = 0; z<small_Z; z++) {
                                        // if(new_nodes[x][y][z].c == 'S') {
                                        //         //Mudar para o R
                                        //         output = fopen("output.wav","wb");
                                        //         fprintf(output, "%s",wavheader );
                                        // }
                                        if(new_nodes[x][y][z].c == 'R') {
                                                //Mudar para o R
                                                output = fopen("output.wav","wb");
                                                fprintf(output, "%s",wavheader );
                                        }
                                }
                        }
                }
                //Wait for other processes
                MPI_Barrier(MPI_COMM_WORLD);
                for (int i =1; i <= num_samples; i++) {
                        printf("Sample %d\n",i );
                        fread(data_buffer, sizeof(data_buffer), 1, input);
                        // dump the data read
                        int data_in_channel = 0;
                        data_in_channel = data_buffer[0];
                        for(int x = 0; x<small_X; x++) {
                                for(int y = 0; y<small_Y; y++) {
                                        for(int z = 0; z<small_Z; z++) {
                                                if(new_nodes[x][y][z].c == 'S') {
                                                        //fprintf(output, "%s",data_buffer );
                                                        //Inject sound
                                                        new_nodes[x][y][z].pup = (data_in_channel / 2);

                                                        new_nodes[x][y][z].pdown =  (data_in_channel / 2);

                                                        new_nodes[x][y][z].pleft = (data_in_channel / 2);

                                                        new_nodes[x][y][z].pright = (data_in_channel / 2);

                                                        new_nodes[x][y][z].pforward = (data_in_channel / 2);

                                                        new_nodes[x][y][z].pback = (data_in_channel / 2);
                                                }
                                        }
                                }
                        }
                        //Wait for other processes
                        MPI_Barrier(MPI_COMM_WORLD);
                        //scattering
                        for(int x = 0; x<small_X; x++) {
                                for(int y = 0; y<small_Y; y++) {
                                        for(int z = 0; z<small_Z; z++) {
                                                if(new_nodes[x][y][z].c == 'S' || new_nodes[x][y][z].c == ' ') {
                                                        new_nodes[x][y][z].p = (new_nodes[x][y][z].pup + new_nodes[x][y][z].pdown + new_nodes[x][y][z].pforward + new_nodes[x][y][z].pback + new_nodes[x][y][z].pleft + new_nodes[x][y][z].pright) / 3;
                                                        new_nodes[x][y][z].mup = new_nodes[x][y][z].p -new_nodes[x][y][z].pup;
                                                        new_nodes[x][y][z].mdown = new_nodes[x][y][z].p -new_nodes[x][y][z].pdown;
                                                        new_nodes[x][y][z].mleft = new_nodes[x][y][z].p - new_nodes[x][y][z].pleft;
                                                        new_nodes[x][y][z].mright =new_nodes[x][y][z].p - new_nodes[x][y][z].pright;
                                                        new_nodes[x][y][z].mforward = new_nodes[x][y][z].p - new_nodes[x][y][z].pforward;
                                                        new_nodes[x][y][z].mback = new_nodes[x][y][z].p - new_nodes[x][y][z].pback;
                                                        //Mudar para o R
                                                        // if(new_nodes[x][y][z].c == 'S') {
                                                        //         data_buffer[0] = new_nodes[x][y][z].p;
                                                        //         fprintf(output, "%s",data_buffer);
                                                        // }

                                                }
                                                if(new_nodes[x][y][z].c == 'R') {
                                                        new_nodes[x][y][z].p = (new_nodes[x][y][z].pup + new_nodes[x][y][z].pdown + new_nodes[x][y][z].pforward + new_nodes[x][y][z].pback + new_nodes[x][y][z].pleft + new_nodes[x][y][z].pright) / 3;
                                                        new_nodes[x][y][z].mup = new_nodes[x][y][z].p -new_nodes[x][y][z].pup;
                                                        new_nodes[x][y][z].mdown = new_nodes[x][y][z].p -new_nodes[x][y][z].pdown;
                                                        new_nodes[x][y][z].mleft = new_nodes[x][y][z].p - new_nodes[x][y][z].pleft;
                                                        new_nodes[x][y][z].mright =new_nodes[x][y][z].p - new_nodes[x][y][z].pright;
                                                        new_nodes[x][y][z].mforward = new_nodes[x][y][z].p - new_nodes[x][y][z].pforward;
                                                        new_nodes[x][y][z].mback = new_nodes[x][y][z].p - new_nodes[x][y][z].pback;
                                                        // data_buffer[0] = new_nodes[x][y][z].p;
                                                        // fprintf(output, "%s",data_buffer);

                                                }
                                                else{
                                                        coef = get_absortion_coefficient(new_nodes[x][y][z].c);
                                                        new_nodes[x][y][z].mup = coef * new_nodes[x][y][z].pup;
                                                        new_nodes[x][y][z].mdown = coef * new_nodes[x][y][z].pdown;
                                                        new_nodes[x][y][z].mleft = coef * new_nodes[x][y][z].pleft;
                                                        new_nodes[x][y][z].mright = coef * new_nodes[x][y][z].pright;
                                                        new_nodes[x][y][z].mforward = coef * new_nodes[x][y][z].pforward;
                                                        new_nodes[x][y][z].mback = coef * new_nodes[x][y][z].pback;
                                                }
                                        }
                                }
                        }
                        //wait for other processes
                        MPI_Barrier(MPI_COMM_WORLD);

                        //delay 0
                        for(int x = 0; x<small_X; x++) {
                                for(int y = 0; y<small_Y; y++) {
                                        for(int z = 0; z<small_Z; z++) {
                                                //Normal delay
                                                if(x - 1>= 0) {
                                                        new_nodes[x-1][y][z].pright = new_nodes[x][y][z].mleft;
                                                }
                                                if(x + 1 < small_X) {
                                                        new_nodes[x+1][y][z].pleft = new_nodes[x][y][z].mright;
                                                }
                                                if(y - 1>= 0) {
                                                        new_nodes[x][y-1][z].pdown = new_nodes[x][y][z].mup;
                                                }

                                                if(y + 1 < small_Y) {
                                                        new_nodes[x][y+1][z].pup = new_nodes[x][y][z].mdown;
                                                }
                                                if(z - 1>= 0) {
                                                        new_nodes[x][y][z-1].pforward =   new_nodes[x][y][z].mback;
                                                }

                                                if(z + 1 < small_Z) {
                                                        new_nodes[x][y][z+1].pback = new_nodes[x][y][z].mforward;
                                                }
                                                //wait for other processes
                                                MPI_Barrier(MPI_COMM_WORLD);
                                                //Sending MPI, neste tem de ser rcord's que são as coordenadas reais do master
                                                //Sending to left
                                                if(x - 1 < 0) {
                                                        process_to_send =(rcordx-1) * Y*Z + rcordy*Z + rcordz;
                                                        if(process_to_send>= 0 && process_to_send <totalProcesses) {
                                                                //MPI SEND
                                                                MPI_Send(&new_nodes[x][y][z].mleft,1,MPI_DOUBLE,process_to_send,0,MPI_COMM_WORLD);
                                                        }
                                                }
                                                //Sending to right
                                                /*if(x + 1 == small_X) {
                                                        process_to_send =(rcordx+1) * Y*Z + rcordy*Z + rcordz;
                                                        if(process_to_send>= 0 && process_to_send <totalProcesses) {
                                                                //MPI SEND
                                                                MPI_Send(&new_nodes[x][y][z].mright,1,MPI_DOUBLE,process_to_send,0,MPI_COMM_WORLD);
                                                        }
                                                   }*/
                                                /*if(y - 1< 0) {
                                                        process_to_send =rcordx * Y*Z + (rcordy-1)*Z + rcordz;
                                                        if(process_to_send>= 0 && process_to_send <totalProcesses) {
                                                                //MPI SEND
                                                                MPI_Send(&new_nodes[x][y][z].mup,1,MPI_DOUBLE,process_to_send,0,MPI_COMM_WORLD);
                                                        }
                                                   }

                                                   if(y + 1 == small_Y) {
                                                        process_to_send =rcordx * Y*Z + (rcordy-1)*Z + rcordz;
                                                        if(process_to_send>= 0 && process_to_send <totalProcesses) {
                                                                //MPI SEND
                                                                MPI_Send(&new_nodes[x][y][z].mdown,1,MPI_DOUBLE,process_to_send,0,MPI_COMM_WORLD);
                                                        }

                                                   }
                                                   if(z - 1< 0) {
                                                        process_to_send =rcordx * Y*Z + rcordy*Z + rcordz-1;
                                                        if(process_to_send>= 0 && process_to_send <totalProcesses) {
                                                                //MPI SEND
                                                                MPI_Send(&new_nodes[x][y][z].mback,1,MPI_DOUBLE,process_to_send,0,MPI_COMM_WORLD);
                                                        }
                                                   }

                                                   if(z + 1 == small_Z) {
                                                        process_to_send =rcordx * Y*Z + rcordy*Z + rcordz+1;
                                                        if(process_to_send>= 0 && process_to_send <totalProcesses) {
                                                                //MPI SEND
                                                                MPI_Send(&new_nodes[x][y][z].mforward,1,MPI_DOUBLE,process_to_send,0,MPI_COMM_WORLD);
                                                        }
                                                   }*/

                                                //Recieving MPI
                                                //Recieving from right
                                                if(x + 1 == small_X) {
                                                        process_to_recieve =(rcordx+1) * Y*Z + rcordy*Z + rcordz;
                                                        if(process_to_recieve>= 0 && process_to_recieve <totalProcesses) {
                                                                //MPI Recieve
                                                                MPI_Recv(&new_nodes[x][y][z].pright,1,MPI_DOUBLE,process_to_recieve,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                                                        }
                                                }
                                                //Recieving from left
                                                /*if(x - 1 < 0) {
                                                        process_to_recieve =(rcordx-1) * Y*Z + rcordy*Z + rcordz;
                                                        if(process_to_recieve>= 0 && process_to_recieve <totalProcesses) {
                                                                //MPI Recieve
                                                                MPI_Recv(&new_nodes[x][y][z].pleft,1,MPI_DOUBLE,process_to_recieve,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                                                        }
                                                   }*/

                                                /*if(y + 1 == small_Y) {
                                                        process_to_recieve =rcordx * Y*Z + (rcordy+1)*Z + rcordz;
                                                        if(process_to_recieve>= 0 && process_to_recieve <totalProcesses) {
                                                                //MPI Recieve
                                                                MPI_Recv(&new_nodes[x][y][z].pup,1,MPI_DOUBLE,process_to_recieve,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                                                        }
                                                   }
                                                   if(y - 1< 0) {
                                                        process_to_recieve =rcordx * Y*Z + (rcordy-1)*Z + rcordz;
                                                        if(process_to_recieve>= 0 && process_to_recieve <totalProcesses) {
                                                                //MPI Recieve
                                                                MPI_Recv(&new_nodes[x][y][z].pdown,1,MPI_DOUBLE,process_to_recieve,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                                                        }
                                                   }
                                                   if(z + 1 == small_Z) {
                                                        process_to_recieve =rcordx * Y*Z + rcordy*Z + rcordz+1;
                                                        if(process_to_recieve>= 0 && process_to_recieve <totalProcesses) {
                                                                //MPI Recieve
                                                                MPI_Recv(&new_nodes[x][y][z].pforward,1,MPI_DOUBLE,process_to_recieve,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                                                        }
                                                   }
                                                   if(z - 1< 0) {
                                                        process_to_recieve =rcordx * Y*Z + rcordy*Z + rcordz-1;
                                                        if(process_to_recieve>= 0 && process_to_recieve <totalProcesses) {
                                                                //MPI Recieve
                                                                MPI_Recv(&new_nodes[x][y][z].pback,1,MPI_DOUBLE,process_to_recieve,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                                                        }
                                                   }*/

                                        }
                                }
                        }
                        //wait for other processes
                        MPI_Barrier(MPI_COMM_WORLD);

                }
        }
        else{
                MPI_Bcast (&freq, 1, MPI_LONG, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&size, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&X, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&Y, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&Z, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&small_X, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&small_Y, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&small_Z, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                MPI_Recv(&count, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                nodes_per_process = (MESH*)malloc(count * sizeof(MESH));
                MPI_Recv(nodes_per_process, count, MPI_MESH, MASTER, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                MPI_Recv(&cordx,1,MPI_INT,MASTER,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                MPI_Recv(&cordy,1,MPI_INT,MASTER,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                MPI_Recv(&cordz,1,MPI_INT,MASTER,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                new_nodes = (MESH***)malloc(small_X*sizeof(MESH**));
                printf("Id %d - %d %d %d\n",process_id,cordx,cordy,cordz);
                for(int i = 0; i<small_X; i++) {
                        new_nodes[i] = (MESH**)malloc(small_Y*sizeof(MESH*));
                        for(int j = 0; j<small_Y; j++) {
                                new_nodes[i][j] = (MESH*)malloc(small_Z*sizeof(MESH));
                                for(int k = 0; k<small_Z; k++) {
                                        new_nodes[i][j][k] = nodes_per_process[value];
                                        value++;
                                }
                        }
                }


                //Each process does it's work, this bit is equal to the above on master since all processes do the same thing

                //Every process reads header
                input = fopen(argv[2],"rb");
                //Read wav file and store header in a string.
                fread(header.riff, sizeof(header.riff), 1, input);
                strcpy(wavheader, (char*)header.riff);
                fread(buffer4, sizeof(buffer4), 1, input);
                strcat(wavheader,(char*)buffer4);
                header.overall_size  = buffer4[0] |
                                       (buffer4[1]<<8) |
                                       (buffer4[2]<<16) |
                                       (buffer4[3]<<24);
                fread(header.wave, sizeof(header.wave), 1, input);
                strcat(wavheader,(char*)header.wave);
                fread(header.fmt_chunk_marker, sizeof(header.fmt_chunk_marker), 1, input);
                strcat(wavheader,(char*)header.fmt_chunk_marker);
                fread(buffer4, sizeof(buffer4), 1, input);
                strcat(wavheader,(char*)buffer4);
                header.length_of_fmt = buffer4[0] |
                                       (buffer4[1] << 8) |
                                       (buffer4[2] << 16) |
                                       (buffer4[3] << 24);

                fread(buffer2, sizeof(buffer2), 1, input);
                strcat(wavheader,(char*)buffer2);
                header.format_type = buffer2[0] | (buffer2[1] << 8);
                fread(buffer2, sizeof(buffer2), 1, input);
                strcat(wavheader,(char*)buffer2);
                header.channels = buffer2[0] | (buffer2[1] << 8);
                fread(buffer4, sizeof(buffer4), 1, input);
                strcat(wavheader,(char*)buffer4);
                header.sample_rate = buffer4[0] |
                                     (buffer4[1] << 8) |
                                     (buffer4[2] << 16) |
                                     (buffer4[3] << 24);
                if(header.sample_rate != freq) {
                        printf("Frequencies don't match!\n" );
                        return -1;
                }
                fread(buffer4, sizeof(buffer4), 1, input);
                strcat(wavheader,(char*)buffer4);
                header.byterate  = buffer4[0] |
                                   (buffer4[1] << 8) |
                                   (buffer4[2] << 16) |
                                   (buffer4[3] << 24);
                fread(buffer2, sizeof(buffer2), 1, input);
                strcat(wavheader,(char*)buffer2);
                header.block_align = buffer2[0] |
                                     (buffer2[1] << 8);
                fread(buffer2, sizeof(buffer2), 1, input);
                strcat(wavheader,(char*)buffer2);
                header.bits_per_sample = buffer2[0] |
                                         (buffer2[1] << 8);
                fread(header.data_chunk_header, sizeof(header.data_chunk_header), 1, input);
                strcat(wavheader,(char*)header.data_chunk_header);

                fread(buffer4, sizeof(buffer4), 1, input);
                strcat(wavheader,(char*)buffer4);
                header.data_size = buffer4[0] |
                                   (buffer4[1] << 8) |
                                   (buffer4[2] << 16) |
                                   (buffer4[3] << 24 );

                num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
                size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
                char data_buffer[size_of_each_sample];

                for(int x = 0; x<small_X; x++) {
                        for(int y = 0; y<small_Y; y++) {
                                for(int z = 0; z<small_Z; z++) {
                                        // if(new_nodes[x][y][z].c == 'S') {
                                        //         //Mudar para o R
                                        //         output = fopen("output.wav","wb");
                                        //         fprintf(output, "%s",wavheader );
                                        // }
                                        if(new_nodes[x][y][z].c == 'R') {
                                                //Mudar para o R
                                                output = fopen("output.wav","wb");
                                                fprintf(output, "%s",wavheader );
                                        }
                                }
                        }
                }
                //Wait for other processes
                MPI_Barrier(MPI_COMM_WORLD);
                for (int i =1; i <= num_samples; i++) {
                        fread(data_buffer, sizeof(data_buffer), 1, input);
                        // dump the data read
                        int data_in_channel = 0;
                        data_in_channel = data_buffer[0];
                        for(int x = 0; x<small_X; x++) {
                                for(int y = 0; y<small_Y; y++) {
                                        for(int z = 0; z<small_Z; z++) {
                                                if(new_nodes[x][y][z].c == 'S') {
                                                        //fprintf(output, "%s",data_buffer );
                                                        //Inject sound
                                                        new_nodes[x][y][z].pup = (data_in_channel / 2);

                                                        new_nodes[x][y][z].pdown =  (data_in_channel / 2);

                                                        new_nodes[x][y][z].pleft = (data_in_channel / 2);

                                                        new_nodes[x][y][z].pright = (data_in_channel / 2);

                                                        new_nodes[x][y][z].pforward = (data_in_channel / 2);

                                                        new_nodes[x][y][z].pback = (data_in_channel / 2);
                                                }
                                        }
                                }
                        }
                        //Wait for other processes
                        MPI_Barrier(MPI_COMM_WORLD);
                        //scattering
                        for(int x = 0; x<small_X; x++) {
                                for(int y = 0; y<small_Y; y++) {
                                        for(int z = 0; z<small_Z; z++) {
                                                if(new_nodes[x][y][z].c == 'S' || new_nodes[x][y][z].c == ' ') {
                                                        new_nodes[x][y][z].p = (new_nodes[x][y][z].pup + new_nodes[x][y][z].pdown + new_nodes[x][y][z].pforward + new_nodes[x][y][z].pback + new_nodes[x][y][z].pleft + new_nodes[x][y][z].pright) / 3;
                                                        new_nodes[x][y][z].mup = new_nodes[x][y][z].p -new_nodes[x][y][z].pup;
                                                        new_nodes[x][y][z].mdown = new_nodes[x][y][z].p -new_nodes[x][y][z].pdown;
                                                        new_nodes[x][y][z].mleft = new_nodes[x][y][z].p - new_nodes[x][y][z].pleft;
                                                        new_nodes[x][y][z].mright =new_nodes[x][y][z].p - new_nodes[x][y][z].pright;
                                                        new_nodes[x][y][z].mforward = new_nodes[x][y][z].p - new_nodes[x][y][z].pforward;
                                                        new_nodes[x][y][z].mback = new_nodes[x][y][z].p - new_nodes[x][y][z].pback;
                                                        //Mudar para o R
                                                        // if(new_nodes[x][y][z].c == 'S') {
                                                        //         data_buffer[0] = new_nodes[x][y][z].p;
                                                        //         fprintf(output, "%s",data_buffer);
                                                        // }

                                                }
                                                if(new_nodes[x][y][z].c == 'R') {
                                                        new_nodes[x][y][z].p = (new_nodes[x][y][z].pup + new_nodes[x][y][z].pdown + new_nodes[x][y][z].pforward + new_nodes[x][y][z].pback + new_nodes[x][y][z].pleft + new_nodes[x][y][z].pright) / 3;
                                                        new_nodes[x][y][z].mup = new_nodes[x][y][z].p -new_nodes[x][y][z].pup;
                                                        new_nodes[x][y][z].mdown = new_nodes[x][y][z].p -new_nodes[x][y][z].pdown;
                                                        new_nodes[x][y][z].mleft = new_nodes[x][y][z].p - new_nodes[x][y][z].pleft;
                                                        new_nodes[x][y][z].mright =new_nodes[x][y][z].p - new_nodes[x][y][z].pright;
                                                        new_nodes[x][y][z].mforward = new_nodes[x][y][z].p - new_nodes[x][y][z].pforward;
                                                        new_nodes[x][y][z].mback = new_nodes[x][y][z].p - new_nodes[x][y][z].pback;
                                                        // data_buffer[0] = new_nodes[x][y][z].p;
                                                        // fprintf(output, "%s",data_buffer);
                                                }
                                                else{
                                                        coef = get_absortion_coefficient(new_nodes[x][y][z].c);
                                                        new_nodes[x][y][z].mup = coef * new_nodes[x][y][z].pup;
                                                        new_nodes[x][y][z].mdown = coef * new_nodes[x][y][z].pdown;
                                                        new_nodes[x][y][z].mleft = coef * new_nodes[x][y][z].pleft;
                                                        new_nodes[x][y][z].mright = coef * new_nodes[x][y][z].pright;
                                                        new_nodes[x][y][z].mforward = coef * new_nodes[x][y][z].pforward;
                                                        new_nodes[x][y][z].mback = coef * new_nodes[x][y][z].pback;
                                                }
                                        }
                                }
                        }
                        //wait for other processes
                        MPI_Barrier(MPI_COMM_WORLD);
                        //delay
                        for(int x = 0; x<small_X; x++) {
                                for(int y = 0; y<small_Y; y++) {
                                        for(int z = 0; z<small_Z; z++) {
                                                //Normal delay
                                                if(x - 1>= 0) {
                                                        new_nodes[x-1][y][z].pright = new_nodes[x][y][z].mleft;
                                                }
                                                if(x + 1 < small_X) {
                                                        new_nodes[x+1][y][z].pleft = new_nodes[x][y][z].mright;
                                                }
                                                if(y - 1>= 0) {
                                                        new_nodes[x][y-1][z].pdown = new_nodes[x][y][z].mup;
                                                }

                                                if(y + 1 < small_Y) {
                                                        new_nodes[x][y+1][z].pup = new_nodes[x][y][z].mdown;
                                                }
                                                if(z - 1>= 0) {
                                                        new_nodes[x][y][z-1].pforward =   new_nodes[x][y][z].mback;
                                                }

                                                if(z + 1 < small_Z) {
                                                        new_nodes[x][y][z+1].pback = new_nodes[x][y][z].mforward;
                                                }
                                                //wait for other processes
                                                MPI_Barrier(MPI_COMM_WORLD);
                                                //Sending MPI
                                                //Sending to left
                                                if(x - 1 < 0) {
                                                        process_to_send =(cordx-1) * Y*Z + cordy*Z + cordz;
                                                        if(process_to_send>= 0 && process_to_send <totalProcesses) {
                                                                //MPI SEND
                                                                MPI_Send(&new_nodes[x][y][z].mleft,1,MPI_DOUBLE,process_to_send,0,MPI_COMM_WORLD);
                                                        }
                                                }
                                                //Sending to right
                                                /*if(x + 1 == small_X) {
                                                        process_to_send =(cordx+1) * Y*Z + cordy*Z + cordz;
                                                        if(process_to_send>= 0 && process_to_send <totalProcesses) {
                                                                //MPI SEND
                                                                MPI_Send(&new_nodes[x][y][z].mright,1,MPI_DOUBLE,process_to_send,0,MPI_COMM_WORLD);
                                                        }
                                                   }*/
                                                /*if(y - 1< 0) {
                                                        process_to_send =cordx * Y*Z + (cordy-1)*Z + cordz;
                                                        if(process_to_send>= 0 && process_to_send <totalProcesses) {
                                                                //MPI SEND
                                                                MPI_Send(&new_nodes[x][y][z].mup,1,MPI_DOUBLE,process_to_send,0,MPI_COMM_WORLD);
                                                        }
                                                   }

                                                   if(y + 1 == small_Y) {
                                                        process_to_send =cordx * Y*Z + (cordy-1)*Z + cordz;
                                                        if(process_to_send>= 0 && process_to_send <totalProcesses) {
                                                                //MPI SEND
                                                                MPI_Send(&new_nodes[x][y][z].mdown,1,MPI_DOUBLE,process_to_send,0,MPI_COMM_WORLD);
                                                        }

                                                   }
                                                   if(z - 1< 0) {
                                                        process_to_send =cordx * Y*Z + cordy*Z + cordz-1;
                                                        if(process_to_send>= 0 && process_to_send <totalProcesses) {
                                                                //MPI SEND
                                                                MPI_Send(&new_nodes[x][y][z].mback,1,MPI_DOUBLE,process_to_send,0,MPI_COMM_WORLD);
                                                        }
                                                   }

                                                   if(z + 1 == small_Z) {
                                                        process_to_send =cordx * Y*Z + cordy*Z + cordz+1;
                                                        if(process_to_send>= 0 && process_to_send <totalProcesses) {
                                                                //MPI SEND
                                                                MPI_Send(&new_nodes[x][y][z].mforward,1,MPI_DOUBLE,process_to_send,0,MPI_COMM_WORLD);
                                                        }
                                                   }*/

                                                //Recieving MPI
                                                //Recieving from right
                                                if(x + 1 == small_X) {
                                                        process_to_recieve =(cordx+1) * Y*Z + cordy*Z + cordz;
                                                        if(process_to_recieve>= 0 && process_to_recieve <totalProcesses) {
                                                                //MPI Recieve
                                                                MPI_Recv(&new_nodes[x][y][z].pright,1,MPI_DOUBLE,process_to_recieve,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                                                        }
                                                }
                                                //Recieving from left
                                                /*if(x - 1 < 0) {
                                                        process_to_recieve =(cordx-1) * Y*Z + cordy*Z + cordz;
                                                        if(process_to_recieve>= 0 && process_to_recieve <totalProcesses) {
                                                                //MPI Recieve
                                                                MPI_Recv(&new_nodes[x][y][z].pleft,1,MPI_DOUBLE,process_to_recieve,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                                                        }
                                                   }*/

                                                /*if(y + 1 == small_Y) {
                                                        process_to_recieve =cordx * Y*Z + (cordy+1)*Z + cordz;
                                                        if(process_to_recieve>= 0 && process_to_recieve <totalProcesses) {
                                                                //MPI Recieve
                                                                MPI_Recv(&new_nodes[x][y][z].pup,1,MPI_DOUBLE,process_to_recieve,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                                                        }
                                                   }
                                                   if(y - 1< 0) {
                                                        process_to_recieve =cordx * Y*Z + (cordy-1)*Z + cordz;
                                                        if(process_to_recieve>= 0 && process_to_recieve <totalProcesses) {
                                                                //MPI Recieve
                                                                MPI_Recv(&new_nodes[x][y][z].pdown,1,MPI_DOUBLE,process_to_recieve,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                                                        }
                                                   }
                                                   if(z + 1 == small_Z) {
                                                        process_to_recieve =cordx * Y*Z + cordy*Z + cordz+1;
                                                        if(process_to_recieve>= 0 && process_to_recieve <totalProcesses) {
                                                              //MPI Recieve
                                                                MPI_Recv(&new_nodes[x][y][z].pforward,1,MPI_DOUBLE,process_to_recieve,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                                                        }
                                                   }
                                                   if(z - 1< 0) {
                                                        process_to_recieve =cordx * Y*Z + cordy*Z + cordz-1;
                                                        if(process_to_recieve>= 0 && process_to_recieve <totalProcesses) {
                                                              //MPI Recieve
                                                                MPI_Recv(&new_nodes[x][y][z].pback,1,MPI_DOUBLE,process_to_recieve,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                                                        }
                                                   }*/

                                        }
                                }
                        }
                        //wait for other processes
                        MPI_Barrier(MPI_COMM_WORLD);

                }
        }
        MPI_Type_free(&MPI_MESH);
        MPI_Finalize();
        return 0;
}
