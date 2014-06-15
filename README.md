#Repeat bzip2 packer

This tool can create massive bzip2 archives quickly. For example if you need to
create 1EB file full of zero bytes compressed twice (if you do, please contact
me and tell me **why**), this is the right tool for you. Can create files up to
2EiB (one byte less exactly). It should not be hard to alter it to be able to
create larger archives, current limitation is caused because we are using 64bit
unsigned integers as pointers to individual bits.

##Usage

```
make
./repeat_packer input output [--export] [--nopack]
```

* input/output: file or - for stdin/stdout
* --export: output is binary file, not our custom format
* --nopack: don't pack (used with --export)

See *generate.sh* and *generate_all.sh* for examples.

##File format

Example (file 1EB.pack)
```
712544676207699905 0 0
hex
00
1 0 0
text
OmnivorousCamelCase
440376828399147071 0 0
hex
00
0
```

Each triple of lines represents one repetitive content:
```
<number of repetitions> <length in bytes> <length in bits>
<format: hex, text>
<data>
```
File ends with number 0. If length in bytes/bits are 0, they are estimated.
Data contain one string without whitespace. For hex data, you can use either
uppercase or lowercase letters.  Text data cannot contain whitespaces.

##Notes
In case of large repetitions, program might crash (there are finite buffers,
which can be filled up and they are not properly checked). this proram also
generates *valid* crc checksum (valid for bzip2, in fact I do not believe that
it is correct crc32 checksum of whole stream).

This was created for the preparation of the [Internet Problem Solving Contest
2014](http://ipsc.ksp.sk), program should be robust, but I would not trust it
with my life. For example if you chain this program more than twice, it will
probably crash. Maybe I will fix it at some point.

You can ignore files starting with test. They were used for debugging purposes.
