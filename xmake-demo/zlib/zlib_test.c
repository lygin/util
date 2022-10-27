#include <zlib.h>
#include <string.h>
#include <stdlib.h>

int main()
{
    const char* const src = "mmmmmmmmmmmmmmmmmmmmsssssssssssssssssssssssssssskkkkkkkkkkkkkkkkkkkkkk!!!!!!!!!!!!";
    int src_size = strlen(src) + 1;
    int dst_size = compressBound(src_size);
    char * dst = (char*)malloc(dst_size);
    int z_size = compress(dst, dst_size, src, src_size);
    dst = realloc(dst, z_size);
    printf("%d ratio: %.2f", z_size/src_size);

    char * gen = (char*)malloc(src_size);
    int gen_size = uncompress(gen, src_size, dst, z_size);
    if(gen_size != src_size) {
        printf("uncompress err");
        exit(1);
    }
    if(memcmp(gen, src, src_size)) {
        printf("validate err");
        exit(1);
    }
    printf("validate OK\n%s", gen);
    return 0;
}