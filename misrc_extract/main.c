/*
* MISRC extract
* Copyright (C) 2024  vrunk11, stefan_o
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#ifndef _WIN32
	#include <getopt.h>
	#define aligned_free(x) free(x)
	#define PERF_MEASURE 1
#else
	#include <windows.h>
	#include <io.h>
	#include <fcntl.h>
	#include "getopt/getopt.h"
	#define aligned_free(x) _aligned_free(x)
	#define aligned_alloc(a,s) _aligned_malloc(s,a)
	#define PERF_MEASURE 0
#endif


#if PERF_MEASURE
#include <time.h>
#endif

#define VERSION "0.2"
#define COPYRIGHT "licensed under GNU GPL v3 or later, (c) 2024 vrunk11, stefan_o"

#define BUFFER_SIZE 65536*32

#define _FILE_OFFSET_BITS 64

void usage(void)
{
	fprintf(stderr,
		"A simple program for extracting captured data into separate files\n\n"
		"Usage:\n"
		"\t[-i input file (use '-' to read from stdin)]\n"
		"\t[-a ADC A output file (use '-' to write on stdout)]\n"
		"\t[-b ADC B output file (use '-' to write on stdout)]\n"
		"\t[-x AUX output file (use '-' to write on stdout)]\n"
		"\t[-p pad lower 4 bits of 16 bit output with 0 instead of upper 4]\n"
	);
	exit(1);
}

//bit masking
#define MASK_1      0xFFF
#define MASK_2      0xFFF00000
#define MASK_AUX    0xFF000

void extract_A_C(uint32_t *in, size_t len, size_t *clip, uint8_t *aux, int16_t *outA, int16_t *outB) {
	for(size_t i = 0; i < len; i++)
	{
		outA[i]  = 2048 - ((int16_t)(in[i] & MASK_1));
		aux[i]   = (in[i] & MASK_AUX) >> 12;
		clip[0] += ((in[i] >> 12) & 1);
	}
}
void extract_B_C(uint32_t *in, size_t len, size_t *clip, uint8_t *aux, int16_t *outA, int16_t *outB) {
	for(size_t i = 0; i < len; i++)
	{
		outB[i]  = 2048 - ((int16_t)((in[i] & MASK_2) >> 20));
		aux[i]   = (in[i] & MASK_AUX) >> 12;
		clip[1] += ((in[i] >> 13) & 1);
	}
}

void extract_AB_C(uint32_t *in, size_t len, size_t *clip, uint8_t *aux, int16_t *outA, int16_t *outB) {
	for(size_t i = 0; i < len; i++)
	{
		outA[i]  = 2048 - ((int16_t)(in[i] & MASK_1));
		outB[i]  = 2048 - ((int16_t)((in[i] & MASK_2) >> 20));
		aux[i]   = (in[i] & MASK_AUX) >> 12;
		clip[0] += ((in[i] >> 12) & 1);
		clip[1] += ((in[i] >> 13) & 1);
	}
}

void extract_A_p_C(uint32_t *in, size_t len, size_t *clip, uint8_t *aux, int16_t *outA, int16_t *outB) {
	for(size_t i = 0; i < len; i++)
	{
		outA[i]  = (2048 - ((int16_t)(in[i] & MASK_1)))<<4;
		aux[i]   = (in[i] & MASK_AUX) >> 12;
		clip[0] += ((in[i] >> 12) & 1);
	}
}
void extract_B_p_C(uint32_t *in, size_t len, size_t *clip, uint8_t *aux, int16_t *outA, int16_t *outB) {
	for(size_t i = 0; i < len; i++)
	{
		outB[i]  = (2048 - ((int16_t)((in[i] & MASK_2) >> 20)))<<4;
		aux[i]   = (in[i] & MASK_AUX) >> 12;
		clip[1] += ((in[i] >> 13) & 1);
	}
}

void extract_AB_p_C(uint32_t *in, size_t len, size_t *clip, uint8_t *aux, int16_t *outA, int16_t *outB) {
	for(size_t i = 0; i < len; i++)
	{
		outA[i]  = (2048 - ((int16_t)(in[i] & MASK_1)))<<4;
		outB[i]  = (2048 - ((int16_t)((in[i] & MASK_2) >> 20)))<<4;
		aux[i]   = (in[i] & MASK_AUX) >> 12;
		clip[0] += ((in[i] >> 12) & 1);
		clip[1] += ((in[i] >> 13) & 1);
	}
}

#if defined(__x86_64__) || defined(_M_X64)
void extract_A_sse   (uint32_t *in, size_t len, size_t *clip, uint8_t *aux, int16_t *outA, int16_t *outB);
void extract_B_sse   (uint32_t *in, size_t len, size_t *clip, uint8_t *aux, int16_t *outA, int16_t *outB);
void extract_AB_sse  (uint32_t *in, size_t len, size_t *clip, uint8_t *aux, int16_t *outA, int16_t *outB);
void extract_A_p_sse (uint32_t *in, size_t len, size_t *clip, uint8_t *aux, int16_t *outA, int16_t *outB);
void extract_B_p_sse (uint32_t *in, size_t len, size_t *clip, uint8_t *aux, int16_t *outA, int16_t *outB);
void extract_AB_p_sse(uint32_t *in, size_t len, size_t *clip, uint8_t *aux, int16_t *outA, int16_t *outB);
int check_cpu_feat();
#endif

int main(int argc, char **argv)
{
//set pipe mode to binary in windows
#if defined(_WIN32) || defined(_WIN64)
	_setmode(_fileno(stdout), O_BINARY);
	_setmode(_fileno(stdin), O_BINARY);
#endif

	int opt, pad=0;


	//file adress
	char *input_name_1    = NULL;
	char *output_name_1   = NULL;
	char *output_name_2   = NULL;
	char *output_name_aux = NULL;
	
	//input file
	FILE *input_1;
	
	//output files
	FILE *output_1;
	FILE *output_2;
	FILE *output_aux;
	
	//buffer
	uint32_t *buf_tmp = aligned_alloc(16,sizeof(uint32_t)*BUFFER_SIZE);
	int16_t  *buf_1   = aligned_alloc(16,sizeof(int16_t) *BUFFER_SIZE);
	int16_t  *buf_2   = aligned_alloc(16,sizeof(int16_t) *BUFFER_SIZE);
	uint8_t  *buf_aux = aligned_alloc(16,sizeof(uint8_t) *BUFFER_SIZE);
	
	//number of byte read
	int nb_block;
	
	//clipping state
	size_t clip[2] = {0, 0};
	
	// conversion function
	void (*conv_function)(uint32_t *in, size_t len, size_t *clip, uint8_t *aux, int16_t *outA, int16_t *outB);

#if PERF_MEASURE
	struct timespec start, stop;
	double timeread = 0.0, timeconv = 0.0, timewrite = 0.0;
#endif

	fprintf(stderr,
		"MISRC extract " VERSION "\n"
		COPYRIGHT "\n\n"
	);

	while ((opt = getopt(argc, argv, "i:a:b:x:p")) != -1) {
		switch (opt) {
		case 'i':
			input_name_1 = optarg;
			break;
		case 'a':
			output_name_1 = optarg;
			break;
		case 'b':
			output_name_2 = optarg;
			break;
		case 'x':
			output_name_aux = optarg;
			break;
		case 'p':
			pad = 1;
			break;
		default:
			usage();
			break;
		}
	}
	
	if(input_name_1 == NULL || (output_name_1 != NULL && output_name_2 != NULL && output_name_aux != NULL))
	{
		usage();
	}
	
	//reading file 1
	if(input_name_1 != NULL)
	{
		if (strcmp(input_name_1, "-") == 0)// Read samples from stdin
		{
			input_1 = stdin;
		}
		else 
		{
			input_1 = fopen(input_name_1, "rb");
			if (!input_1) {
				fprintf(stderr, "(1) : Failed to open %s\n", input_name_1);
				return -ENOENT;
			}
		}
	}
	
	if(output_name_1 != NULL)
	{
		//opening output file 1
		if (strcmp(output_name_1, "-") == 0)// Read samples from stdin
		{
			output_1 = stdout;
		}
		else
		{
			output_1 = fopen(output_name_1, "wb");
			if (!output_1) {
				fprintf(stderr, "(2) : Failed to open %s\n", output_name_1);
				return -ENOENT;
			}
		}
	}
	
	if(output_name_2 != NULL)
	{
		//opening output file 2
		if (strcmp(output_name_2, "-") == 0)// Read samples from stdin
		{
			output_2 = stdout;
		}
		else if(output_name_2 != NULL)
		{
			output_2 = fopen(output_name_2, "wb");
			if (!output_2) {
				fprintf(stderr, "(2) : Failed to open %s\n", output_name_2);
				return -ENOENT;
			}
		}
	}
	
	if(output_name_aux != NULL)
	{
		//opening output file aux
		if (strcmp(output_name_aux, "-") == 0)// Read samples from stdin
		{
			output_aux = stdout;
		}
		else if(output_name_aux != NULL)
		{
			output_aux = fopen(output_name_aux, "wb");
			if (!output_aux) {
				fprintf(stderr, "(2) : Failed to open %s\n", output_name_aux);
				return -ENOENT;
			}
		}
	}
#if defined(__x86_64__) || defined(_M_X64)
	if(check_cpu_feat()==0) {
		fprintf(stderr,"Detected processor with SSSE3 and POPCNT, using optimized extraction routine\n\n");
		if (pad==1) {
			if (output_name_1 == NULL) conv_function = &extract_B_p_sse;
			else if (output_name_2 == NULL) conv_function = &extract_A_p_sse;
			else conv_function = &extract_AB_p_sse;
		}
		else {
			if (output_name_1 == NULL) conv_function = &extract_B_sse;
			else if (output_name_2 == NULL) conv_function = &extract_A_sse;
			else conv_function = &extract_AB_sse;
		}
	}
	else {
		fprintf(stderr,"Detected processor without SSSE3 and POPCNT, using standard extraction routine\n\n");
#endif
		if (pad==1) {
			if (output_name_1 == NULL) conv_function = &extract_B_p_C;
			else if (output_name_2 == NULL) conv_function = &extract_A_p_C;
			else conv_function = &extract_AB_p_C;
		}
		else {
			if (output_name_1 == NULL) conv_function = &extract_B_C;
			else if (output_name_2 == NULL) conv_function = &extract_A_C;
			else conv_function = &extract_AB_C;
		}
#if defined(__x86_64__) || defined(_M_X64)
	}
#endif
	
	if(input_name_1 != NULL && (output_name_1 != NULL || output_name_2 != NULL || output_name_aux != NULL))
	{
		while(!feof(input_1))
		{
#if PERF_MEASURE
			clock_gettime(CLOCK_MONOTONIC, &start);
#endif

			nb_block = fread(buf_tmp,4,BUFFER_SIZE,input_1);

#if PERF_MEASURE
			clock_gettime(CLOCK_MONOTONIC, &stop);
			timeread += (stop.tv_sec - start.tv_sec) * 1e6 + (stop.tv_nsec - start.tv_nsec) / 1e3;
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
#endif

			conv_function(buf_tmp, nb_block, clip, buf_aux, buf_1, buf_2);

#if PERF_MEASURE
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
			timeconv += (stop.tv_sec - start.tv_sec) * 1e6 + (stop.tv_nsec - start.tv_nsec) / 1e3;
#endif

			if(clip[0] > 0)
			{
				fprintf(stderr,"ADC A : %ld samples clipped\n",clip[0]);
				clip[0] = 0; 
			}
			
			if(clip[1] > 0)
			{
				fprintf(stderr,"ADC B : %ld samples clipped\n",clip[1]);
				clip[1] = 0; 
			}
#if PERF_MEASURE
			clock_gettime(CLOCK_MONOTONIC, &start);
#endif
			//write output
			if(output_name_1   != NULL){fwrite(buf_1, 2,nb_block,output_1);}
			if(output_name_2   != NULL){fwrite(buf_2, 2,nb_block,output_2);}
			if(output_name_aux != NULL){fwrite(buf_aux,1,nb_block,output_aux);}

#if PERF_MEASURE
			clock_gettime(CLOCK_MONOTONIC, &stop);
			timewrite += (stop.tv_sec - start.tv_sec) * 1e6 + (stop.tv_nsec - start.tv_nsec) / 1e3;
#endif
		}
	}

////ending of the program
	
	aligned_free(buf_1);
	aligned_free(buf_2);
	aligned_free(buf_aux);
	
	//Close file 1
	if(input_name_1 != NULL)
	{
		if (input_1 && (input_1 != stdin))
		{
			fclose(input_1);
		}
	}
	
	if(output_name_1 != NULL)
	{
		//Close out file
		if (output_1 && (output_1 != stdout))
		{
			fclose(output_1);
		}
	}
	
	if(output_name_2 != NULL)
	{
		if (output_2 && (output_2 != stdout))
		{
			fclose(output_2);
		}
	}
	
	if(output_name_aux != NULL)
	{
		if (output_aux && (output_aux != stdout))
		{
			fclose(output_aux);
		}
	}

#if PERF_MEASURE
	fprintf(stderr, "Readtime: %f\nConvtime: %f\nwrittime: %f\n", timeread, timeconv, timewrite);
#endif

	return 0;
}
