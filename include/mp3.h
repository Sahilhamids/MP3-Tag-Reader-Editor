#ifndef MP3_H
#define MP3_H

typedef enum
{
    success,
    failure
} status;

typedef enum
{
    HELP,
    MODIFY,
    VIEW
} mode;

typedef struct _apic
{
    char img_type[50];
    unsigned long img_size;
    char img_path_description[100];
} apic;

typedef struct _tag_info
{
    char version[50];
    char *title;
    char *artist;
    char *album;
    unsigned short year;
    unsigned short trackNumber;
    char *genre;
    char *comment;
    apic APIC;
} tag_info;

void view_mp3_tags(char *filepath);
void modify_mp3_tag(char *flag, char *new_value, char *filepath);

#endif
