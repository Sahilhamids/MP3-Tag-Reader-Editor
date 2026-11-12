#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/mp3.h"
#include "include/modify.h"

/*
 * ID3v1 layout (128 bytes at end of file):
 *   Bytes   0-2  : "TAG"
 *   Bytes   3-32 : Title   (30 bytes)
 *   Bytes  33-62 : Artist  (30 bytes)
 *   Bytes  63-92 : Album   (30 bytes)
 *   Bytes  93-96 : Year    (4 bytes, ASCII)
 *   Bytes  97-124: Comment (28 bytes, ID3v1.1)
 *   Byte  125    : Zero byte (ID3v1.1 indicator)
 *   Byte  126    : Track number
 *   Byte  127    : Genre index
 *
 * ID3v2.3 frame layout:
 *   Bytes 0-3  : Frame ID       ("TIT2", "TPE1", etc.)
 *   Bytes 4-7  : Frame size     (big-endian 32-bit)
 *   Bytes 8-9  : Flags
 *   Byte  10   : Text encoding  (0x00 = ISO-8859-1)
 *   Bytes 11+  : Text content
 *
 *   COMM frame is special:
 *   Byte  10   : Encoding
 *   Bytes 11-13: Language       ("eng")
 *   Byte  14   : Content descriptor (\0 for empty)
 *   Bytes 15+  : Actual text
 */

/* Helper function to decode synchsafe integer */
static unsigned long parse_synchsafe(const unsigned char *b)
{
    return ((unsigned long)(b[0] & 0x7F) << 21)
         | ((unsigned long)(b[1] & 0x7F) << 14)
         | ((unsigned long)(b[2] & 0x7F) <<  7)
         |  (unsigned long)(b[3] & 0x7F);
}

/* Helper to copy string and zero out remaining bytes */
static void write_field(char *buffer, int offset, int max_len, const char *value)
{
    memset(buffer + offset, 0, max_len);
    int len = (int)strlen(value);
    if (len > max_len)
        len = max_len;
    memcpy(buffer + offset, value, len);
}

/* Map CLI flags to ID3v2 frame IDs */
static const char *flag_to_frame(const char *flag)
{
    if (strcmp(flag, "-t") == 0) return "TIT2";
    if (strcmp(flag, "-a") == 0) return "TPE1";
    if (strcmp(flag, "-A") == 0) return "TALB";
    if (strcmp(flag, "-y") == 0) return "TYER";
    if (strcmp(flag, "-T") == 0) return "TRCK";
    if (strcmp(flag, "-C") == 0) return "COMM";
    if (strcmp(flag, "-g") == 0) return "TCON";
    return NULL;
}

/* Modify ID3v1 tag fields */
static void modify_id3v1(FILE *fptr, const char *flag, const char *new_value)
{
    if (fseek(fptr, -128, SEEK_END) != 0)
    {
        printf("ERROR: file too small for an ID3v1 tag\n");
        return;
    }

    char buffer[128];
    if (fread(buffer, 1, 128, fptr) != 128)
    {
        printf("ERROR: could not read ID3v1 tag block\n");
        return;
    }

    if (strncmp(buffer, "TAG", 3) != 0)
    {
        printf("ERROR: no ID3v1 tag found\n");
        return;
    }

    if (strcmp(flag, "-t") == 0)
        write_field(buffer, 3, 30, new_value);
    else if (strcmp(flag, "-a") == 0)
        write_field(buffer, 33, 30, new_value);
    else if (strcmp(flag, "-A") == 0)
        write_field(buffer, 63, 30, new_value);
    else if (strcmp(flag, "-y") == 0)
        write_field(buffer, 93, 4, new_value);
    else if (strcmp(flag, "-C") == 0)
    {
        write_field(buffer, 97, 28, new_value);
        buffer[125] = '\0';
    }
    else if (strcmp(flag, "-T") == 0)
    {
        int track = atoi(new_value);
        if (track < 1 || track > 255)
        {
            printf("ERROR: track number must be 1-255\n");
            return;
        }
        buffer[125] = '\0';
        buffer[126] = (unsigned char)track;
    }
    else if (strcmp(flag, "-g") == 0)
    {
        int genre = atoi(new_value);
        if (genre < 0 || genre > 255)
        {
            printf("ERROR: genre index must be 0-255\n");
            return;
        }
        buffer[127] = (unsigned char)genre;
    }
    else
    {
        printf("ERROR: unknown flag '%s'\n", flag);
        return;
    }

    fseek(fptr, -128, SEEK_END);
    if (fwrite(buffer, 1, 128, fptr) != 128)
        printf("ERROR: could not write updated ID3v1 tag\n");
    else
        printf("OK: ID3v1 tag updated\n");
}

/* Modify ID3v2 tag fields in-place */
static void modify_id3v2(FILE *fptr, const char *flag, const char *new_value)
{
    const char *target_frame = flag_to_frame(flag);
    if (target_frame == NULL)
    {
        printf("ERROR: unknown flag '%s'\n", flag);
        return;
    }

    unsigned char header[10];
    fseek(fptr, 0, SEEK_SET);
    if (fread(header, 1, 10, fptr) != 10)
    {
        printf("ERROR: could not read ID3v2 header\n");
        return;
    }

    unsigned long tag_size = parse_synchsafe(header + 6);

    char *tag_buffer = (char *)malloc(tag_size);
    if (tag_buffer == NULL)
    {
        printf("ERROR: memory allocation failed\n");
        return;
    }

    if (fread(tag_buffer, 1, tag_size, fptr) != tag_size)
    {
        printf("ERROR: could not read ID3v2 tag data\n");
        free(tag_buffer);
        return;
    }

    unsigned long offset = 0;
    int found = 0;

    while (offset < tag_size - 10)
    {
        if (tag_buffer[offset] == 0x00)
            break;

        char frame_id[5];
        memcpy(frame_id, tag_buffer + offset, 4);
        frame_id[4] = '\0';

        unsigned long frame_size = 0;
        frame_size |= ((unsigned long)(unsigned char)tag_buffer[offset + 4] << 24);
        frame_size |= ((unsigned long)(unsigned char)tag_buffer[offset + 5] << 16);
        frame_size |= ((unsigned long)(unsigned char)tag_buffer[offset + 6] <<  8);
        frame_size |=  (unsigned long)(unsigned char)tag_buffer[offset + 7];

        if (frame_size == 0 || offset + 10 + frame_size > tag_size)
            break;

        if (strcmp(frame_id, target_frame) == 0)
        {
            unsigned long new_len = strlen(new_value);

            if (strcmp(target_frame, "COMM") == 0)
            {
                /*
                 * COMM content: encoding(1) + language(3) + descriptor(\0) + text
                 * We keep the encoding and language intact, only replace text.
                 * text starts at tag_buffer + offset + 10 + 1 + 3 + 1 = offset + 15
                 * text capacity = frame_size - 5
                 */
                unsigned long text_capacity = frame_size - 5;
                if (new_len > text_capacity)
                {
                    printf("ERROR: value too long for COMM frame (max %lu chars)\n", text_capacity);
                    break;
                }
                /* Ensure language is set, zero descriptor, write text */
                if (tag_buffer[offset + 11] == '\0')
                    memcpy(tag_buffer + offset + 11, "eng", 3);
                tag_buffer[offset + 14] = '\0';
                memset(tag_buffer + offset + 15, 0, text_capacity);
                memcpy(tag_buffer + offset + 15, new_value, new_len);
            }
            else
            {
                /*
                 * Text frame content: encoding(1) + text
                 * text starts at tag_buffer + offset + 11
                 * text capacity = frame_size - 1
                 */
                unsigned long text_capacity = frame_size - 1;
                if (new_len > text_capacity)
                {
                    printf("ERROR: value too long for %s frame (max %lu chars)\n",
                           target_frame, text_capacity);
                    break;
                }
                tag_buffer[offset + 10] = 0x00; /* ISO-8859-1 encoding */
                memset(tag_buffer + offset + 11, 0, text_capacity);
                memcpy(tag_buffer + offset + 11, new_value, new_len);
            }

            found = 1;
            break;
        }

        offset += 10 + frame_size;
    }

    if (!found)
    {
        printf("ERROR: frame '%s' not found in ID3v2 tag\n", target_frame);
        free(tag_buffer);
        return;
    }

    /* Write modified tag buffer back (file position: right after the 10-byte header) */
    fseek(fptr, 10, SEEK_SET);
    if (fwrite(tag_buffer, 1, tag_size, fptr) != tag_size)
        printf("ERROR: could not write updated ID3v2 tag\n");
    else
        printf("OK: ID3v2 tag updated (%s)\n", target_frame);

    free(tag_buffer);
}

/* Check tag version and call appropriate modifier */
void modify_mp3_tag(char *flag, char *new_value, char *filepath)
{
    FILE *fptr = fopen(filepath, "r+b");
    if (fptr == NULL)
    {
        printf("ERROR: could not open file '%s'\n", filepath);
        return;
    }

    char sig[3];
    fseek(fptr, 0, SEEK_SET);
    fread(sig, 1, 3, fptr);

    if (strncmp(sig, "ID3", 3) == 0)
        modify_id3v2(fptr, flag, new_value);
    else
        modify_id3v1(fptr, flag, new_value);

    fclose(fptr);
}
