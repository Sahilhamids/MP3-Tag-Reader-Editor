# MP3 Tag Reader - Project Notes

## Table of Contents
1. [What This Project Does](#what-this-project-does)
2. [File Structure](#file-structure)
3. [MP3 File Format Overview](#mp3-file-format-overview)
4. [ID3v1 Tag Format](#id3v1-tag-format)
5. [ID3v2 Tag Format](#id3v2-tag-format)
6. [Code Structure and Data Types](#code-structure-and-data-types)
7. [File-by-File Breakdown](#file-by-file-breakdown)
8. [How Parsing Works](#how-parsing-works)
9. [How Modify Works](#how-modify-works)
10. [Build and Usage](#build-and-usage)

---

## What This Project Does

This is a command-line C program that reads and writes **ID3 metadata tags** embedded inside `.mp3` files. These tags store information like the song title, artist name, album, year, genre, and comment (the data you see in a music player).

The program supports:
- **Viewing** tags from any MP3 file (ID3v1, ID3v1.1, ID3v2.3, ID3v2.4)
- **Modifying** tags in-place without touching the audio data

---

## File Structure

```
MP3TagReader/
├── main.c          - Entry point, argument parsing, mode selection
├── reader.c        - Opens the file, detects tag version, calls parsers
├── id3v1.c         - Parses the 128-byte ID3v1/v1.1 tag block
├── id3v2.c         - Parses the variable-length ID3v2 tag block
├── view.c          - Formats and prints tag data to the terminal
├── modify.c        - Writes updated tag fields back into the file
└── include/
    ├── mp3.h       - Shared types: status, mode, tag_info, apic
    ├── id3v1.h     - Declares parse_id3v1, extract_string
    ├── id3v2.h     - Declares parse_id3v2
    ├── reader.h    - Declares parse_mp3_metadata, inspect_file_boundaries
    └── modify.h    - Declares modify_mp3_tag
```

---

## MP3 File Format Overview

An MP3 file has two parts:

```
+---------------------------+
|   ID3v2 Tag (optional)    |  <-- at the very START of the file
+---------------------------+
|                           |
|   Audio Data (MP3 frames) |  <-- the actual music
|                           |
+---------------------------+
|   ID3v1 Tag (optional)    |  <-- last 128 bytes at the END of the file
+---------------------------+
```

- **ID3v1** came first (1996). Fixed size, 128 bytes, very limited fields.
- **ID3v2** came later (1998). Variable size, sits at the front, supports much more data.
- A file can have both, one, or neither.
- When both exist, ID3v2 generally takes priority since it has more detail.

---

## ID3v1 Tag Format

ID3v1 is always the **last 128 bytes** of the file.

```
Offset  Size   Field
------  ----   -----
0       3      Signature: "TAG"
3       30     Title
33      30     Artist
63      30     Album
93      4      Year (ASCII, e.g. "2003")
97      30     Comment (ID3v1.0) OR 28 bytes comment + separator (ID3v1.1)
125     1      Zero byte -- if this is 0x00, it signals ID3v1.1
126     1      Track number (ID3v1.1 only, otherwise part of comment)
127     1      Genre index (integer, maps to a fixed genre name table)
```

**ID3v1 vs ID3v1.1:**
The only difference is bytes 125-126. In v1.0, the comment field runs all 30 bytes (97-126). In v1.1, byte 125 is forced to 0x00 and byte 126 holds a track number. When reading, check: if `buffer[125] == 0` and `buffer[126] != 0`, it is v1.1.

**Genre:**
Genre is stored as a single byte (0-79). There is a standard lookup table of 80 genre names (Blues, Rock, Pop, etc.). The byte is an index into that table.

**String fields:**
All string fields are fixed-width and padded with spaces or null bytes if the actual value is shorter. When reading them, you strip trailing spaces and null bytes from the right.

---

## ID3v2 Tag Format

ID3v2 sits at the **beginning** of the file and has a header followed by a series of "frames".

### ID3v2 Header (10 bytes)

```
Byte    Size   Field
----    ----   -----
0-2     3      Signature: "ID3"
3       1      Version (3 = ID3v2.3, 4 = ID3v2.4)
4       1      Revision
5       1      Flags
6-9     4      Tag size (synchsafe integer)
```

### Synchsafe Integer

The tag size is NOT a regular 32-bit integer. It is a "synchsafe" integer where the most significant bit of each byte is always 0. This avoids the byte pattern 0xFF 0xFF that MP3 audio uses for frame sync.

Decoding:
```
size = (byte[0] & 0x7F) << 21
     | (byte[1] & 0x7F) << 14
     | (byte[2] & 0x7F) <<  7
     | (byte[3] & 0x7F)
```

This gives you the number of bytes that follow the 10-byte header (the total frame data area).

### ID3v2 Frames

After the 10-byte header, the rest of the tag is a sequence of frames. Each frame has its own 10-byte header followed by content:

```
Offset  Size   Field
------  ----   -----
0-3     4      Frame ID (e.g. "TIT2", "TPE1", "TALB")
4-7     4      Frame size (regular big-endian 32-bit integer, NOT synchsafe)
8-9     2      Flags (usually 0x00 0x00, ignored for basic use)
10      1      Text encoding (0x00 = ISO-8859-1, 0x01 = UTF-16)
11+     N      Text content (length = frame_size - 1)
```

**Common Frame IDs:**

| Frame ID | Field        |
|----------|--------------|
| TIT2     | Title        |
| TPE1     | Artist       |
| TALB     | Album        |
| TYER     | Year (v2.3)  |
| TDRC     | Year (v2.4)  |
| TRCK     | Track number |
| TCON     | Genre        |
| COMM     | Comment      |

**COMM frame is special:** Its content is structured differently from text frames:
```
Byte 0    : Encoding
Bytes 1-3 : Language code (e.g. "eng")
Byte 4    : Content descriptor (null-terminated, usually just 0x00)
Byte 5+   : Actual comment text
```

**Padding:**
After all frames, there may be zero bytes (padding) before the audio data starts. When scanning frames, if you read a 0x00 byte at the start of a frame ID, you have hit the padding and must stop.

---

## Code Structure and Data Types

### `mp3.h` -- Shared Types

**`status` enum:**
```c
typedef enum { success, failure } status;
```
Used as a return type for functions that can fail (like file I/O). Cleaner than using -1/0 everywhere.

**`mode` enum:**
```c
typedef enum { HELP, MODIFY, VIEW } mode;
```
Represents which operation the user requested from the command line.

**`apic` struct:**
Holds information about embedded album art (not fully implemented, reserved for future use).

**`tag_info` struct:**
```c
typedef struct _tag_info {
    char version[50];       // "ID3v1", "ID3v1.1", "ID3v2.3", "ID3v2.4"
    char *title;            // heap-allocated string
    char *artist;           // heap-allocated string
    char *album;            // heap-allocated string
    unsigned short year;    // numeric year (e.g. 2003)
    unsigned short trackNumber;
    char *genre;            // heap-allocated string
    char *comment;          // heap-allocated string
    apic APIC;
} tag_info;
```

String fields are `char *` (heap pointers) because their length varies. They are set to `NULL` by default and must be `free()`d after use.

---

## File-by-File Breakdown

### `main.c`

Entry point. Handles all command-line argument parsing.

**`main(argc, argv)`**
- Checks that at least one argument was passed
- Figures out which mode to run based on `argv[1]`:
  - `-h` or `--help` -> HELP
  - `-v` or a `.mp3` filename -> VIEW
  - A modify flag (`-t`, `-a`, etc.) -> MODIFY
- Dispatches to the right function based on mode

**`is_mp3_file_path(str)`**
- Uses `strrchr` to find the last `.` in the string
- Checks if the extension is `.mp3`
- Returns 1 (true) or 0 (false)

**`is_modify_flag(str)`**
- Checks if the string is one of the known flags: `-t -a -A -y -T -C -g`
- Returns 1 or 0

**`print_help()`**
- Prints usage instructions and the list of flags to stdout

---

### `reader.c`

Coordinates the full parsing process for a given file.

**`parse_mp3_metadata(filename, tagInfo)`**
- Initialises the `tag_info` struct to safe defaults (all NULLs, year=0)
- Opens the file in binary read mode (`"rb"`)
- Reads the first 3 bytes and checks for `"ID3"` signature
  - If found: rewinds, calls `parse_id3v2()`
- If no ID3v2 tag is present, it calls `parse_id3v1()` as fallback
- Closes the file and returns `success`

**`inspect_file_boundaries(fptr)`**
- Reads the first 3 bytes to check for ID3v2 (`"ID3"`)
- If not found, seeks to -128 from the end and checks for ID3v1 (`"TAG"`)
- Returns `STATUS_ID3V2_FOUND`, `STATUS_ID3V1_FOUND`, or `STATUS_NO_TAG`
- Utility function -- not used in the main flow but useful for debugging

---

### `id3v1.c`

Parses the fixed 128-byte ID3v1 tag block at the end of the file.

**`extract_string(src, dest, len)`**
- Copies exactly `len` bytes from `src` into `dest` using `memcpy`
- Adds a null terminator at `dest[len]`
- Strips trailing spaces and null bytes from the right by walking back from the end
- Used by both `id3v1.c` and `id3v2.c` (via `id3v1.h`)

**`parse_id3v1(fptr, tagInfo)`**
1. Seeks to -128 bytes from end of file (`fseek(fptr, -128, SEEK_END)`)
2. Reads 128 bytes into a local buffer
3. Checks the first 3 bytes for `"TAG"` -- returns `failure` if not found
4. Allocates memory for title, artist, album (31 bytes each -- 30 chars + null)
5. Calls `extract_string` on the correct byte offsets to fill those fields
6. Reads the year field (bytes 93-96) as a 4-character ASCII string, converts with `atoi`
7. Checks bytes 125-126 to decide between ID3v1 and ID3v1.1:
   - v1.1: allocates 29-byte comment, reads 28 bytes, picks up track from byte 126
   - v1.0: allocates 31-byte comment, reads 30 bytes, sets trackNumber = 0
8. Reads byte 127 as an unsigned char genre index, looks up the genre name string from the table
9. Returns `success`

---

### `id3v2.c`

Parses the variable-length ID3v2 tag at the start of the file.

**`parse_synchsafe(bytes)`**
- Decodes a 4-byte synchsafe integer
- Masks off the top bit of each byte with `& 0x7F` and shifts into position
- Returns the actual size as `unsigned long`

**`parse_id3v2(fptr, tagInfo)`**
1. Seeks to byte 3 (skipping `"ID3"`) and reads 1 byte -- the version number
   - Sets `tagInfo->version` to `"ID3v2.3"` or `"ID3v2.4"` accordingly
2. Seeks to byte 6 and reads 4 bytes -- the synchsafe tag size
3. Calls `parse_synchsafe` to decode the size
4. Allocates a heap buffer of that size with `malloc`
5. Reads the entire tag data into that buffer (this is the frame data, starting right after the 10-byte header)
6. Enters a loop scanning frames:
   - Reads 4-byte frame ID into a string
   - Reads 4-byte big-endian frame size
   - Skips 2 flag bytes
   - Skips the encoding byte (byte 10 of the frame), gets a pointer to the content
   - Matches the frame ID to a known field and fills the appropriate `tag_info` field:
     - `TIT2` -> title
     - `TPE1` -> artist
     - `TALB` -> album
     - `TYER` / `TDRC` -> year
     - `TRCK` -> track number
     - `TCON` -> genre
     - `COMM` -> comment (skips language and content descriptor)
   - Advances the offset by `frame_size` to reach the next frame
   - Stops if a 0x00 byte is found (padding) or the buffer boundary is exceeded
7. Frees the buffer and returns `success`

---

### `view.c`

Handles display output and memory cleanup.

**`view_mp3_tags(filepath)`**
- Declares a local `tag_info` struct on the stack
- Calls `parse_mp3_metadata` to fill it
- Calls `print_metadata` to display it
- Calls `free_tag_info` to release all heap-allocated strings

**`print_metadata(tagInfo)`**
- Prints a formatted table to stdout
- For each string field: checks if the pointer is non-NULL and the first character is not `\0` before printing, otherwise prints a placeholder like `[Unknown]`
- Year and track number are numeric -- prints `[Unknown]` or `N/A` if zero

**`free_tag_info(tagInfo)`**
- Calls `free()` on each heap-allocated string field (title, artist, album, comment, genre)
- Sets each pointer back to `NULL` after freeing to prevent double-free bugs
- Does nothing for `year` and `trackNumber` -- they are stored directly in the struct (not heap-allocated)

---

### `modify.c`

Writes updated tag data back into the file in-place.

**`flag_to_frame(flag)`**
- Maps a CLI flag string to its corresponding ID3v2 frame ID string
- `-t` -> `"TIT2"`, `-a` -> `"TPE1"`, `-A` -> `"TALB"`, etc.
- Returns `NULL` for unrecognised flags

**`write_field(buffer, offset, max_len, value)`**
- Helper used by the ID3v1 modifier
- Zeroes out `max_len` bytes at `buffer + offset` with `memset`
- Copies the new value in with `memcpy`, capped at `max_len`

**`modify_id3v1(fptr, flag, new_value)`**
- Seeks to -128 from end, reads the 128-byte block
- Verifies the `"TAG"` signature
- Based on the flag, writes the new value into the correct byte offset using `write_field`:
  - `-t` -> offset 3, 30 bytes
  - `-a` -> offset 33, 30 bytes
  - `-A` -> offset 63, 30 bytes
  - `-y` -> offset 93, 4 bytes
  - `-C` -> offset 97, 28 bytes (also sets byte 125 to 0x00 for v1.1 format)
  - `-T` -> converts value with `atoi`, writes byte at offset 126, sets byte 125 to 0x00
  - `-g` -> converts value with `atoi`, writes genre index byte at offset 127
- Seeks back to -128 and writes the full 128-byte buffer back
- The audio data is never touched

**`modify_id3v2(fptr, flag, new_value)`**
- Reads the 10-byte header to get the tag size
- Loads the entire tag into a heap buffer
- Scans frame by frame looking for the frame ID that matches the flag
- When the matching frame is found:
  - For regular text frames: zeroes the content area, copies the new value starting at offset 11 within the frame (after the encoding byte). Sets encoding byte to 0x00 (ISO-8859-1)
  - For COMM frames: keeps encoding and language bytes intact, zeroes from the descriptor byte onward, writes new text starting at offset 15
  - Checks that the new value fits within the existing frame size -- reports an error if too long (no frame resizing is done)
- Seeks to byte 10 (right after the header) and writes the modified buffer back
- Frees the buffer

**`modify_mp3_tag(flag, new_value, filepath)`**
- Opens the file in `"r+b"` mode (read+write, binary, no truncation)
- Reads the first 3 bytes to detect the tag type
- If `"ID3"` is found -> calls `modify_id3v2`
- Otherwise -> calls `modify_id3v1`
- Closes the file

---

## How Parsing Works

### Reading an ID3v1 tag step by step

```
File on disk:
[ ... audio data ... ][ TAG | title(30) | artist(30) | album(30) | year(4) | comment(28/30) | \0 | track | genre ]
                       ^--- -128 from end of file
```

1. `fseek(fptr, -128, SEEK_END)` -- jump to 128 bytes before EOF
2. `fread(buffer, 1, 128, fptr)` -- load the whole block
3. `strncmp(buffer, "TAG", 3)` -- confirm signature
4. Copy sub-sections with `extract_string` using fixed offsets

### Reading an ID3v2 tag step by step

```
File on disk:
[ I D 3 | ver | rev | flags | size(4) | frame1 | frame2 | ... | padding | audio data ]
  byte 0                      byte 6    byte 10
```

1. Read bytes 0-2 -> check for `"ID3"`
2. Read byte 3 -> version number
3. Read bytes 6-9 -> decode synchsafe size -> N bytes of tag data follow
4. Read N bytes into a buffer
5. Walk the buffer frame by frame:
   - Bytes 0-3 of frame = ID string
   - Bytes 4-7 of frame = content size (how many bytes follow the 10-byte frame header)
   - Byte 10 of frame = encoding indicator (skip it)
   - Bytes 11+ of frame = the actual text
6. Stop when hitting a 0x00 byte (padding)

---

## How Modify Works

### Key Principle

The file is opened in `"r+b"` mode -- read and write, binary, without truncating the file. This lets us overwrite specific bytes while leaving everything else untouched.

### ID3v1 Modify

Since the tag is always exactly 128 bytes at a fixed position, modification is straightforward:
1. Read the full 128-byte block into memory
2. Overwrite just the relevant bytes in memory
3. Seek back to the same position
4. Write the entire 128-byte block back

The audio data is at bytes 0 to `filesize - 128` and is never read or written.

### ID3v2 Modify

ID3v2 is trickier because frames are variable-length. The approach used here is **in-place modification**:
1. Read the entire tag block into memory (size known from the synchsafe header)
2. Scan to find the target frame
3. Overwrite the content area of that frame in the buffer
4. Write the entire tag buffer back to the file at byte offset 10

This only works when the new value fits within the existing frame size. If the new value were longer than the original, the frame would need to grow, which would shift all subsequent frames and invalidate the tag. That case is not supported -- the user gets an error message.

---

## Build and Usage

### Compile

```bash
gcc -Wall -Wextra -o mp3reader main.c reader.c id3v1.c id3v2.c view.c modify.c
```

### View tags

```bash
./mp3reader song.mp3
./mp3reader -v song.mp3
```

### Modify a tag

```bash
./mp3reader -t "Bohemian Rhapsody" song.mp3    # set title
./mp3reader -a "Queen"             song.mp3    # set artist
./mp3reader -A "A Night at Opera"  song.mp3    # set album
./mp3reader -y "1975"              song.mp3    # set year
./mp3reader -T "1"                 song.mp3    # set track number
./mp3reader -C "Greatest ever"     song.mp3    # set comment
./mp3reader -g "17"                song.mp3    # set genre (index 17 = Rock)
```

### Help

```bash
./mp3reader -h
./mp3reader --help
```

