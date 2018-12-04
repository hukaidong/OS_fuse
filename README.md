FUSE Exercise
---

Testing environment: Ubuntu 18.04
Required package: libfuse


## Testing commands

Unittest: under ./src/prelude run `gcc *.c; ./a.out`
Execute: under ./example run `./batch` then test functions on `mountdir`

## Tech Details

 - SuperBlock  128 Bytes
 - Inodes      4095 * 128 Bytes 
   * Free space managed by bit maps on following block after inode allocated
   * Dir-inode structure 
      - metadata 16 Bytes
      - directed filename~inum map: 96 Bytes
      - lv1 extended map 8 Bytes
      - lv2 extended map 8 Bytes
   * File-inode structure 
      - metadata 16 Bytes
      - directed file blknum record: 96 Bytes 24 entries
      - lv1 extended record 8 Bytes 4 * 32 entries
      - lv2 extended map 8 Bytes 4 * 64 * 32 entries
   * Root dir binding: inode num 2
   * Maximum filename length: 13
   * Maximum entries under dir: 2084
   * Maximum file size: 4272128 Bytes ~= 4 MB
   * Maximum inode entries: 4091
 - Data Blocks
   * Free space mamaged by free linked list
   * 15 MB available space in total (1 MB for inodes)
   * Supporting function:
     - Directory create / destory
     - File read / write / **truncate**
     - Disk usage indication ( support for core du util ) 
