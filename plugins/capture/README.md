# Capture

Dumps any recorded frame do disk.
For color images, the plugin uses the `.bmp` format, for all other modalities the plugin uses a `.raw` format.
The `.raw` format is 32-bit numpy data dtype descriptor (`=u1\0`, `=u2\0`, `=u4\0`, `=f4\0` or `=f2\0`), followed by 3 32-bit integer for width, height and depth of an image.
After these initial 16 byte, the format contains the binary image data.

See code for more details.
