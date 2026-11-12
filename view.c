#include <stdlib.h>
#include <stdio.h>
#include "include/mp3.h"
#include "include/reader.h"

void print_metadata(const tag_info *tagInfo);
void free_tag_info(tag_info *tagInfo);

void view_mp3_tags(char *filepath)
{
    tag_info tagInfo;
    if (parse_mp3_metadata(filepath, &tagInfo) == failure)
    {
        printf("ERROR: could not parse '%s'\n", filepath);
        return;
    }
    print_metadata(&tagInfo);
    free_tag_info(&tagInfo);
}

void print_metadata(const tag_info *tagInfo)
{
    printf("\n========================================\n");
    printf("          MP3 TAG VIEWER                \n");
    printf("========================================\n");
    printf(" Version      : %s\n", tagInfo->version[0] != '\0' ? tagInfo->version : "None");
    printf("----------------------------------------\n");

    printf(" Title        : %s\n", (tagInfo->title   && tagInfo->title[0])   ? tagInfo->title   : "[Unknown]");
    printf(" Artist       : %s\n", (tagInfo->artist  && tagInfo->artist[0])  ? tagInfo->artist  : "[Unknown]");
    printf(" Album        : %s\n", (tagInfo->album   && tagInfo->album[0])   ? tagInfo->album   : "[Unknown]");

    if (tagInfo->year > 0)
        printf(" Year         : %u\n", tagInfo->year);
    else
        printf(" Year         : [Unknown]\n");

    if (tagInfo->trackNumber > 0)
        printf(" Track        : %u\n", tagInfo->trackNumber);
    else
        printf(" Track        : N/A\n");

    printf(" Genre        : %s\n", (tagInfo->genre   && tagInfo->genre[0])   ? tagInfo->genre   : "[Unknown]");
    printf(" Comment      : %s\n", (tagInfo->comment && tagInfo->comment[0]) ? tagInfo->comment : "[None]");

    printf("========================================\n\n");
}

void free_tag_info(tag_info *tagInfo)
{
    if (tagInfo == NULL)
        return;

    free(tagInfo->title);   tagInfo->title   = NULL;
    free(tagInfo->artist);  tagInfo->artist  = NULL;
    free(tagInfo->album);   tagInfo->album   = NULL;
    free(tagInfo->comment); tagInfo->comment = NULL;
    free(tagInfo->genre);   tagInfo->genre   = NULL;
}
