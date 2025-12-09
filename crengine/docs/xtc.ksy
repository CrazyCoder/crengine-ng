meta:
  id: xtc
  title: XTC/XTCH Comic Container Format
  file-extension:
    - xtc
    - xtch
  endian: le
  license: GPL-2.0-or-later

doc: |
  XTC/XTCH is a container format for storing multiple XTG (1-bit monochrome)
  or XTH (2-bit grayscale) bitmap pages, designed for ESP32 e-paper displays.
  Used for comic/PDF reading on e-ink devices.

seq:
  - id: header
    type: header

instances:
  metadata:
    pos: header.metadata_offset
    type: metadata
    if: header.has_metadata != 0
  chapters:
    pos: header.chapter_offset
    type: chapter
    repeat: expr
    repeat-expr: metadata.chapter_count
    if: header.has_chapters != 0 and header.has_metadata != 0
  page_index:
    pos: header.index_offset
    type: page_index_entry
    repeat: expr
    repeat-expr: header.page_count

types:
  header:
    doc: XTC/XTCH file header (56 bytes)
    seq:
      - id: magic
        type: u4
        valid:
          any-of:
            - 0x00435458  # "XTC\0"
            - 0x48435458  # "XTCH"
      - id: version
        type: u2
        doc: Version number (0x0100 = v1.0)
      - id: page_count
        type: u2
        doc: Total number of pages (1-65535)
      - id: read_direction
        type: u1
        enum: reading_direction
        doc: Reading direction
      - id: has_metadata
        type: u1
        doc: Has metadata section (0 or 1)
      - id: has_thumbnails
        type: u1
        doc: Has thumbnail section (0 or 1)
      - id: has_chapters
        type: u1
        doc: Has chapter section (0 or 1)
      - id: current_page
        type: u4
        doc: Current page position (1-based, for bookmarking)
      - id: metadata_offset
        type: u8
        doc: Byte offset to metadata section
      - id: index_offset
        type: u8
        doc: Byte offset to page index table
      - id: data_offset
        type: u8
        doc: Byte offset to data area (XTG/XTH pages)
      - id: thumb_offset
        type: u8
        doc: Byte offset to thumbnail area
      - id: chapter_offset
        type: u8
        doc: Byte offset to chapter section
    instances:
      is_xtch:
        value: magic == 0x48435458
      version_major:
        value: (version >> 8) & 0xff
      version_minor:
        value: version & 0xff

  metadata:
    doc: Optional metadata structure (256 bytes)
    seq:
      - id: title
        type: str
        size: 128
        encoding: UTF-8
        doc: Title (UTF-8, null-terminated)
      - id: author
        type: str
        size: 64
        encoding: UTF-8
        doc: Author (UTF-8, null-terminated)
      - id: publisher
        type: str
        size: 32
        encoding: UTF-8
        doc: Publisher/source (UTF-8, null-terminated)
      - id: language
        type: str
        size: 16
        encoding: UTF-8
        doc: Language code (e.g., "zh-CN", "en-US")
      - id: create_time
        type: u4
        doc: Creation time (Unix timestamp)
      - id: cover_page
        type: u2
        doc: Cover page index (0-based, 0xFFFF = none)
      - id: chapter_count
        type: u2
        doc: Number of chapters
      - id: reserved
        size: 8
        doc: Reserved (zero-filled)

  chapter:
    doc: Chapter structure (96 bytes per chapter)
    seq:
      - id: name
        type: str
        size: 80
        encoding: UTF-8
        doc: Chapter name (UTF-8, null-terminated)
      - id: start_page
        type: u2
        doc: Start page (0-based)
      - id: end_page
        type: u2
        doc: End page (0-based, inclusive)
      - id: reserved1
        type: u4
        doc: Reserved 1 (zero-filled)
      - id: reserved2
        type: u4
        doc: Reserved 2 (zero-filled)
      - id: reserved3
        type: u4
        doc: Reserved 3 (zero-filled)

  page_index_entry:
    doc: Page index table entry (16 bytes per page)
    seq:
      - id: offset
        type: u8
        doc: XTG/XTH image offset (absolute, from file start)
      - id: len_data
        type: u4
        doc: XTG/XTH image size in bytes (including 22-byte XTG/XTH header)
      - id: width
        type: u2
        doc: Image width in pixels
      - id: height
        type: u2
        doc: Image height in pixels
    instances:
      data:
        io: _root._io
        pos: offset
        size: len_data
        doc: Raw XTG/XTH page data at the specified offset

enums:
  reading_direction:
    0: left_to_right
    1: right_to_left
    2: top_to_bottom