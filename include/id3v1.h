#ifndef ID3V1_H
#define ID3V1_H

#include <stdio.h>
#include "mp3.h"

status parse_id3v1(FILE *fptr, tag_info *tagInfo);
void extract_string(const char *src, char *dest, int len);

#endif
