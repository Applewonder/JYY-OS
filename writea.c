#include <stdio.h>
#include <stdlib.h>

int main()
{
    FILE *fp;
    char str[10002]; // 用于存储字符串的字符数组
    int i;

    // 打开文件
    fp = fopen("test.txt", "w");

    // 生成长度为10000的字符串
    for(i = 0; i < 10000; i++)
    {
        str[i] = 'a';
    }
    str[10000] = '\n'; // 末尾添加字符串结束符
    str[10001] = '\0'; // 末尾添加字符串结束1
    // 将字符串写入文件
    fprintf(fp, "%s", str);

    // 关闭文件
    fclose(fp);

    return 0;
}
