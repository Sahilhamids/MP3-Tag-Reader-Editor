#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/mp3.h"
#include "include/id3v1.h"
#include "include/id3v2.h"
#include "include/reader.h"

#define STATUS_ID3V2_FOUND  2
#define STATUS_ID3V1_FOUND  1
#define STATUS_NO_TAG       0

static void init_tag_info(tag_info *tagInfo)
{
    tagInfo->title = NULL;
    tagInfo->artist = NULL;
    tagInfo->album = NULL;
    tagInfo->comment = NULL;
    tagInfo->genre = NULL;
    tagInfo->year = 0;
    tagInfo->trackNumber = 0;
    strcpy(tagInfo->version, "Unknown");
}

status parse_mp3_metadata(const char *filename, tag_info *tagInfo)
{
    init_tag_info(tagInfo);

    FILE *fptr = fopen(filename, "rb");
    if (fptr == NULL)
    {
        printf("ERROR: could not open file '%s'\n", filename);
        return failure;
    }

    char header_sig[3];
    int has_id3v2 = 0;

    if (fread(header_sig, 1, 3, fptr) == 3 && strncmp(header_sig, "ID3", 3) == 0)
    {
        rewind(fptr);
        if (parse_id3v2(fptr, tagInfo) == success)
        {
            has_id3v2 = 1;
        }
    }

    /* Fallback to ID3v1 if no ID3v2 tag is present */
    if (!has_id3v2)
    {
        parse_id3v1(fptr, tagInfo);
    }

    fclose(fptr);
    return success;
}

int inspect_file_boundaries(FILE *fptr)
{
    fseek(fptr, 0, SEEK_SET);

    char tag[4];
    fread(tag, 3, 1, fptr);
    tag[3] = '\0';

    if (strcmp(tag, "ID3") == 0)
    {
        return STATUS_ID3V2_FOUND;
    }

    fseek(fptr, -128, SEEK_END);
    fread(tag, 3, 1, fptr);
    tag[3] = '\0';

    if (strcmp(tag, "TAG") == 0)
    {
        return STATUS_ID3V1_FOUND;
    }

    printf("ERROR: no recognised ID3 tag found\n");
    return STATUS_NO_TAG;
}
