#ifndef STREAM_H
#define STREAM_H value

int fstream_read(int inum, char *buf, int size, int offset);
int fstream_write(int inum, const char *buf, int size, int offset);
void fstream_free(int inum, int new_b_size);

#endif /* ifndef STREAM_H */
