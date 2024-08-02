// #include "head.h"
#include <func_zm.h>
int ls(const char *dir){
    // 列出当前目录下所有目录和文件
    DIR *dirp = opendir(dir);
    ERROR_CHECK(dirp, NULL, "opendir");
    struct dirent *pdirent;
    while ((pdirent = readdir(dirp))!=NULL)
    {
        // 打印文件名
        if(strcmp(pdirent->d_name,".") == 0 ||  strcmp(pdirent->d_name,"..") ==0){
            continue;
        }
        printf("%s ", pdirent->d_name);
    }
    closedir(dirp);
    puts("");
    return 0;
}

void pwd(){
    dfs_pwd(".", 0, 2);
    puts("");
}

int dfs_pwd(const char* dir, ino_t chid_ino, ino_t stop){
    // 打印当前工作目录 递归实现 "./ ./_ino"
    DIR *dirp = opendir(dir);   
    ERROR_CHECK(dirp, NULL, "opendir");
    struct dirent *pdirent;
    // 获取当前目录的 ino
    ino_t cur_ino;
    char cur_name[1024]= {0};
    bool parent_exist = false;
    while((pdirent = readdir(dirp)) != NULL){
        if(strcmp(pdirent->d_name, ".") == 0){
            cur_ino = pdirent->d_ino;
        }
        else if(strcmp(pdirent->d_name, "..") == 0){
            // 默认 stop = 2 即系统主目录 / 其 ino = 2
            parent_exist = true;
        }
        else if(pdirent->d_ino == chid_ino && strcmp(pdirent->d_name, ".")!=0){
            memcpy(cur_name, pdirent->d_name, sizeof(pdirent->d_name));
        }
    }
    // cur_ino !=stop && cur_ino != chid_ino
    if(cur_ino !=stop){
        char t[1024] = "";
        memcpy(t, dir, strlen(dir));
        dfs_pwd(strcat(t, "/.."), cur_ino, stop);
    }
    if(strlen(cur_name) > 0 || cur_ino ==stop){
        printf("/%s", cur_name);
    }
    closedir(dirp);
    return 0;
}

void cd(char* args){
    // 切换路径 判断特殊路径
    // 判断特殊路径
    int ret;
    char path[2048];
    if(strcmp(args, "/") == 0){
        ret = chdir("/");
        ERROR_CHECK(ret, -1, "chidr");
    }else{
        getcwd(path, sizeof(path));
        printf("cwd = %s\n", path);
        strcat(path, args);
        chdir(path);
    }
    getcwd(path, sizeof(path));
}
