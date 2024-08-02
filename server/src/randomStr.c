#include <func_zm.h>

int gennerateStr(char **res,int str_len){
    srand(time(NULL));
    for(int i = 0; i < str_len; ++i){
        int temp = rand() % 3;
        switch (temp)
        {
        case 0:
            (*res)[i] = rand() % 26 + 'a';
            break;
        case 1:
            (*res)[i] = rand() % 26 + 'A';
            break;
        case 2:
            (*res)[i] = rand() % 10 + '0';
            break;
        default:
            break;
        }
    }
    return 0;
}