#ifndef ID3V2_H
#define ID3V2_H

#include <stdio.h>
#include "mp3.h"

status parse_id3v2(FILE *fptr, tag_info *tagInfo);

#endif
