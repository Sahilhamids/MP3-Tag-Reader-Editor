#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "include/mp3.h"
#include "include/reader.h"

int is_modify_flag(char *str);
int is_mp3_file_path(char *str);
void print_help(void);

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("ERROR: insufficient arguments passed. Use -h for help\n");
        return 1;
    }

    int choice = -1;

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
        choice = HELP;
    }
    else if (strcmp(argv[1], "-v") == 0 || is_mp3_file_path(argv[1]))
    {
        choice = VIEW;
    }
    else if (is_modify_flag(argv[1]))
    {
        choice = MODIFY;
    }

    switch (choice)
    {
        case HELP:
            print_help();
            break;

        case VIEW:
            if (argc == 2)
            {
                view_mp3_tags(argv[1]);
            }
            else if (argc == 3 && is_mp3_file_path(argv[2]))
            {
                view_mp3_tags(argv[2]);
            }
            else
            {
                printf("ERROR: invalid arguments for view mode. Use -v <file.mp3>\n");
                return 1;
            }
            break;

        case MODIFY:
            if (argc == 4 && is_mp3_file_path(argv[3]))
            {
                modify_mp3_tag(argv[1], argv[2], argv[3]);
            }
            else
            {
                printf("ERROR: invalid arguments for modify mode. Use <flag> <value> <file.mp3>\n");
                return 1;
            }
            break;

        default:
            printf("ERROR: unknown option '%s'. Use -h for help\n", argv[1]);
            return 1;
    }

    return 0;
}

int is_modify_flag(char *str)
{
    if (strcmp(str, "-t") == 0 || strcmp(str, "-a") == 0 ||
        strcmp(str, "-A") == 0 || strcmp(str, "-y") == 0 ||
        strcmp(str, "-T") == 0 || strcmp(str, "-C") == 0 ||
        strcmp(str, "-g") == 0)
    {
        return 1;
    }
    return 0;
}

int is_mp3_file_path(char *str)
{
    char *ptr = strrchr(str, '.');
    if (ptr != NULL && strcmp(ptr, ".mp3") == 0)
    {
        return 1;
    }
    return 0;
}

void print_help(void)
{
    printf("\nUsage:\n");
    printf("  ./mp3reader <file.mp3>               View tags\n");
    printf("  ./mp3reader -v <file.mp3>             View tags\n");
    printf("  ./mp3reader <flag> <value> <file.mp3> Modify a tag\n");
    printf("\nModify flags:\n");
    printf("  -t <title>    Set title\n");
    printf("  -a <artist>   Set artist\n");
    printf("  -A <album>    Set album\n");
    printf("  -y <year>     Set year\n");
    printf("  -T <track>    Set track number\n");
    printf("  -C <comment>  Set comment\n");
    printf("  -g <genre>    Set genre\n");
    printf("\n");
}
