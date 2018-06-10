#include <unistd.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wav.h"
#include <math.h>
#define MASTER 0

char* readFile(char *name,int *W,int *D,int *H,long *freq){
        //Debugging
        //printf("File name %s\n",name);
        FILE *f = fopen(name,"r");
        fscanf(f,"%d %d %d %ld",&*W,&*D,&*H,&*freq);
        //Debugging
        //printf("%d %d %d %ld\n",*W,*D,*H,*freq);
        char *nodes = (char*)malloc(*W * *D * *H *sizeof(char));
        for(int i = 0; i<*W; i++) {
                for(int j = 0; j<*D; j++) {
                        for(int k = 0; k<*H; k++) {
                                fscanf(f,"%c",&nodes[i + *W * (j+ *D *k)]);
                        }
                }
        }
        return nodes;
}
//Flat[x + WIDTH * (y + DEPTH * z)] = Original[x, y, z]
int main(int argc, char **argv) {

        MPI_Init(&argc, &argv);

        if(argc != 3) {
                printf("Not enough arguments!\n" );
                return -1;
        }
        //Declarations

        int totalProcesses, process_id;
        unsigned char buffer4[4];
        unsigned char buffer2[2];
        FILE *input,*output;
        struct HEADER header;
        char wavheader[100];
        char *nodes,*sub_nodes;
        float *buffer,*sub_buffer;
        int W,D,H;
        long freq;
        int elements_process;
        int size;

        MPI_Comm_size(MPI_COMM_WORLD, &totalProcesses);
        MPI_Comm_rank(MPI_COMM_WORLD, &process_id);

        if(process_id == MASTER) {


                //Read Room
                printf("Reading room file\n");
                nodes = readFile(argv[1],&W,&D,&H,&freq);
                buffer= (float*)malloc(W * D * H *sizeof(float));
                for(int i = 0; i<W; i++) {
                        for(int j = 0; j<D; j++) {
                                for(int k = 0; k<H; k++) {
                                        buffer[i + W * (j+ D *k)] = 0;
                                }
                        }
                }
                size = W*D*H;
                if(size % totalProcesses != 0 ) {
                        printf("Process number must be divisible by the room size!\n" );
                        return -1;
                }
                elements_process = size/totalProcesses;

                //send broadcast to slaves
                MPI_Bcast (&elements_process, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&size, 1, MPI_INT, MASTER, MPI_COMM_WORLD);

                input = fopen(argv[2],"rb");
                output = fopen("output.wav","wb");
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


                fprintf(output, "%s", (unsigned char *)wavheader);
                // calculate no.of samples
                long num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
                printf("Number of samples:%lu \n", num_samples);

                long size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
                printf("Size of each sample:%ld bytes\n", size_of_each_sample);

                long bytes_in_each_channel = (size_of_each_sample / header.channels);
                if ((bytes_in_each_channel  * header.channels) != size_of_each_sample) {
                        printf("Error: %ld x %ud <> %ld\n", bytes_in_each_channel, header.channels, size_of_each_sample);
                        return -1;
                }
                long low_limit = 0l;
                long high_limit = 0l;

                switch (header.bits_per_sample) {
                case 8:
                        low_limit = -128;
                        high_limit = 127;
                        break;
                case 16:
                        low_limit = -32768;
                        high_limit = 32767;
                        break;
                case 32:
                        low_limit = -2147483648;
                        high_limit = 2147483647;
                        break;
                }
                char data_buffer[size_of_each_sample];
                for (int x = 0; x<W; x++) {
                        for (int y = 0; y < D; y++) {
                                for (int z = 0; z < H; z++) {
                                        //Flat[x + WIDTH * (y + DEPTH * z)] = Original[x, y, z]
                                        if(nodes[x +W * (y+D*z)] == 'S') {
                                                //printf("%d\n",x +W * (y+D*z) );
                                                for (int i =1; i <= num_samples; i++) {
                                                        fread(data_buffer, sizeof(data_buffer), 1, input);

                                                        // dump the data read
                                                        unsigned int xchannels = 0;
                                                        int data_in_channel = 0;

                                                        for (xchannels = 0; xchannels < header.channels; xchannels++ ) {
                                                                // convert data from little endian to big endian based on bytes in each channel sample
                                                                if (bytes_in_each_channel == 4) {
                                                                        data_in_channel = data_buffer[0] |
                                                                                          (data_buffer[1]<<8) |
                                                                                          (data_buffer[2]<<16) |
                                                                                          (data_buffer[3]<<24);
                                                                }
                                                                else if (bytes_in_each_channel == 2) {
                                                                        data_in_channel = data_buffer[0] |
                                                                                          (data_buffer[1] << 8);
                                                                }
                                                                else if (bytes_in_each_channel == 1) {
                                                                        data_in_channel = data_buffer[0];
                                                                }
                                                                // check if value was in range
                                                                if (data_in_channel < low_limit || data_in_channel > high_limit)
                                                                        printf("**value out of range\n");


                                                                //Inject sound
                                                                float pup = 0,pdown = 0,pleft=0,pright=0,pforward=0,pback=0;
                                                                //x +W * (y+D*z)
                                                                if((x - 1)+W * (y+D*z) >= 0 && (x + 1)+W * (y+D*z)< size) {
                                                                        pup = buffer[(x - 1)+W * (y+D*z)];
                                                                        pdown = buffer[(x + 1)+W * (y+D*z)];

                                                                }
                                                                if(x+W * ((y-1)+D*z) >= 0 && x+W * ((y+1)+D*z) < size) {
                                                                        pleft = buffer[x+W * ((y-1)+D*z)];
                                                                        pright = buffer[x+W * ((y+1)+D*z)];

                                                                }
                                                                if(x+W * (y+D*(z-1)) >= 0 && x+W * (y+D*(z+1)) < size) {
                                                                        pforward = buffer[x+W * (y+D*(z-1))];
                                                                        pback = buffer[x+W * (y+D*(z+1))];
                                                                }

                                                                //float p = (pup + pdown + pleft + pright + pback + pforward) / 3;
                                                                float p = data_in_channel;
                                                                buffer[x +W * (y+D*z)]+=data_in_channel;
                                                                if((x - 1)+W * (y+D*z) >= 0 && (x + 1)+W * (y+D*z)< size) {
                                                                        buffer[(x - 1)+W * (y+D*z)] = p - pup;
                                                                        buffer[(x + 1)+W * (y+D*z)] = p-pdown;

                                                                }
                                                                if(x+W * ((y-1)+D*z) >= 0 && x+W * ((y+1)+D*z) < size) {
                                                                        buffer[x+W * ((y-1)+D*z)] = p-pleft;
                                                                        buffer[x+W * ((y+1)+D*z)] = p-pright;

                                                                }
                                                                if(x+W * (y+D*(z-1)) >= 0 && x+W * (y+D*(z+1)) < size) {
                                                                        buffer[x+W * (y+D*(z-1))] = p-pforward;
                                                                        buffer[x+W * (y+D*(z+1))] = p-pback;
                                                                }

                                                                //Divide data per processes
                                                                sub_buffer  = (float *) malloc(elements_process * sizeof(float));
                                                                sub_nodes = (char * )malloc(elements_process * sizeof(char));
                                                                MPI_Scatter(nodes, elements_process, MPI_BYTE, sub_nodes, elements_process, MPI_BYTE, 0, MPI_COMM_WORLD);
                                                                MPI_Scatter(buffer, elements_process, MPI_FLOAT, sub_buffer, elements_process, MPI_FLOAT, 0, MPI_COMM_WORLD);
                                                                //fprintf(output, "%s", data_buffer);
                                                        }
                                                }
                                        }
                                }
                        }
                }
                fclose(input);
                fclose(output);
        }
        else{
                //recieve broadcast from master
                MPI_Bcast (&elements_process, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                MPI_Bcast (&size, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
                sub_buffer  = (float *) malloc(elements_process * sizeof(float));
                sub_nodes = (char * )malloc(elements_process * sizeof(char));
                MPI_Scatter(nodes, elements_process, MPI_BYTE, sub_nodes, elements_process, MPI_BYTE, 0, MPI_COMM_WORLD);
                MPI_Scatter(buffer, elements_process, MPI_FLOAT, sub_buffer, elements_process, MPI_FLOAT, 0, MPI_COMM_WORLD);
                printf("Process %d %d %d\n",process_id,elements_process,size);
        }


        MPI_Finalize();
        return 0;
}
