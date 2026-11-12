# MP3 Tag Reader and Editor

A command-line tool written in C to view and edit ID3 metadata tags (both ID3v1 and ID3v2) in MP3 audio files. This project was developed as part of Advanced C Programming coursework to practice low-level binary file handling, bitwise operations, memory management, and structured program design.

---

## Features

- **View Mode**:
  - Auto-detects tag version (`ID3v2.3`, `ID3v2.4`, `ID3v1`, `ID3v1.1`).
  - Displays Title, Artist, Album, Year, Track Number, Genre, and Comments.
  - Prioritizes modern ID3v2 tags and seamlessly falls back to legacy ID3v1 tags.

- **Edit / Modify Mode**:
  - Updates metadata tags directly in-place without corrupting the underlying audio data.
  - Supports editing Title, Artist, Album, Year, Track Number, Comment, and Genre.
  - Works with both ID3v1 (fixed 128-byte block) and ID3v2 (tag header and frames).

---

## Technical Concepts Applied

1. **Binary File I/O**:
   - Using `fopen`, `fseek`, `fread`, and `fwrite` in binary modes (`"rb"`, `"r+b"`).
   - Reading fixed structures at negative offsets (`SEEK_END - 128`) for ID3v1 epilogue tags.
   - Parsing variable-length headers and frames at the start of the file (`SEEK_SET`) for ID3v2.

2. **Bitwise Manipulation**:
   - Decoding 4-byte **synchsafe integers** (7 bits per byte, MSB masked to avoid audio synchronization false positives) using bit shifts and masks:
     `size = ((b[0] & 0x7F) << 21) | ((b[1] & 0x7F) << 14) | ((b[2] & 0x7F) << 7) | (b[3] & 0x7F)`
   - Assembling big-endian 32-bit frame lengths from byte streams.

3. **Dynamic Memory & Pointers**:
   - Dynamic buffer allocation for variable-length frame content.
   - Safe string extraction, trailing space/null-padding stripping, and memory cleanup (`free()`).

4. **Modular Architecture**:
   - Independent modules separated cleanly into header files (`include/`) and implementation files (`.c`).

---

## Project Structure

```
MP3TagReader/
├── main.c              # Command-line interface and argument dispatcher
├── reader.c            # File validation and metadata parser orchestrator
├── id3v1.c             # ID3v1 / ID3v1.1 parser and genre table
├── id3v2.c             # ID3v2.3 / ID3v2.4 frame parser and synchsafe decoder
├── view.c              # Formatted terminal output and memory deallocation
├── modify.c            # In-place tag modification for both versions
├── include/
│   ├── mp3.h           # Core structs (tag_info) and common enums
│   ├── id3v1.h         # ID3v1 prototypes
│   ├── id3v2.h         # ID3v2 prototypes
│   ├── reader.h        # Reader prototypes
│   └── modify.h        # Tag modifier prototypes
└── NOTES.md            # Detailed technical documentation and spec notes
```

---

## Supported ID3 Frames (ID3v2)

| Flag | Frame ID | Description |
|------|----------|-------------|
| `-t` | `TIT2`   | Song Title  |
| `-a` | `TPE1`   | Artist      |
| `-A` | `TALB`   | Album       |
| `-y` | `TYER` / `TDRC` | Year / Recording Time |
| `-T` | `TRCK`   | Track Number|
| `-g` | `TCON`   | Content / Genre |
| `-C` | `COMM`   | Comment     |

---

## Compilation

Compile using GCC with warnings enabled:

```bash
gcc -Wall -Wextra -o mp3reader main.c reader.c id3v1.c id3v2.c view.c modify.c
```

*(On Windows, you can name the output `mp3reader.exe`)*

---

## Usage

### 1. View Metadata
```bash
./mp3reader song.mp3
# or
./mp3reader -v song.mp3
```

**Sample Output:**
```
========================================
          MP3 TAG VIEWER                
========================================
 Version      : ID3v2.3
----------------------------------------
 Title        : Bohemian Rhapsody
 Artist       : Queen
 Album        : A Night at the Opera
 Year         : 1975
 Track        : 11
 Genre        : Classic Rock
 Comment      : Remastered
========================================
```

### 2. Modify Metadata
Pass the flag, the new value in quotes, and the MP3 file path:

```bash
# Change title
./mp3reader -t "New Song Title" song.mp3

# Change artist
./mp3reader -a "Artist Name" song.mp3

# Change album
./mp3reader -A "Album Name" song.mp3

# Change year
./mp3reader -y "2024" song.mp3

# Change track number
./mp3reader -T "3" song.mp3

# Change comment
./mp3reader -C "My Favorite Song" song.mp3

# Change genre (index for v1 or name for v2)
./mp3reader -g "Rock" song.mp3
```

### 3. Help Menu
```bash
./mp3reader -h
# or
./mp3reader --help
```

