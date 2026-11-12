#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/mp3.h"
#include "include/id3v1.h"

#define TOTAL_GENRES 80

static const char *genre_lookup_table[TOTAL_GENRES] = {
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge", "Hip-Hop",
    "Jazz", "Metal", "New Age", "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae",
    "Rock", "Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks",
    "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk", "Fusion",
    "Trance", "Classical", "Instrumental", "Acid", "House", "Game", "Sound Clip",
    "Gospel", "Noise", "AlternRock", "Bass", "Soul", "Punk", "Space", "Meditative",
    "Instrumental Pop", "Instrumental Rock", "Ethnic", "Gothic", "Darkwave",
    "Techno-Industrial", "Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock",
    "Comedy", "Cult", "Gangsta", "Top 40", "Christian Rap", "Pop/Funk", "Jungle",
    "Native American", "Cabaret", "New Wave", "Psychadelic", "Rave", "Showtunes",
    "Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro",
    "Musical", "Rock & Roll", "Hard Rock"
};

void extract_string(const char *src, char *dest, int len)
{
    memcpy(dest, src, len);
    dest[len] = '\0';

    int i = len - 1;
    while (i >= 0 && (dest[i] == ' ' || dest[i] == '\0'))
    {
        dest[i] = '\0';
        i--;
    }
}

status parse_id3v1(FILE *fptr, tag_info *tagInfo)
{
    if (fseek(fptr, -128, SEEK_END) != 0)
    {
        return failure;
    }

    char buffer[128];
    if (fread(buffer, 1, 128, fptr) != 128)
    {
        printf("ERROR: failed to read ID3v1 tag block\n");
        return failure;
    }

    if (strncmp(buffer, "TAG", 3) != 0)
    {
        return failure;
    }

    if (tagInfo->title == NULL)
    {
        tagInfo->title  = (char *)malloc(31);
        extract_string(buffer + 3,  tagInfo->title,  30);
    }

    if (tagInfo->artist == NULL)
    {
        tagInfo->artist = (char *)malloc(31);
        extract_string(buffer + 33, tagInfo->artist, 30);
    }

    if (tagInfo->album == NULL)
    {
        tagInfo->album  = (char *)malloc(31);
        extract_string(buffer + 63, tagInfo->album,  30);
    }

    if (tagInfo->year == 0)
    {
        char year_str[5];
        extract_string(buffer + 93, year_str, 4);
        tagInfo->year = (unsigned short)atoi(year_str);
    }

    if (buffer[125] == '\0' && buffer[126] != 0)
    {
        if (tagInfo->comment == NULL)
        {
            tagInfo->comment = (char *)malloc(29);
            extract_string(buffer + 97, tagInfo->comment, 28);
        }
        if (tagInfo->trackNumber == 0)
            tagInfo->trackNumber = (unsigned char)buffer[126];
        if (strcmp(tagInfo->version, "Unknown") == 0)
            strcpy(tagInfo->version, "ID3v1.1");
    }
    else
    {
        if (tagInfo->comment == NULL)
        {
            tagInfo->comment = (char *)malloc(31);
            extract_string(buffer + 97, tagInfo->comment, 30);
        }
        if (strcmp(tagInfo->version, "Unknown") == 0)
            strcpy(tagInfo->version, "ID3v1");
    }

    if (tagInfo->genre == NULL)
    {
        unsigned char genre_idx = (unsigned char)buffer[127];
        tagInfo->genre = (char *)malloc(32);
        if (genre_idx < TOTAL_GENRES)
            strcpy(tagInfo->genre, genre_lookup_table[genre_idx]);
        else
            strcpy(tagInfo->genre, "Unknown");
    }

    return success;
}
