meta:
  id: xtg
  title: XTG Monochrome Image Format
  application: Xteink E-Paper Reader
  file-extension: xtg
  xref:
    wikidata: Q116950148
  license: GPL-2.0-or-later
  ks-version: 0.9
  endian: le

doc: |
  XTG is a 1-bit per pixel monochrome bitmap format designed for ESP32 e-paper
  displays. Part of the Xteink X4 e-ink reader format family (XTC/XTCH/XTG/XTH).

  Pixel Storage:
  - Rows stored top to bottom
  - Each row stored left to right
  - 8 pixels per byte (MSB first)
  - Bit order: bit 7 (MSB) = leftmost pixel, bit 0 (LSB) = rightmost pixel

  Pixel Values:
  - 0 = Black
  - 1 = White

  Note: When using inverted bitmap drawing, the display may invert these values.

doc-ref: https://github.com/user/crengine-ng/blob/main/crengine/docs/XtcFormat.md

seq:
  - id: header
    type: header
  - id: data
    size: header.data_size
    doc: |
      Raw bitmap data, 1 bit per pixel.
      Rows stored top to bottom, pixels left to right within each row.
      8 pixels packed per byte with MSB (bit 7) representing leftmost pixel.

types:
  header:
    doc: XTG file header (22 bytes)
    seq:
      - id: magic
        type: u4
        valid: 0x00475458
        doc: 'File identifier: "XTG\0" (0x00475458 in little-endian)'
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
          expr: _ == color_mode::monochrome
        doc: Color mode (must be 0 for monochrome)
      - id: compression
        type: u1
        enum: compression_type
        valid:
          expr: _ == compression_type::uncompressed
        doc: Compression type (must be 0 for uncompressed)
      - id: data_size
        type: u4
        doc: Image data size in bytes
      - id: md5_partial
        type: u8
        doc: First 8 bytes of MD5 checksum (optional, may be 0)
    instances:
      row_bytes:
        value: (width + 7) / 8
        doc: Number of bytes per row (width rounded up to byte boundary)
      expected_data_size:
        value: row_bytes * height
        doc: |
          Expected data size based on dimensions.
          Should equal data_size field value.

enums:
  color_mode:
    0: monochrome
  compression_type:
    0: uncompressed
