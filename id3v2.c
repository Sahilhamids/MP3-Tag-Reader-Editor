#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/mp3.h"
#include "include/id3v1.h"
#include "include/id3v2.h"

static unsigned long parse_synchsafe(const unsigned char *bytes)
{
    unsigned long size = 0;
    size  = ((unsigned long)(bytes[0] & 0x7F) << 21);
    size |= ((unsigned long)(bytes[1] & 0x7F) << 14);
    size |= ((unsigned long)(bytes[2] & 0x7F) << 7);
    size |=  (unsigned long)(bytes[3] & 0x7F);
    return size;
}

/*
 * ID3v2 header layout (10 bytes total):
 *   Bytes 0-2 : "ID3"
 *   Byte  3   : Version (e.g. 3 = ID3v2.3)
 *   Byte  4   : Revision
 *   Byte  5   : Flags
 *   Bytes 6-9 : Tag size as a 4-byte synchsafe integer
 */
status parse_id3v2(FILE *fptr, tag_info *tagInfo)
{
    if (fseek(fptr, 3, SEEK_SET) != 0)
    {
        return failure;
    }

    unsigned char version;
    if (fread(&version, 1, 1, fptr) != 1)
    {
        return failure;
    }

    if (version == 3)
    {
        strcpy(tagInfo->version, "ID3v2.3");
    }
    else if (version == 4)
    {
        strcpy(tagInfo->version, "ID3v2.4");
    }

    if (fseek(fptr, 6, SEEK_SET) != 0)
    {
        printf("ERROR: could not seek to tag size field\n");
        return failure;
    }

    unsigned char size_bytes[4];
    if (fread(size_bytes, 1, 4, fptr) != 4)
    {
        printf("ERROR: could not read tag size\n");
        return failure;
    }

    unsigned long tag_size = parse_synchsafe(size_bytes);

    char *tag_buffer = (char *)malloc(tag_size);
    if (tag_buffer == NULL)
    {
        printf("ERROR: memory allocation failed\n");
        return failure;
    }

    if (fread(tag_buffer, 1, tag_size, fptr) != tag_size)
    {
        printf("ERROR: could not read tag data\n");
        free(tag_buffer);
        return failure;
    }

    unsigned long offset = 0;

    while (offset < tag_size - 10)
    {
        if (tag_buffer[offset] == 0x00)
        {
            break;
        }

        char frame_id[5];
        memcpy(frame_id, tag_buffer + offset, 4);
        frame_id[4] = '\0';
        offset += 4;

        unsigned long frame_size = 0;
        frame_size |= ((unsigned long)(unsigned char)tag_buffer[offset]     << 24);
        frame_size |= ((unsigned long)(unsigned char)tag_buffer[offset + 1] << 16);
        frame_size |= ((unsigned long)(unsigned char)tag_buffer[offset + 2] << 8);
        frame_size |=  (unsigned long)(unsigned char)tag_buffer[offset + 3];
        offset += 4;

        offset += 2; /* skip frame flags */

        if (frame_size == 0 || offset + frame_size > tag_size)
        {
            break;
        }

        /* first byte of frame content is the text encoding indicator */
        char *content_ptr = tag_buffer + offset + 1;
        unsigned long content_len = frame_size - 1;

        if (strcmp(frame_id, "TIT2") == 0)
        {
            tagInfo->title = (char *)malloc(content_len + 1);
            extract_string(content_ptr, tagInfo->title, content_len);
        }
        else if (strcmp(frame_id, "TPE1") == 0)
        {
            tagInfo->artist = (char *)malloc(content_len + 1);
            extract_string(content_ptr, tagInfo->artist, content_len);
        }
        else if (strcmp(frame_id, "TALB") == 0)
        {
            tagInfo->album = (char *)malloc(content_len + 1);
            extract_string(content_ptr, tagInfo->album, content_len);
        }
        else if (strcmp(frame_id, "TYER") == 0 || strcmp(frame_id, "TDRC") == 0)
        {
            char year_str[5];
            extract_string(content_ptr, year_str, 4);
            tagInfo->year = (unsigned short)atoi(year_str);
        }
        else if (strcmp(frame_id, "TRCK") == 0)
        {
            char track_str[16];
            extract_string(content_ptr, track_str, content_len < 15 ? content_len : 15);
            tagInfo->trackNumber = (unsigned short)atoi(track_str);
        }
        else if (strcmp(frame_id, "TCON") == 0)
        {
            tagInfo->genre = (char *)malloc(content_len + 1);
            extract_string(content_ptr, tagInfo->genre, content_len);
        }
        else if (strcmp(frame_id, "COMM") == 0)
        {
            if (content_len > 3)
            {
                const char *text_start = content_ptr + 3;
                unsigned long text_len = content_len - 3;
                const char *desc_end = (const char *)memchr(text_start, '\0', text_len);
                if (desc_end != NULL && desc_end < content_ptr + content_len - 1)
                {
                    text_start = desc_end + 1;
                    text_len = (content_ptr + content_len) - text_start;
                }
                tagInfo->comment = (char *)malloc(text_len + 1);
                extract_string(text_start, tagInfo->comment, text_len);
            }
        }

        offset += frame_size;
    }

    free(tag_buffer);
    return success;
}
