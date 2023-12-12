// File name: convert.c 
// Description: File converts (decodes/encodes) between PCM and G.711 (mulaw,alaw) encodings. Has capability
// to output either wave file or raw PCM file.
//
// Tools: gcc, Make to compile (see Makefile for reference)
//
// Leveraged code: Used Apple's opensource snack library for the mulaw/alaw encodings/decodings:
// https://opensource.apple.com/source/tcl/tcl-20/tcl_ext/snack/snack/generic/g711.c
//
// Authors: Michael Starks and Sovann Chak
//
// Used information in this webpage to generate/parse wave header
// https://docs.fileformat.com/audio/wav/
//

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h> 
#include <stdbool.h>

#include "g711.h"

#define WAVE_FORMAT_PCM  (0x01)
#define WAVE_FORMAT_ALAW (0x06)
#define WAVE_FORMAT_ULAW (0x07)

#define BPS_8            (0x08)
#define BPS_16           (0x10)


// Wave File Header
// Positions	Sample Value	Description
// ---
// 1 – 4	“RIFF”	Marks the file as a riff file. Characters are each 1 byte long.
// 5 – 8	File size (integer)	Size of the overall file – 8 bytes, in bytes (32-bit integer). Typically, you’d fill this in after creation.
// 9 -12	“WAVE”	File Type Header. For our purposes, it always equals “WAVE”.
// 13-16	“fmt “	Format chunk marker. Includes trailing null
// 17-20	16	Length of format data as listed above
// 21-22	1	Type of format (1 is PCM) – 2 byte integer
// 23-24	2	Number of Channels – 2 byte integer
// 25-28	44100	Sample Rate – 32 byte integer. Common values are 44100 (CD), 48000 (DAT). Sample Rate = Number of Samples per second, or Hertz.
// 29-32	176400	(Sample Rate * BitsPerSample * Channels) / 8.
// 33-34	4	(BitsPerSample * Channels) / 8.1 – 8 bit mono2 – 8 bit stereo/16 bit mono4 – 16 bit stereo
// 35-36	16	Bits per sample
// 37-40	“data”	“data” chunk header. Marks the beginning of the data section.
// 41-44	File size (data)	Size of the data section.
typedef struct 
{
    uint8_t  type[5];
    uint32_t file_size;
    uint8_t  wave_marker[5];
    uint8_t  fmt[5];
    uint32_t fmt_length;
    uint16_t fmt_type;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t sample_calc1;
    uint16_t bit_mode; 
    uint16_t bits_per_sample; 
    uint8_t  data_marker[5];
    uint32_t data_size;

} wave_file_header_t;

// For debug purposes - simply prints out the header
// to console
static void print_header(wave_file_header_t header)
{
    printf("Type: %s\n", header.type);
    printf("File Size: %d\n", header.file_size);
    printf("Wave Marker: %s\n", header.wave_marker);
    
    printf("Format: %s\n", header.fmt);
    printf("Format Length: %d\n", header.fmt_length);
    printf("Format Type: %d\n", header.fmt_type);
    
    printf("Num Channels: %d\n", header.num_channels);
    
    printf("Sample Rate: %d\n", header.sample_rate);
    printf("Sample Calc: %d\n", header.sample_calc1);
    
    printf("Bit Mode: %d\n", header.bit_mode);
    printf("Bits Per Sample: %d\n", header.bits_per_sample);
    
    printf("Data Marker: %s\n", header.data_marker);
    printf("Data Size: %d\n", header.data_size);
}

// Writes the wave_file_header_t struct to a file
static void write_header_to_file(wave_file_header_t header, FILE* output)
{
    fwrite(header.type, sizeof(uint8_t), 4, output);

    fwrite(&header.file_size, sizeof(uint32_t), 1, output);

    fwrite(header.wave_marker, sizeof(uint8_t), 4, output);

    fwrite(header.fmt, sizeof(uint8_t), 4, output);
    fwrite(&header.fmt_length, sizeof(uint32_t), 1, output);
    fwrite(&header.fmt_type, sizeof(uint16_t), 1, output);

    fwrite(&header.num_channels, sizeof(uint16_t), 1, output);

    fwrite(&header.sample_rate, sizeof(uint32_t), 1, output);
    fwrite(&header.sample_calc1, sizeof(uint32_t), 1, output);

    fwrite(&header.bit_mode, sizeof(uint16_t), 1, output);
    fwrite(&header.bits_per_sample, sizeof(uint16_t), 1, output);

    // there is no  null marker for this entry
    fwrite(header.data_marker, sizeof(uint8_t), 4, output);
    header.data_marker[4] = '\0';

    fwrite(&header.data_size, sizeof(uint32_t), 1, output);
}

// Generates a wave_file_header_t parsed from input file
static void init_wave_struct(wave_file_header_t *header, FILE* input_file)
{

    // there is no  null marker for this entry
    fread(header->type, sizeof(uint8_t), 4, input_file);
    header->type[4] = '\0'; 

    fread(&header->file_size, sizeof(uint32_t), 1, input_file);

    // there is no  null marker for this entry
    fread(header->wave_marker, sizeof(uint8_t), 4, input_file);
    header->wave_marker[4] = '\0';

    fread(header->fmt, sizeof(uint8_t), 4, input_file);
    fread(&header->fmt_length, sizeof(uint32_t), 1, input_file);
    fread(&header->fmt_type, sizeof(uint16_t), 1, input_file);

    fread(&header->num_channels, sizeof(uint16_t), 1, input_file);

    fread(&header->sample_rate, sizeof(uint32_t), 1, input_file);
    fread(&header->sample_calc1, sizeof(uint32_t), 1, input_file);

    fread(&header->bit_mode, sizeof(uint16_t), 1, input_file);
    fread(&header->bits_per_sample, sizeof(uint16_t), 1, input_file);

    // there is no  null marker for this entry
    fread(header->data_marker, sizeof(uint8_t), 4, input_file);
    header->data_marker[4] = '\0';

    fread(&header->data_size, sizeof(uint32_t), 1, input_file);
}

// NOTE: Sizes for the strings do not match the byte count because
// in the actual binary file we don't want the null character, but
// I keep them in the struct for printing/debuging purposes 
//
// Generates wave header with some custom arguments to set the values
// in the header
static void generate_wav_header(wave_file_header_t *header, uint32_t sample_rate, 
                                uint32_t byte_count, uint16_t fmt_type, uint16_t bps)
{
    uint8_t header_byte_count = 0;
    memcpy(header->type, "RIFF\0", 5);
    header_byte_count+=4;
    header->file_size = 0;
    header_byte_count+=4;
    memcpy(header->wave_marker, "WAVE\0", 5);
    header_byte_count+=4;
    memcpy(header->fmt, "fmt \0", 5);
    header_byte_count+=4;
    header->fmt_length = header_byte_count;
    header_byte_count+=4;
    header->fmt_type = fmt_type;
    header_byte_count+=2;
    header->num_channels = 1;
    header_byte_count+=2;
    header->sample_rate = sample_rate;
    header_byte_count+=4;
    header->bits_per_sample = bps;
    header_byte_count+=2;
    header->sample_calc1 = (header->sample_rate * header->bits_per_sample * header->num_channels)/4;
    header_byte_count+=4;
    header->bit_mode = header->num_channels * header->bits_per_sample/8;
    header_byte_count+=2;
    memcpy(header->data_marker, "data\0", 5);
    header_byte_count+=4;
    header->data_size = byte_count;
    header_byte_count+=4;

    // finally update filesize
    header->file_size = (header_byte_count + header->data_size) - 8;
}

//The program looks for file extensions, so if your input file file has a .wav extension, it assumes
//there is a wav header and parses it. If your output file has a .wav extension, it will generate a 
//wav header. Directions to use the program, it expects 3 arguments --
//
//	Argument 1: path to input file (.wav or .PCM)
//	Argument 2: path to output file (.wav or .PCM)
//	Argument 3:
//		1 - u-Law encode
//		2 - u-Law decode
//		3 - A-Law encode
//		4 - A-Law decode
//
//Some examples:
//
// * Generate a ulaw wav file from a PCM wav file: ./convert input_file.wav output_file.wav 1 
//
// * Generate a raw ulaw encoded file from a PCM wav file: ./convert input_file.wav output_file 1 
int main(int args, char* argv[])
{
    if(args != 4)
    {
        printf("\nUnexpected number of arguments: %d ... \n", args);
        printf("\tArgument 1: path to input file (.wav or .PCM)\n\tArgument 2: path to output file (.wav or .PCM)\n\tArgument 3: \n");
        printf("\t\t1 - u-Law encode\n\t\t2 - u-Law decode\n\t\t3 - A-Law encode\n\t\t4 - A-Law decode\n");
        printf("Exiting!\n");
        return -1;
    }

    FILE *input_file;
    FILE *output_file;
    int16_t val16 = 0;
    uint8_t  val8 = 0;

    uint32_t count = 0;
    uint32_t byte_count = 0;

    bool is_input_wav = false;
    bool is_output_wav = false;

    wave_file_header_t wav_header = {};
    wave_file_header_t wav_header_new = {};

    // Open a file in read mode
    input_file  = fopen(argv[1], "r");
    output_file = fopen(argv[2], "w");

    char* ext = strrchr(argv[1], '.');
    if (ext)
    {
        if((strcmp(ext, ".wav") == 0) || (strcmp(ext, ".WAV") == 0))
        {
            is_input_wav = true;
            printf("Treating input as wav file\n");
        }
        else 
        {
            printf("Treating input as PCM file\n");
        }
    }
    else 
    {
        printf("Treating input as PCM file\n");
    }

    ext = strrchr(argv[2], '.');
    if (ext)
    {
        if((strcmp(ext, ".wav") == 0) || (strcmp(ext, ".WAV") == 0))
        {
            is_output_wav = true;
        
            printf("Generating output as wav\n");
        }
        else 
        {
            printf("Generating output as PCM\n");
        }
    }
    else 
    {
        printf("Generating output as PCM\n");
    }


    if(is_input_wav)
    {
        init_wave_struct(&wav_header, input_file);
    }

    if(is_output_wav)
    {
        // data starts at byte 44
        fseek(output_file, 44, SEEK_SET);
    }


    switch(atoi(argv[3]))
    {
        case 1: 
            printf("u-Law Encoding\n");

            while(fread(&val16, sizeof(int16_t), 1, input_file) == 1)
            {
                val8 = Snack_Lin2Mulaw(val16);
                fwrite(&val8, 1, 1, output_file);
                byte_count+=1;
            }

            // write header at index 0
            fseek(output_file, 0, SEEK_SET);

            if(is_output_wav)
            {
                generate_wav_header(&wav_header_new, 16000, byte_count, WAVE_FORMAT_ULAW, BPS_8);
                write_header_to_file(wav_header_new, output_file);
            }

            break;
        case 2: 
            printf("u-Law Decoding\n");

            while(fread(&val8, sizeof(uint8_t), 1, input_file) == 1)
            {
                val16 = Snack_Mulaw2Lin(val8) ;
                fwrite(&val16, sizeof(int16_t), 1, output_file);
                byte_count+=2;
            }

            if(is_output_wav)
            {
                fseek(output_file, 0, SEEK_SET);
                generate_wav_header(&wav_header_new, 8000, byte_count, WAVE_FORMAT_PCM, BPS_16);
                write_header_to_file(wav_header_new, output_file);
                //print_header(wav_header_new);
            }

            break;
        case 3: 
            printf("A-Law Encoding\n");

            while(fread(&val16, sizeof(int16_t), 1, input_file) == 1)
            {
                val8 = Snack_Lin2Alaw(val16);
                fwrite(&val8, 1, 1, output_file);
                byte_count+=1;
            }

            if(is_output_wav)
            {
                fseek(output_file, 0, SEEK_SET);
                generate_wav_header(&wav_header_new, 22050, byte_count, WAVE_FORMAT_ALAW, BPS_8);
                write_header_to_file(wav_header_new, output_file);
            }

            break;
        case 4: 
            printf("A-Law Decoding\n");

            while(fread(&val8, sizeof(uint8_t), 1, input_file) == 1)
            {
                val16 = Snack_Alaw2Lin(val8);
                fwrite(&val16, sizeof(int16_t), 1, output_file);
                byte_count+=2;
            }

            if(is_output_wav)
            {
                fseek(output_file, 0, SEEK_SET);
                generate_wav_header(&wav_header_new, 22050, byte_count, WAVE_FORMAT_PCM, BPS_16);
                write_header_to_file(wav_header_new, output_file);
            }

            break;

        case 5:
            print_header(wav_header);

    }

    fclose(output_file);
    fclose(input_file);

    return 0;
}

