meta:
  id: xth
  title: XTH 4-Level Grayscale Image Format
  application: Xteink E-Paper Reader
  file-extension: xth
  xref:
    wikidata: Q116950148
  license: GPL-2.0-or-later
  ks-version: 0.9
  endian: le

doc: |
  XTH is a 2-bit per pixel grayscale bitmap format for 4-level grayscale e-paper
  displays. Part of the Xteink X4 e-ink reader format family (XTC/XTCH/XTG/XTH).

  Data is stored as two separate bit planes using vertical scan order (column-major):
  - First plane contains Bit1 (high bit) for all pixels, sent via e-paper command 0x24
  - Second plane contains Bit2 (low bit) for all pixels, sent via e-paper command 0x26

  Vertical Scan Order:
  - Columns scanned from RIGHT to LEFT (x = width-1 down to 0)
  - Within each column, 8 vertical pixels packed per byte (top to bottom)
  - MSB (bit 7) = topmost pixel in the group of 8
  - This matches the Xteink e-paper display refresh pattern

  Pixel Value Calculation: pixelValue = (bit1 << 1) | bit2

  Grayscale Mapping (Xteink LUT - NOTE: middle values are swapped):
  - 0 (00): White
  - 1 (01): Dark Grey  (non-linear: darker than expected)
  - 2 (10): Light Grey (non-linear: lighter than expected)
  - 3 (11): Black

  Example for 480x800 image:
  - Each column: 800 pixels = 100 bytes (800/8 vertical groups)
  - Total columns: 480 (scanned right to left)
  - Each bit plane: 480 * 100 = 48000 bytes
  - Total data size: 96000 bytes

doc-ref: https://github.com/user/crengine-ng/blob/main/crengine/docs/XtcFormat.md

seq:
  - id: header
    type: header
  - id: bit_plane_1
    size: header.plane_size
    doc: |
      First bit plane containing Bit1 (high bit) for all pixels.
      Sent to e-paper display via command 0x24.
      Data uses vertical scan order: columns right-to-left, 8 vertical pixels per byte.
  - id: bit_plane_2
    size: header.plane_size
    doc: |
      Second bit plane containing Bit2 (low bit) for all pixels.
      Sent to e-paper display via command 0x26.
      Data uses vertical scan order: columns right-to-left, 8 vertical pixels per byte.

types:
  header:
    doc: XTH file header (22 bytes)
    seq:
      - id: magic
        type: u4
        valid: 0x00485458
        doc: 'File identifier: "XTH\0" (0x00485458 in little-endian)'
      - id: width
        type: u2
        valid:
          min: 1
        doc: Image width in pixels (1-65535)
      - id: height
        type: u2
        valid:
          min: 1
        doc: Image height in pixels (1-65535)
      - id: color_mode
        type: u1
        enum: color_mode
        valid:
          expr: _ == color_mode::grayscale_4level
        doc: Color mode (0 = 4-level grayscale)
      - id: compression
        type: u1
        enum: compression_type
        valid:
          expr: _ == compression_type::uncompressed
        doc: Compression type (must be 0 for uncompressed)
      - id: data_size
        type: u4
        doc: Total image data size in bytes (both planes combined)
      - id: md5_partial
        type: u8
        doc: First 8 bytes of MD5 checksum (optional, may be 0)
    instances:
      total_pixels:
        value: width * height
        doc: Total number of pixels in the image
      plane_size:
        value: (total_pixels + 7) / 8
        doc: Size of each bit plane in bytes (pixels rounded up to byte boundary)
      expected_data_size:
        value: plane_size * 2
        doc: |
          Expected total data size for both bit planes.
          Should equal data_size field value.
      num_columns:
        value: width
        doc: Number of columns (scanned right to left)
      bytes_per_column:
        value: (height + 7) / 8
        doc: Bytes per column (height rounded up to groups of 8 pixels)

enums:
  color_mode:
    0:
      id: grayscale_4level
      doc: 4-level grayscale (2 bits per pixel)
  compression_type:
    0:
      id: uncompressed
      doc: No compression applied

  grayscale_level:
    0:
      id: white
      doc: 'Level 0 (00): White'
    1:
      id: dark_grey
      doc: 'Level 1 (01): Dark Grey - Xteink LUT swapped value'
    2:
      id: light_grey
      doc: 'Level 2 (10): Light Grey - Xteink LUT swapped value'
    3:
      id: black
      doc: 'Level 3 (11): Black'
