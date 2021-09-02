# base64_c
A simple base64 encoder written in C89

## To compile
```
gcc -o base64 base64.c -Ofast
```
(With O=fast, gcc)

## Usage:
./base64 -[h|d] [FILE]

-d, --decode: Decode base64 string (default is to encode)
-h, --help:   Display this usage info.

When no FILE is specified or when FILE is '-', it will read
from standard input. (On bash, it will stop reading stdin
when you hit Ctrl+D.)
