# Gamehook server

This is a simple http server to debug and interact with gamehook.
By default, the server uses `localhost:8766`.
The interfaces are

## `/status`
Simply prints if the game and wrapper `started` (but are not rendering yet), `running` (rendering frames), `stopped` (is closing).

## `/targets`
List all render targets in JSON.

## `/settings`
Get/set the settings of the server.
For example `/settings?W=100&H=100&fps=10&targets=final&targets=albedo` will record a 100x100 final and albedo image at 10 frames per second.

## `/current`
Get the most recent recording frame id.

## `/ids`
Get all the frame ids in the frame buffer.

## `/info`
Get the game state for a frame id.

## `/send`
Update the game state. Either use '?' and GET, or use JSON and POST.

## `/bmp`
Get a bitmap of a target `t=..` and frame id `id=..`.

## `/raw`
Same as `/bmp`, but returns the raw data.
The format is 32-bit numpy data dtype descriptor (`=u1\0`, `=u2\0`, `=u4\0`, `=f4\0` or `=f2\0`), followed by 3 32-bit integer for width, height and depth of an image.
After these initial 16 byte, the format contains the binary image data.


The server uses asio and simple-web-server (https://gitlab.com/eidheim/Simple-Web-Server)