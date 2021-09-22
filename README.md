# Air Mouse (Linux)
Linux server for Air Mouse.

### Dependencies
- <href a="https://github.com/jordansissel/xdotool">xdotool</href>
- gtk3

### Compilation
``gcc air-mouse.c -o air-mouse -lpthread `pkg-config --cflags gtk+-3.0 --libs gtk+-3.0 --cflags glib-2.0 --libs glib-2.0`gcc air-mouse.c -o air-mouse -lpthread `pkg-config --cflags gtk+-3.0 --libs gtk+-3.0 --cflags glib-2.0 --libs glib-2.0` ``
