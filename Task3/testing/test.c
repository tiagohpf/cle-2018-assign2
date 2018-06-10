/**
 * Read and parse a wave file
 *
 **/
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wav.h"
#define TRUE 1
#define FALSE 0

// WAVE header structure

unsigned char buffer4[4];
unsigned char buffer2[2];

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
        printf("Finished Reading room file!\n" );
        return nodes;
}



FILE *ptr;
char *filename;
struct HEADER header;

int main(int argc, char **argv) {
        if(argc != 3) {
                printf("Check the number of arguments, must be 5!\n");
                return 0;
        }

        int W,D,H;
        long freq;

        //Read Room
        printf("Reading room file\n");
        char ***nodes = readFile(argv[1],&W,&D,&H,&freq);
        float ***buffer = (float***)malloc(W*sizeof(float**));
        for(int i = 0; i<W; i++) {
                buffer[i] = (float**)malloc(D*sizeof(float*));
                for(int j = 0; j<D; j++) {
                        buffer[i][j] = (float*)malloc(H*sizeof(float));
                        for(int k = 0; k<H; k++) {
                                buffer[i][j][k] = 0;
                        }
                }
        }

        FILE *f = fopen("output.wav","w");
        filename = (char*) malloc(sizeof(char) * 1024);
        if (filename == NULL) {
                printf("Error in malloc\n");
                exit(1);
        }

        // get file path
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {

                strcpy(filename, cwd);

                // get filename from command line
                if (argc < 2) {
                        printf("No wave file specified\n");
                        return 0;
                }

                strcat(filename, "/");
                strcat(filename, argv[2]);
                printf("%s\n", filename);
        }

        // open file
        printf("Opening  file..\n");
        ptr = fopen(filename, "rb");
        if (ptr == NULL) {
                printf("Error opening file\n");
                exit(1);
        }

        int read = 0;

        // read header parts

        read = fread(header.riff, sizeof(header.riff), 1, ptr);
        printf("(1-4): %s \n", header.riff);
        fprintf(f, "%s", header.riff);

        read = fread(buffer4, sizeof(buffer4), 1, ptr);
        printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

        // convert little endian to bxg endian 4 byte int
        header.overall_size  = buffer4[0] |
                               (buffer4[1]<<8) |
                               (buffer4[2]<<16) |
                               (buffer4[3]<<24);

        printf("(5-8) Overall size: bytes:%u, Kb:%u \n", header.overall_size, header.overall_size/1024);
        fprintf(f, "%s", buffer4);
        read = fread(header.wave, sizeof(header.wave), 1, ptr);
        printf("(9-12) Wave marker: %s\n", header.wave);
        fprintf(f, "%s", header.wave);

        read = fread(header.fmt_chunk_marker, sizeof(header.fmt_chunk_marker), 1, ptr);
        printf("(13-16) Fmt marker: %s\n", header.fmt_chunk_marker);
        fprintf(f, "%s", header.fmt_chunk_marker);
        read = fread(buffer4, sizeof(buffer4), 1, ptr);
        printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

        // convert little endian to bxg endian 4 byte integer
        header.length_of_fmt = buffer4[0] |
                               (buffer4[1] << 8) |
                               (buffer4[2] << 16) |
                               (buffer4[3] << 24);
        printf("(17-20) Length of Fmt header: %u \n", header.length_of_fmt);
        fprintf(f, "%s", buffer4);
        read = fread(buffer2, sizeof(buffer2), 1, ptr); printf("%u %u \n", buffer2[0], buffer2[1]);

        header.format_type = buffer2[0] | (buffer2[1] << 8);
        char format_name[10] = "";
        if (header.format_type == 1)
                strcpy(format_name,"PCM");
        else if (header.format_type == 6)
                strcpy(format_name, "A-law");
        else if (header.format_type == 7)
                strcpy(format_name, "Mu-law");

        printf("(21-22) Format type: %u %s \n", header.format_type, format_name);
        fprintf(f, "%s", buffer2);
        read = fread(buffer2, sizeof(buffer2), 1, ptr);
        printf("%u %u \n", buffer2[0], buffer2[1]);

        header.channels = buffer2[0] | (buffer2[1] << 8);
        printf("(23-24) Channels: %u \n", header.channels);
        fprintf(f, "%s", buffer2);

        read = fread(buffer4, sizeof(buffer4), 1, ptr);
        printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

        header.sample_rate = buffer4[0] |
                             (buffer4[1] << 8) |
                             (buffer4[2] << 16) |
                             (buffer4[3] << 24);

        printf("(25-28) Sample rate: %u\n", header.sample_rate);
        fprintf(f, "%s", buffer4);
        if(header.sample_rate != freq) {
                printf("Frequencies don't match!\n" );
                return -1;
        }

        read = fread(buffer4, sizeof(buffer4), 1, ptr);
        printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

        header.byterate  = buffer4[0] |
                           (buffer4[1] << 8) |
                           (buffer4[2] << 16) |
                           (buffer4[3] << 24);
        printf("(29-32) Byte Rate: %u , bxt Rate:%u\n", header.byterate, header.byterate*8);
        fprintf(f, "%s", buffer4);

        read = fread(buffer2, sizeof(buffer2), 1, ptr);
        printf("%u %u \n", buffer2[0], buffer2[1]);

        header.block_align = buffer2[0] |
                             (buffer2[1] << 8);
        printf("(33-34) Block Alignment: %u \n", header.block_align);
        fprintf(f, "%s", buffer2);

        read = fread(buffer2, sizeof(buffer2), 1, ptr);
        printf("%u %u \n", buffer2[0], buffer2[1]);

        header.bits_per_sample = buffer2[0] |
                                 (buffer2[1] << 8);
        printf("(35-36) bxts per sample: %u \n", header.bits_per_sample);
        fprintf(f, "%s", buffer2);

        read = fread(header.data_chunk_header, sizeof(header.data_chunk_header), 1, ptr);
        printf("(37-40) Data Marker: %s \n", header.data_chunk_header);
        fprintf(f, "%s", header.data_chunk_header);

        read = fread(buffer4, sizeof(buffer4), 1, ptr);
        printf("%u %u %u %u\n", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

        header.data_size = buffer4[0] |
                           (buffer4[1] << 8) |
                           (buffer4[2] << 16) |
                           (buffer4[3] << 24 );
        printf("(41-44) Size of data chunk: %u \n", header.data_size);
        fprintf(f, "%s", buffer4);

        // calculate no.of samples
        long num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
        printf("Number of samples:%lu \n", num_samples);

        long size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
        printf("Size of each sample:%ld bytes\n", size_of_each_sample);

        // read each sample from data chunk if PCM
        if (header.format_type == 1) { // PCM
                printf("Dump sample data? Y/N?");
                char c = 'n';
                scanf("%c", &c);
                if (c == 'Y' || c == 'y') {
                        long i =0;
                        char data_buffer[size_of_each_sample];

                        // make sure that the bytes-per-sample is completely divisible by num.of channels
                        long bytes_in_each_channel = (size_of_each_sample / header.channels);

                        //NEW CODE GOES HERE!
                        for (int x = 0; x<W; x++) {
                                for (int y = 0; y < D; y++) {
                                        for (int z = 0; z < H; z++) {

                                                if(nodes[x][y][z] == 'S') {
                                                        printf("Found source!\n" );

                                                        for (i =1; i <= num_samples; i++) {
                                                                read = fread(data_buffer, sizeof(data_buffer), 1, ptr);
                                                                if (read == 1) {


                                                                        // dump the data read
                                                                        unsigned int xchannels = 0;
                                                                        int data_in_channel = 0;

                                                                        for (xchannels = 0; xchannels < header.channels; xchannels++ ) {

                                                                                if (bytes_in_each_channel == 1) {
                                                                                        data_in_channel = data_buffer[0];
                                                                                }
                                                                                else{
                                                                                        return -1;
                                                                                }
                                                                                //fprintf(f, "%s", data_buffer);


                                                                                //Inject sound
                                                                                float pup = 0,pdown = 0,pleft=0,pright=0,pforward=0,pback=0;
                                                                                if(x - 1 >= 0 && x + 1 < W) {
                                                                                        pup = buffer[x-1][y][z];
                                                                                        pdown = buffer[x+1][y][z];

                                                                                }
                                                                                if(y - 1 >= 0 && y + 1 < D) {
                                                                                        pleft = buffer[x][y-1][z];
                                                                                        pright = buffer[x][y+1][z];

                                                                                }
                                                                                if(z - 1 >= 0 && z + 1 < H) {
                                                                                        pforward = buffer[x][y][z-1];
                                                                                        pback = buffer[x][y][z+1];
                                                                                }

                                                                                //float p = (pup + pdown + pleft + pright + pback + pforward) / 3;
                                                                                float p = data_in_channel;
                                                                                buffer[x][y][z]+=data_in_channel;


                                                                                if(x - 1 >= 0 && x + 1 < W) {
                                                                                        buffer[x-1][y][z] = p - pup;
                                                                                        buffer[x+1][y][z] = p-pdown;

                                                                                }
                                                                                if(y - 1 >= 0 && y + 1 < D) {
                                                                                        buffer[x][y-1][z] = p-pleft;
                                                                                        buffer[x][y+1][z] = p-pright;

                                                                                }
                                                                                if(z - 1 >= 0 && z + 1 < H) {
                                                                                        buffer[x][y][z-1] = p-pforward;
                                                                                        buffer[x][y][z+1] = p-pback;
                                                                                }

                                                                                //back
                                                                                int a = x;
                                                                                int b = y;
                                                                                int c = z;
                                                                                for(int bx = a; bx>=0; bx-- ) {
                                                                                        for(int by = b; by >= 0; by--) {
                                                                                                for(int bz = c; bz>=0; bz--) {
                                                                                                        if(nodes[bx][by][bz] == ' ') {
                                                                                                                float pup = 0,pdown = 0,pleft=0,pright=0,pforward=0,pback=0;
                                                                                                                if(bx - 1 >= 0 && bx + 1 < W) {
                                                                                                                        pup = buffer[bx-1][by][bz];
                                                                                                                        pdown = buffer[bx+1][by][bz];

                                                                                                                }
                                                                                                                if(by - 1 >= 0 && by + 1 < D) {
                                                                                                                        pleft = buffer[bx][by-1][bz];
                                                                                                                        pright = buffer[bx][by+1][bz];

                                                                                                                }
                                                                                                                if(bz - 1 >= 0 && bz + 1 < H) {
                                                                                                                        pforward = buffer[bx][by][bz-1];
                                                                                                                        pback = buffer[bx][by][bz+1];
                                                                                                                }

                                                                                                                float p = (pup + pdown + pleft + pright + pback + pforward) / 3;




                                                                                                                if(bx - 1 >= 0 && bx + 1 < W) {
                                                                                                                        buffer[bx-1][by][bz] = p - pup;
                                                                                                                        buffer[bx+1][by][bz] = p-pdown;

                                                                                                                }
                                                                                                                if(by - 1 >= 0 && by + 1 < D) {
                                                                                                                        buffer[bx][by-1][bz] = p-pleft;
                                                                                                                        buffer[bx][by+1][bz] = p-pright;

                                                                                                                }
                                                                                                                if(bz - 1 >= 0 && bz + 1 < H) {
                                                                                                                        buffer[bx][by][bz-1] = p-pforward;
                                                                                                                        buffer[bx][by][bz+1] = p-pback;
                                                                                                                }
                                                                                                        }
                                                                                                        if(nodes[bx][by][bz] == 'R') {
                                                                                                                //data_buffer[0] = data_in_channel;//buffer[bx][by][bz];
                                                                                                                data_buffer[0] = buffer[bx][by][bz];
                                                                                                                fprintf(f, "%s", data_buffer);
                                                                                                        }
                                                                                                }
                                                                                                c = H-1;
                                                                                        }
                                                                                        b = D-1;
                                                                                }

                                                                                //forward
                                                                                a = x;
                                                                                b = y;
                                                                                c = z+1;
                                                                                for(int fx = a; fx<W; fx++ ) {
                                                                                        for(int fy = b; fy < D; fy++) {
                                                                                                for(int fz = c; fz<H; fz++) {
                                                                                                        if(nodes[fx][fy][fz] == ' ') {
                                                                                                                float pup = 0,pdown = 0,pleft=0,pright=0,pforward=0,pback=0;
                                                                                                                if(fx - 1 >= 0 && fx + 1 < W) {
                                                                                                                        pup = buffer[fx-1][fy][fz];
                                                                                                                        pdown = buffer[fx+1][fy][fz];

                                                                                                                }
                                                                                                                if(fy - 1 >= 0 && fy + 1 < D) {
                                                                                                                        pleft = buffer[fx][fy-1][fz];
                                                                                                                        pright = buffer[fx][fy+1][fz];

                                                                                                                }
                                                                                                                if(fz - 1 >= 0 && fz + 1 < H) {
                                                                                                                        pforward = buffer[fx][fy][fz-1];
                                                                                                                        pback = buffer[fx][fy][fz+1];
                                                                                                                }

                                                                                                                float p = (pup + pdown + pleft + pright + pback + pforward) / 3;




                                                                                                                if(fx - 1 >= 0 && fx + 1 < W) {
                                                                                                                        buffer[fx-1][fy][fz] = p - pup;
                                                                                                                        buffer[fx+1][fy][fz] = p-pdown;

                                                                                                                }
                                                                                                                if(fy - 1 >= 0 && fy + 1 < D) {
                                                                                                                        buffer[fx][fy-1][fz] = p-pleft;
                                                                                                                        buffer[fx][fy+1][fz] = p-pright;

                                                                                                                }
                                                                                                                if(fz - 1 >= 0 && fz + 1 < H) {
                                                                                                                        buffer[fx][fy][fz-1] = p-pforward;
                                                                                                                        buffer[fx][fy][fz+1] = p-pback;
                                                                                                                }
                                                                                                        }
                                                                                                        if(nodes[fx][fy][fz] == 'R') {
                                                                                                                //data_buffer[0] = data_in_channel;//buffer[fx][fy][fz];
                                                                                                                data_buffer[0] = buffer[fx][fy][fz];

                                                                                                                fprintf(f, "%s", data_buffer);
                                                                                                        }
                                                                                                }
                                                                                                c = 0;
                                                                                        }
                                                                                        b = 0;
                                                                                }

                                                                        }
                                                                        // if(nodes[x][y][z] == 'R') {
                                                                        //         printf("Found reciever!\n" );
                                                                        // }
                                                                }
                                                                else {
                                                                        printf("Error reading file. %d bytes\n", read);
                                                                        break;
                                                                }

                                                        }
                                                }

                                        }
                                }
                        }

                        //  for (i =1; i <= num_samples; i++) {
                }
        }

        for(int i = 0; i<W; i++) {
                for(int j = 0; j<D; j++) {
                        for(int k = 0; k<H; k++) {
                                printf("%f ",buffer[i][j][k]);
                        }
                        printf("\n");
                }
        }

        printf("Closing file..\n");
        fclose(ptr);
        fclose(f);
        free(nodes);
        free(buffer);

        // cleanup before quitting
        free(filename);
        return 0;

}
