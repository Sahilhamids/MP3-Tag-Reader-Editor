#ifndef READER_H
#define READER_H

#include <stdio.h>
#include "mp3.h"

status parse_mp3_metadata(const char *filename, tag_info *tagInfo);
int inspect_file_boundaries(FILE *fptr);

#endif
