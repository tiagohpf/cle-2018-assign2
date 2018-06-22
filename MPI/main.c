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
        MESH ***nodes, ***new_nodes;
        MESH *nodes_per_process;
        unsigned char buffer4[4];
        unsigned char buffer2[2];
        FILE *input,*output;
        struct HEADER header;
        char wavheader[100];
        //Read from terminal later
        int X=2,Y=2,Z=2;
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
        if(process_id == 0) {
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
                for (int i=0; i < block_number; i++) {
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
                                //Send to other processes
                        }

                        //Do my work
                        if(i == MASTER) {
                                for(int i = 0; i<small_X; i++) {
                                        for(int j = 0; j<small_Y; j++) {
                                                for(int k = 0; k<small_Z; k++) {
                                                        if(new_nodes[i][j][k].c == 'S')
                                                                printf("%c ",new_nodes[i][j][k].c );
                                                }
                                        }
                                }
                                for(int i = 0; i<small_X; i++) {
                                        for(int j = 0; j<small_Y; j++) {
                                                for(int k = 0; k<small_Z; k++) {
                                                        if(new_nodes[i][j][k].c == 'S') {
                                                                //printf("%c ",new_nodes[i][j][k].c );
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
                                                                output = fopen("output.wav","wb");
                                                        }
                                                        if(new_nodes[i][j][k].c == 'R') {

                                                        }
                                                }
                                        }
                                }
                                long num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
                                long size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
                                char data_buffer[size_of_each_sample];

                                for(int x = 0; x<small_X; x++) {
                                        for(int y = 0; y<small_Y; y++) {
                                                for(int z = 0; z<small_Z; z++) {
                                                        if(new_nodes[x][y][z].c == 'S') {
                                                                //abrir ficheiro if sample = 1  e encontrar R
                                                                fprintf(output, "%s", wavheader);
                                                                for (int i =1; i <= num_samples; i++) {
                                                                        fread(data_buffer, sizeof(data_buffer), 1, input);
                                                                        // dump the data read
                                                                        int data_in_channel = 0;
                                                                        data_in_channel = data_buffer[0];

                                                                        //scattering
                                                                        for(int bx = 0; bx<small_X; bx++ ) {
                                                                                for(int by = 0; by < small_Y; by++) {
                                                                                        for(int bz = 0; bz< small_Z; bz++) {
                                                                                                if(new_nodes[bx][by][bz].c == 'S') {

                                                                                                        new_nodes[bx][by][bz].pup = (data_in_channel / 2);

                                                                                                        new_nodes[bx][by][bz].pdown =  (data_in_channel / 2);

                                                                                                        new_nodes[bx][by][bz].pleft = (data_in_channel / 2);

                                                                                                        new_nodes[bx][by][bz].pright = (data_in_channel / 2);

                                                                                                        new_nodes[bx][by][bz].pforward = (data_in_channel / 2);

                                                                                                        new_nodes[bx][by][bz].pback = (data_in_channel / 2);

                                                                                                        new_nodes[bx][by][bz].p = (new_nodes[bx][by][bz].pup + new_nodes[bx][by][bz].pdown + new_nodes[bx][by][bz].pforward + new_nodes[bx][by][bz].pback + new_nodes[bx][by][bz].pleft + new_nodes[bx][by][bz].pright) / 3;

                                                                                                        new_nodes[bx][by][bz].mup = new_nodes[bx][by][bz].p -new_nodes[bx][by][bz].pup;

                                                                                                        new_nodes[bx][by][bz].mdown = new_nodes[bx][by][bz].p -new_nodes[bx][by][bz].pdown;

                                                                                                        new_nodes[bx][by][bz].mleft = new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pleft;

                                                                                                        new_nodes[bx][by][bz].mright =new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pright;

                                                                                                        new_nodes[bx][by][bz].mforward = new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pforward;

                                                                                                        new_nodes[bx][by][bz].mback = new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pback;

                                                                                                        data_buffer[0] = new_nodes[bx][by][bz].p;
                                                                                                        fprintf(output, "%s", data_buffer);
                                                                                                }
																								else if(new_nodes[bx][by][bz].c == ' ' || new_nodes[bx][by][bz].c == 'R'){
																									new_nodes[bx][by][bz].p = (new_nodes[bx][by][bz].pup + new_nodes[bx][by][bz].pdown + new_nodes[bx][by][bz].pforward + new_nodes[bx][by][bz].pback + new_nodes[bx][by][bz].pleft + new_nodes[bx][by][bz].pright) / 3;

																									new_nodes[bx][by][bz].mup = new_nodes[bx][by][bz].p -new_nodes[bx][by][bz].pup;

																									new_nodes[bx][by][bz].mdown = new_nodes[bx][by][bz].p -new_nodes[bx][by][bz].pdown;

																									new_nodes[bx][by][bz].mleft = new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pleft;

																									new_nodes[bx][by][bz].mright =new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pright;

																									new_nodes[bx][by][bz].mforward = new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pforward;

																									new_nodes[bx][by][bz].mback = new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pback;
																								}
																								else {
																									coef = get_absortion_coefficient(new_nodes[bx][by][bz].p);
																									new_nodes[bx][by][bz].mup = coef * new_nodes[bx][by][bz].pup;
																									new_nodes[bx][by][bz].mdown = coef * new_nodes[bx][by][bz].pdown;
																									new_nodes[bx][by][bz].mleft = coef * new_nodes[bx][by][bz].pleft;
																									new_nodes[bx][by][bz].mright = coef * new_nodes[bx][by][bz].pright;
																								}
																								

                                                                                        }
                                                                                }
                                                                        }


                                                                        //delay
                                                                        for(int bx = 0; bx<small_X; bx++ ) {
                                                                                for(int by = 0; by < small_Y; by++) {
                                                                                        for(int bz = 0; bz< small_Z; bz++) {
																								
																							if(bx - 1>= 0) {
																									new_nodes[bx-1][by][bz].pright = new_nodes[bx][by][bz].mleft;
																							}


																							if(bx + 1 < small_X) {
																									new_nodes[bx+1][by][bz].pleft = new_nodes[bx][by][bz].mright;
																							}


																							if(by - 1>= 0) {
																									new_nodes[bx][by-1][bz].pdown = new_nodes[bx][by][bz].mup;
																							}

																							if(by + 1 < small_Y) {
																									new_nodes[bx][by+1][bz].pup = new_nodes[bx][by][bz].mdown;
																							}


																							if(bz - 1>= 0) {
																									new_nodes[bx][by][bz-1].pforward =   new_nodes[bx][by][bz].mback;
																							}

																							if(bz + 1 < small_Z) {
																									new_nodes[bx][by][bz+1].pback = new_nodes[bx][by][bz].mforward;
																							}
                                                                                        }
                                                                                }
                                                                        }
                                                                        // for(int ii = 0; ii<small_X; ii++) {
                                                                        //         for(int j = 0; j<small_Y; j++) {
                                                                        //                 for(int k = 0; k<small_Z; k++) {
                                                                        //                         printf("%f ",new_nodes[ii][j][k].p );
                                                                        //                 }
                                                                        //                 printf("\n");
                                                                        //         }
                                                                        // }


                                                                }
                                                        }
                                                }
                                        }
                                }
                        }

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

                //Each process does it's work, this bit is equal to the above on master since all processes do the same thing
                for(int i = 0; i<small_X; i++) {
                        for(int j = 0; j<small_Y; j++) {
                                for(int k = 0; k<small_Z; k++) {
                                        if(new_nodes[i][j][k].c == 'S') {
                                                //printf("%c ",new_nodes[i][j][k].c );
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
                                                output = fopen("output.wav","wb");
                                        }
                                        if(new_nodes[i][j][k].c == 'R') {

                                        }
                                }
                        }
                }
                long num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
                long size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
                char data_buffer[size_of_each_sample];

                for(int x = 0; x<small_X; x++) {
                        for(int y = 0; y<small_Y; y++) {
                                for(int z = 0; z<small_Z; z++) {
                                        if(new_nodes[x][y][z].c == 'S') {
                                                //abrir ficheiro if sample = 1  e encontrar R
                                                fprintf(output, "%s", wavheader);
                                                for (int i =1; i <= num_samples; i++) {
                                                        fread(data_buffer, sizeof(data_buffer), 1, input);
                                                        // dump the data read
                                                        int data_in_channel = 0;
                                                        data_in_channel = data_buffer[0];

                                                        //scattering
                                                        for(int bx = 0; bx<small_X; bx++ ) {
                                                                for(int by = 0; by < small_Y; by++) {
                                                                        for(int bz = 0; bz< small_Z; bz++) {
                                                                                if(new_nodes[bx][by][bz].c == 'S') {

                                                                                        new_nodes[bx][by][bz].pup = (data_in_channel / 2);

                                                                                        new_nodes[bx][by][bz].pdown =  (data_in_channel / 2);

                                                                                        new_nodes[bx][by][bz].pleft = (data_in_channel / 2);

                                                                                        new_nodes[bx][by][bz].pright = (data_in_channel / 2);

                                                                                        new_nodes[bx][by][bz].pforward = (data_in_channel / 2);

                                                                                        new_nodes[bx][by][bz].pback = (data_in_channel / 2);

                                                                                        new_nodes[bx][by][bz].p = (new_nodes[bx][by][bz].pup + new_nodes[bx][by][bz].pdown + new_nodes[bx][by][bz].pforward + new_nodes[bx][by][bz].pback + new_nodes[bx][by][bz].pleft + new_nodes[bx][by][bz].pright) / 3;

                                                                                        new_nodes[bx][by][bz].mup = new_nodes[bx][by][bz].p -new_nodes[bx][by][bz].pup;

                                                                                        new_nodes[bx][by][bz].mdown = new_nodes[bx][by][bz].p -new_nodes[bx][by][bz].pdown;

                                                                                        new_nodes[bx][by][bz].mleft = new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pleft;

                                                                                        new_nodes[bx][by][bz].mright =new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pright;

                                                                                        new_nodes[bx][by][bz].mforward = new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pforward;

                                                                                        new_nodes[bx][by][bz].mback = new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pback;

                                                                                        data_buffer[0] = new_nodes[bx][by][bz].p;
                                                                                        fprintf(output, "%s", data_buffer);
                                                                                }
                                                                                else if(new_nodes[bx][by][bz].c == ' ' || new_nodes[bx][by][bz].c == 'R'){
                                                                                        new_nodes[bx][by][bz].p = (new_nodes[bx][by][bz].pup + new_nodes[bx][by][bz].pdown + new_nodes[bx][by][bz].pforward + new_nodes[bx][by][bz].pback + new_nodes[bx][by][bz].pleft + new_nodes[bx][by][bz].pright) / 3;

                                                                                        new_nodes[bx][by][bz].mup = new_nodes[bx][by][bz].p -new_nodes[bx][by][bz].pup;

                                                                                        new_nodes[bx][by][bz].mdown = new_nodes[bx][by][bz].p -new_nodes[bx][by][bz].pdown;

                                                                                        new_nodes[bx][by][bz].mleft = new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pleft;

                                                                                        new_nodes[bx][by][bz].mright =new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pright;

                                                                                        new_nodes[bx][by][bz].mforward = new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pforward;

                                                                                        new_nodes[bx][by][bz].mback = new_nodes[bx][by][bz].p - new_nodes[bx][by][bz].pback;
                                                                                }
																				else {
																					coef = get_absortion_coefficient(new_nodes[bx][by][bz].p);
																					new_nodes[bx][by][bz].mup = coef * new_nodes[bx][by][bz].pup;
																					new_nodes[bx][by][bz].mdown = coef * new_nodes[bx][by][bz].pdown;
																					new_nodes[bx][by][bz].mleft = coef * new_nodes[bx][by][bz].pleft;
																					new_nodes[bx][by][bz].mright = coef * new_nodes[bx][by][bz].pright;
																				}
                                                                        }
                                                                }
                                                        }

                                                        //delay
                                                        for(int bx = 0; bx<small_X; bx++ ) {
                                                                for(int by = 0; by < small_Y; by++) {
                                                                        for(int bz = 0; bz< small_Z; bz++) {                                                                             
																			if(bx - 1>= 0) {
																					new_nodes[bx-1][by][bz].pright = new_nodes[bx][by][bz].mleft;
																			}


																			if(bx + 1 < small_X) {
																					new_nodes[bx+1][by][bz].pleft = new_nodes[bx][by][bz].mright;
																			}


																			if(by - 1>= 0) {
																					new_nodes[bx][by-1][bz].pdown = new_nodes[bx][by][bz].mup;
																			}

																			if(by + 1 < small_Y) {
																					new_nodes[bx][by+1][bz].pup = new_nodes[bx][by][bz].mdown;
																			}


																			if(bz - 1>= 0) {
																					new_nodes[bx][by][bz-1].pforward =   new_nodes[bx][by][bz].mback;
																			}

																			if(bz + 1 < small_Z) {
																					new_nodes[bx][by][bz+1].pback = new_nodes[bx][by][bz].mforward;
																			}                                                                               
                                                                        }
                                                                }
                                                        }
                                                        // for(int ii = 0; ii<small_X; ii++) {
                                                        //         for(int j = 0; j<small_Y; j++) {
                                                        //                 for(int k = 0; k<small_Z; k++) {
                                                        //                         printf("%f ",new_nodes[ii][j][k].p );
                                                        //                 }
                                                        //                 printf("\n");
                                                        //         }
                                                        // }


                                                }
                                        }
                                }
                        }
                }


        }
        MPI_Type_free(&MPI_MESH);
        MPI_Finalize();
        return 0;
}
