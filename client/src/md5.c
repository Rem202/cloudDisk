#include "head.h"

int getFileMd5(const char *file_name, char *md5_value)
{
    // bzero(*md5_value, sizeof(*md5_value));
    FILE *file = fopen(file_name, "rb");

    ERROR_CHECK(file, NULL, "fopen");
    MD5_CTX md5_context;
    MD5_Init(&md5_context);
    char buf[1024 * 16];
    int ret;
    while((ret = fread(buf, 1, sizeof(buf), file)) != 0)
    {
        MD5_Update(&md5_context, buf, ret);
    }

    // 
    unsigned char res[MD5_DIGEST_LENGTH];
    MD5_Final(res, &md5_context);
    char hex[MD5_DIGEST_LENGTH * 2 + 1];
    bzero(hex, sizeof(hex));
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i)
    {
        sprintf(hex + i * 2, "%02x", res[i]);
    }
    hex[32] = '\0';
    strcpy(md5_value, hex);
    fclose(file);
    return 0;
}