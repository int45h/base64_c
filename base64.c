#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;
typedef unsigned long   u64;

u8
decode_chunk(char chunklet)
{
    if      (chunklet >= 'A' && chunklet <= 'Z')    return (chunklet - 'A');
    else if (chunklet >= 'a' && chunklet <= 'z')    return (chunklet - 'a') + 26;
    else if (chunklet >= '0' && chunklet <= '9')    return (chunklet - '0') + 52;
    else switch (chunklet)
    {   case '+': return 62;
        case '/': return 63;    }
    return 0;
}

void
b64_decode (const char *input, char **output, u64 *size_ptr)
{
    if (*output != NULL) free(*output);

    /* 4 6-bit chunks can fit into a 24-bit block */
    u64     size        = strlen(input),
            chunk_size  = (size / 4),
            i;
    *output             = malloc(chunk_size*3*sizeof(char));
    *size_ptr           = chunk_size*3;

    for (i = 0; i < chunk_size; i++)
    {
        u32 block = (   (decode_chunk(input[(i*4) + 0]) << 18)    |
                        (decode_chunk(input[(i*4) + 1]) << 12)    |
                        (decode_chunk(input[(i*4) + 2]) << 6)     |
                        (decode_chunk(input[(i*4) + 3])));
        
        (*output)[(i*3) + 0] = ((block & 0x00FF0000) >> 16);
        (*output)[(i*3) + 1] = ((block & 0x0000FF00) >> 8);
        (*output)[(i*3) + 2] = ((block & 0x000000FF));
    }
}

void
b64_encode (const char *input, char **output, u64 *size_ptr)
{
    if (*output != NULL) free(*output);

    const char  *encode_str = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    u64         size        = *size_ptr,
                chunk_size  = (size / 3),
                remainder   = size - chunk_size,
                i;
    *output                 = malloc((chunk_size+1)*4*sizeof(char)+1);
    *size_ptr               = (chunk_size+1)*4+1;

    for (i = 0; i < chunk_size; i++)
    {
        u8  a = ((input[(i*3)+0] & 0xFC) >> 2),
            b = ((input[(i*3)+0] & 0x03) << 4) | ((input[(i*3)+1] & 0xF0) >> 4),
            c = ((input[(i*3)+1] & 0x0F) << 2) | ((input[(i*3)+2] & 0xC0) >> 6),
            d = ((input[(i*3)+2] & 0x3F));
        
        (*output)[(i*4)+0] = encode_str[a];
        (*output)[(i*4)+1] = encode_str[b];
        (*output)[(i*4)+2] = encode_str[c];
        (*output)[(i*4)+3] = encode_str[d];
    }
    if (remainder > 0)
    {
        u8  a = (remainder > 1) ? encode_str[((input[(chunk_size*3)+0] & 0xFC) >> 2)]   : '=',
            b = (remainder > 2) ? encode_str[((input[(chunk_size*3)+0] & 0x03) << 4) | 
                                             ((input[(chunk_size*3)+1] & 0xF0) >> 4)]   : '=',
            c = (remainder > 3) ? encode_str[((input[(chunk_size*3)+1] & 0x0F) << 2) | 
                                             ((input[(chunk_size*3)+2] & 0xC0) >> 4)]   : '=',
            d = '=';

        (*output)[(chunk_size*4) + 0] = a;
        (*output)[(chunk_size*4) + 1] = b;
        (*output)[(chunk_size*4) + 2] = c;
        (*output)[(chunk_size*4) + 3] = d;
    }
    (*output)[((chunk_size+1)*4)] = '\n';
}

char *
open_file(const char *filename, char **input, u64 *size)
{
    FILE *stream = fopen(filename, "r");
    if (stream == NULL || ferror(stream))
    {
        return(NULL);
    }
    else
    {
        fseek(stream, 0, SEEK_END);
        u32 file_size = ftell(stream);
        rewind(stream);
                        
        *input  = realloc(*input, file_size*sizeof(char));
        *size   = fread(*input, sizeof(char), file_size, stream);
        fclose(stream);
    }
    return *input;
}

void
usage()
{
    printf("Usage: b64 [OPTION]...[FILE]\n");
    printf("Base64 encode or decode FILE or standard input, to standard output.\n");
    printf("With no FILE, or when FILE is -, read standard input.\n");
    printf("\nOptions:\n");
    printf("\t-d, --decode\t\tDecode data\n");
    printf("\t-h, --help\t\tDisplay this text\n");
}

void
die(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    exit(1);
}

int 
main(int argc, char ** argv)
{
    char    *input  = malloc(2048*sizeof(char)),
            *output = NULL;
    u64     size;
    if (argc > 1)
    {
        u16 i;
        for (i = 1; i < argc; i++)
        {
            if      ((strncmp(argv[i], "-h", 2) == 0) || (strncmp(argv[i], "--help", 6) == 0))
            {
                usage();
                free(input);
                return 0;
            }
            if      ((strncmp(argv[i], "-d", 2) == 0) || (strncmp(argv[i], "--decode", 8) == 0))
            {
                /* Decode string or file */
                if      ((i == (argc - 1)) || (strncmp(argv[i+1], "-", 1) == 0)) 
                    goto stdin_decode;
                else
                {
                    if (open_file(argv[i+1], &input, &size) == NULL)
                    {
                        free(input);
                        die("%s: %s: %s\n", argv[0], argv[i+1], strerror(errno));
                    }
                    goto b64_decode_stream;
                }
            }
            else if ((strncmp(argv[i], "-", 1) == 0)) 
                goto stdin_encode;
            else
            {
                if (open_file(argv[i], &input, &size) == NULL)
                {
                    free(input);
                    die("%s: %s: %s\n", argv[0], argv[i], strerror(errno));
                }
                goto b64_encode_stream;
            }
        }
    }
    else goto stdin_encode;

    stdin_encode:
        size = fread(input, sizeof(char), 2048, stdin);
        goto b64_encode_stream;
    stdin_decode:
        size = fread(input, sizeof(char), 2048, stdin);
        goto b64_decode_stream;              
    b64_decode_stream:
        b64_decode(input, &output, &size);
        fwrite(output, sizeof(char), size, stdout);
        goto cleanup;
    b64_encode_stream:
        b64_encode(input, &output, &size);
        fwrite(output, sizeof(char), size, stdout);
        goto cleanup;
    cleanup:
        free(output);
        free(input);
    return 0;
}