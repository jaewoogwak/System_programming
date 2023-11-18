#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_CMD_SIZE (128)

extern char **environ;

void append(char *dst, char c)
{
    char *p = dst;
    while (*p != '\0')
        p++; // 문자열 끝 탐색
    *p = c;
    *(p + 1) = '\0';
}

void nothing()
{
}

// 두 경로 중 같은 부분을 찾는 함수
char *findCommonPrefix(const char *str1, const char *str2)
{
    int len1 = strlen(str1);
    int len2 = strlen(str2);
    int minLen = (len1 < len2) ? len1 : len2;

    char *commonPrefix = (char *)malloc(minLen + 1);
    strcpy(commonPrefix, "");
    int i = 0;
    int k = 0;

    // 같은 부분 찾아서 commonPrefix에 저장
    while (i < len1)
    {
        if (str1[i] == str2[k])
        {
            append(commonPrefix, str1[i]);
            i += 1;
            k += 1;
        }
        else
        {
            strcpy(commonPrefix, "");
            i += 1;
            k = 0;
        }
    }
    return commonPrefix;
}

// 주어진 경로가 /tmp/test보다 상위 경로면 에러 출력하는 함수
int check_path(char *path)
{
    char *position = strstr(path, "/tmp/test");
    if (position == NULL)
    {
        return 0;
    }
    return 1;
}

// 경로 만들어주는 함수
char *make_path(char *tok_str)
{
    char *cp = getcwd(NULL, BUFSIZ);

    char *prefix = findCommonPrefix(cp, tok_str);

    int len = strlen(prefix);
    int idx = len;
    char *remains = (char *)malloc(MAX_CMD_SIZE);
    strcpy(remains, "");

    // 두 경로 중 같은 부분 빼고 남은 부분 remains에 저장
    // ex) A: /tmp/test/wo B: /tmp/test/wo/hi . remains: /hi
    while (idx < strlen(tok_str))
    {
        append(remains, tok_str[idx]);
        idx += 1;
    }

    // /가 없으면 붙여주기
    if (remains[0] != '/')
    {
        char *temp = (char *)malloc(MAX_CMD_SIZE);
        strcpy(temp, "/");
        strcat(temp, remains);
        strcpy(remains, temp);

        free(temp);
    }

    // 현재 경로에다가 remains 붙여서 최종 경로 만들기
    char *path = (char *)malloc(MAX_CMD_SIZE);
    strcpy(path, cp);
    strcat(path, remains);

    free(cp);
    free(prefix);
    free(remains);

    return path;
}

// 파일 사이즈 구하는 함수
int filesize(const char *filename)
{

    struct stat file_info;
    int sz_file;

    if (0 > stat(filename, &file_info))
    {
        return -1; // file이 없거나 에러
    }
    return file_info.st_size;
}

int main(int argc, char **argv)
{
    // ctrl+c 시그널 무시
    sigset_t set;

    char *command, *tok_str;
    char *current_dir = (char *)malloc(MAX_CMD_SIZE);
    strcpy(current_dir, "/");
    char *REAL_PATH_CONSTANT = getcwd(NULL, BUFSIZ);

    // 처음에 ~ 경로로 가서 경로 전체 가져오기
    char *tmp = getcwd(NULL, BUFSIZ);

    // *tmp에다가 /tmp/test 붙이기
    strcat(tmp, "/tmp/test");

    // 그걸 REAL_PATH_CONSTANT에 복사
    strcpy(REAL_PATH_CONSTANT, tmp);
    char *real_path = REAL_PATH_CONSTANT;

    command = (char *)malloc(MAX_CMD_SIZE);

    if (command == NULL)
    {
        perror("malloc");
        exit(1);
    }

    // 만약 현재 경로에 /tmp/test가 없다면 / 경로에 만들어주기
    DIR *dp;
    dp = opendir(REAL_PATH_CONSTANT);

    if (dp == NULL)
    {
        int result = mkdir("tmp", 0777);
        if (result == -1)
        {
            perror("mkdir error");
            exit(1);
        }
        result = chdir("tmp");
        if (result == -1)
        {
            perror("chdir error");
            exit(1);
        }
        result = mkdir("test", 0777);
        if (result == -1)
        {
            perror("mkdir error");
            exit(1);
        }
        printf("Success make initial directory(/tmp/test)\n");
    }

    // 현재 경로를 /tmp/test로 바꿔주기
    int result = chdir(REAL_PATH_CONSTANT);

    if (result == -1)
    {
        perror("chdir error");
        exit(1);
    }

    do
    {
        printf("%s $ ", current_dir);

        signal(SIGINT, nothing);

        if (fgets(command, MAX_CMD_SIZE - 1, stdin) == NULL)
            break;

        char *tempString = (char *)malloc(MAX_CMD_SIZE);
        strcpy(tempString, command);

        tok_str = strtok(command, " \n");

        if (tok_str == NULL)
            continue;

        // $로 시작하면 환경변수로 바꿔주기
        if (tok_str[0] == '$')
        {
            // 환경변수 이름 가져오기
            char *env_name = (char *)malloc(MAX_CMD_SIZE);
            strcpy(env_name, tok_str + 1);

            // 환경변수 값 가져오기
            char *env_value = getenv(env_name);

            // 환경변수 값으로 바꿔주기
            strcpy(tempString, env_value);
            // printf("tpstring 1 %s\n", tempString);
            strcat(tempString, tok_str + strlen(env_name) + 1);

            tok_str = strtok(tempString, " \n");
        }

        if (strcmp(tok_str, "quit") == 0)
        {
            break;
        }
        else if (strcmp(tok_str, "help") == 0)
        {
            printf("------------------------------\n");
            printf("help: print all commands\n");
            printf("cd <path>: change directory\n");
            printf("mkdir: make directory\n");
            printf("rmdir: remove directory\n");
            printf("rename: rename file\n");
            printf("ls: list files\n");
            printf("quit: exit program\n");
            printf("pwd: print current directory\n");
            printf("ln: make link file\n");
            printf("rm: remove file\n");
            printf("chmod: change file permission\n");
            printf("cat: print file contents\n");
            printf("cp: copy file\n");
            printf("mv: move file\n");
            printf("ps: displays information about a selection of the active processes.\n");
            printf("------------------------------\n");
        }
        // cd
        else if (strcmp(tok_str, "cd") == 0)
        {
            tok_str = strtok(NULL, " \n");

            // tok_str이 ~이면 /tmp/test로 바꿔주기
            if (strcmp(tok_str, "~") == 0 || strcmp(tok_str, "/") == 0)
            {
                int result = chdir(REAL_PATH_CONSTANT);
                strcpy(current_dir, "/");
                if (result == -1)
                {
                    perror("chdir error");
                    exit(1);
                }
                continue;
            }
            int result = chdir(tok_str);

            if (result == -1)
            {
                perror("chdir error");
                exit(1);
            }

            real_path = getcwd(NULL, BUFSIZ);
            char *position = strstr(real_path, "/tmp/test");

            if (position != NULL)
            {
                // target_path의 위치 계산
                int index = position - real_path;
                // printf("/tmp/test의 위치: %d\n", index);

                // real_path에서 index+8 위치부터 끝까지를 target_path로 복사
                char *target_path = (char *)malloc(MAX_CMD_SIZE);
                strcpy(target_path, real_path + index + 9);

                // current_dir에 target_path 복사
                if (strcmp(target_path, "") == 0)
                {
                    strcpy(current_dir, "/");
                }
                else
                {
                    strcpy(current_dir, target_path);
                }
                free(target_path);
                printf("Success change directory\n");
            }
            else
            {
                // 루트보다 상위 경로로 가려할 때
                printf("Cannot change directory\n");
                strcpy(current_dir, "/");
                chdir(REAL_PATH_CONSTANT);
                // real_path도 되돌리기
                real_path = REAL_PATH_CONSTANT;
            }
        }
        // pwd
        else if (strcmp(tok_str, "pwd") == 0)
        {
            printf("%s \n", current_dir);
        }
        // mkdir
        else if (strcmp(tok_str, "mkdir") == 0)
        {
            tok_str = strtok(NULL, " \n");

            // tok_str이 " "이면 missing operand
            if (tok_str == NULL)
            {
                perror("missing operand");
                continue;
            }

            // 경로 생성
            char *path = (char *)malloc(MAX_CMD_SIZE);
            realpath(tok_str, path);

            // path가 /tmp/test보다 상위 경로면 에러
            int result = check_path(path);

            if (result == 0)
            {
                printf("Access error: cannot make directory\n");
                continue;
            }

            result = mkdir(path, 0777);
            if (result == -1)
            {
                perror("mkdir error");
                exit(1);
            }
            free(path);
            printf("Success make directory\n");
        }
        else if (strcmp(tok_str, "rmdir") == 0)
        {
            tok_str = strtok(NULL, " \n");

            // tok_str이 " "이면 missing operand
            if (tok_str == NULL)
            {
                perror("missing operand");
                continue;
            }

            // 경로 생성
            char *path = (char *)malloc(MAX_CMD_SIZE);
            realpath(tok_str, path);

            // path가 /tmp/test보다 상위 경로면 에러
            int result = check_path(path);

            if (result == 0)
            {
                printf("Access error: cannot remove directory\n");
                continue;
            }

            result = rmdir(path);
            if (result == -1)
            {
                perror("rmdir error");
                exit(1);
            }
            free(path);
            printf("Success remove directory\n");
        }
        // rename
        else if (strcmp(tok_str, "rename") == 0)
        {
            tok_str = strtok(NULL, " \n");
            char *old_name = tok_str;
            // 경로 생성
            char *path = (char *)malloc(MAX_CMD_SIZE);
            realpath(old_name, path);

            // path가 /tmp/test보다 상위 경로면 에러
            int result = check_path(path);

            if (result == 0)
            {
                printf("Access error: cannot rename directory\n");
                continue;
            }

            tok_str = strtok(NULL, " \n");
            char *new_name = tok_str;
            // 경로 생성
            char *path2 = (char *)malloc(MAX_CMD_SIZE);
            realpath(new_name, path2);

            // path가 /tmp/test보다 상위 경로면 에러
            result = check_path(path2);

            if (result == 0)
            {
                printf("Access error: cannot rename directory\n");
                continue;
            }

            result = rename(old_name, new_name);

            if (result == -1)
            {
                perror("rename error");
                exit(1);
            }

            free(path);
            free(path2);

            printf("Success rename file\n");
        }
        // ls
        else if (strcmp(tok_str, "ls") == 0)
        {
            DIR *dp;
            struct dirent *dent;
            struct passwd *pwd;
            struct group *gr;
            struct stat st;

            dp = opendir(".");

            if (dp == NULL)
            {
                perror("opendir error");
                exit(1);
            }

            printf("File type    File name      size       UID      GID     Last access time          Last modification time    Last status change time    Hlink count  Slink \n");
            printf("---------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

            while ((dent = readdir(dp)))
            {

                stat(dent->d_name, &st);
                printf("%c", (S_ISDIR(st.st_mode)) ? 'd' : '-');
                printf("%c", (st.st_mode & S_IRUSR) ? 'r' : '-');
                printf("%c", (st.st_mode & S_IWUSR) ? 'w' : '-');
                printf("%c", (st.st_mode & S_IXUSR) ? 'x' : '-');
                printf("%c", (st.st_mode & S_IRGRP) ? 'r' : '-');
                printf("%c", (st.st_mode & S_IWGRP) ? 'w' : '-');
                printf("%c", (st.st_mode & S_IXGRP) ? 'x' : '-');
                printf("%c", (st.st_mode & S_IROTH) ? 'r' : '-');
                printf("%c", (st.st_mode & S_IWOTH) ? 'w' : '-');
                printf("%c", (st.st_mode & S_IXOTH) ? 'x' : '-');
                printf("   ");

                printf("%-15s", dent->d_name);
                printf("%-7ld", st.st_size);
                printf("%-12s", (struct passwd *)getpwuid(getuid())->pw_name);
                printf("%-12s", (struct passwd *)getgrgid(getgid())->gr_name);

                // 파일 접근 생성, 수정 시간을 하나의 문자열로 저장
                char *access_time = (char *)malloc(MAX_CMD_SIZE);
                char *modify_time = (char *)malloc(MAX_CMD_SIZE);
                char *change_time = (char *)malloc(MAX_CMD_SIZE);

                strcpy(access_time, ctime(&st.st_atime));
                strcpy(modify_time, ctime(&st.st_mtime));
                strcpy(change_time, ctime(&st.st_ctime));

                // 개행 없애기
                access_time[strlen(access_time) - 1] = '\0';
                modify_time[strlen(modify_time) - 1] = '\0';
                change_time[strlen(change_time) - 1] = '\0';

                // 파일 접근 생성, 수정 시간 출력
                printf("%-26s", access_time);
                printf("%-26s", modify_time);
                printf("%-27s", change_time);

                free(access_time);
                free(modify_time);
                free(change_time);
                printf("%-7ld", st.st_nlink);
                // 심볼릭 링크 출력
                char *buf = (char *)malloc(MAX_CMD_SIZE);
                int result = readlink(dent->d_name, buf, MAX_CMD_SIZE);
                if (result != -1)
                {
                    printf("%s -> %s", dent->d_name, buf);
                }
                free(buf);
                printf("\n");
            }
            closedir(dp);
            printf("Success list files\n");
        }

        // ln
        else if (strcmp(tok_str, "ln") == 0)
        {
            tok_str = strtok(NULL, " \n");
            char *option = tok_str;

            // 옵션이 -s면 심볼릭 링크 생성
            // 아니면 하드 링크 생성
            if (strcmp(option, "-s") == 0)
            {
                tok_str = strtok(NULL, " \n");
                char *old_name = tok_str;
                // 경로 생성
                char *path = (char *)malloc(MAX_CMD_SIZE);
                realpath(old_name, path);

                // path가 /tmp/test보다 상위 경로면 에러
                int result = check_path(path);
                if (result == 0)
                {
                    printf("Access error: cannot link directory or file\n");
                    continue;
                }

                tok_str = strtok(NULL, " \n");
                char *new_name = tok_str;

                // 경로 생성
                char *path2 = (char *)malloc(MAX_CMD_SIZE);
                realpath(new_name, path2);

                // path가 /tmp/test보다 상위 경로면 에러
                result = check_path(path2);

                if (result == 0)
                {
                    printf("Access error: cannot link directory or file\n");
                    continue;
                }

                result = symlink(old_name, new_name);

                if (result == -1)
                {
                    perror("Symlink error");
                    exit(1);
                }

                free(path);
                free(path2);
            }
            else
            {
                // 역시 old_name, new_name 모두 경로 체크
                char *old_name = option;
                // 경로 생성
                char *path = (char *)malloc(MAX_CMD_SIZE);
                realpath(old_name, path);

                // path가 /tmp/test보다 상위 경로면 에러
                int result = check_path(path);

                if (result == 0)
                {
                    printf("Access error: cannot link directory or file\n");
                    continue;
                }

                tok_str = strtok(NULL, " \n");
                char *new_name = tok_str;

                // 경로 생성
                char *path2 = (char *)malloc(MAX_CMD_SIZE);
                realpath(new_name, path2);

                // path가 /tmp/test보다 상위 경로면 에러
                result = check_path(path2);

                if (result == 0)
                {
                    printf("Access error: cannot link directory or file\n");
                    continue;
                }

                result = link(old_name, new_name);

                if (result == -1)
                {
                    perror("link error");
                    exit(1);
                }

                free(path);
                free(path2);
            }

            printf("Success link file\n");
        }
        // rm
        else if (strcmp(tok_str, "rm") == 0)
        {
            tok_str = strtok(NULL, " \n");
            // 경로 생성
            char *path = (char *)malloc(MAX_CMD_SIZE);
            realpath(tok_str, path);

            // path가 /tmp/test보다 상위 경로면 에러
            int result = check_path(path);

            if (result == 0)
            {
                printf("Access error: cannot remove file\n");
                continue;
            }

            result = remove(path);

            if (result == -1)
            {
                perror("File remove error");
                exit(1);
            }

            free(path);
            printf("Success remove file\n");
        }
        // chmod
        else if (strcmp(tok_str, "chmod") == 0)
        {
            // chmod 777 test.txt, chmod u-x test.txt 둘다 가능하게 하기
            tok_str = strtok(NULL, " \n");
            char *mode = tok_str;

            // mode가 숫자면 그대로 사용
            if (mode[0] >= '0' && mode[0] <= '9')
            {
                tok_str = strtok(NULL, " \n");
                // 경로 생성
                char *path = (char *)malloc(MAX_CMD_SIZE);
                realpath(tok_str, path);

                // path가 /tmp/test보다 상위 경로면 에러
                int result = check_path(path);

                if (result == 0)
                {
                    printf("Access error: cannot change mode directory or file\n");
                    continue;
                }

                result = chmod(path, strtol(mode, 0, 8));

                if (result == -1)
                {
                    perror("chmod error");
                    exit(1);
                }

                free(path);

                printf("Success chmod\n");
            }
            // 문자면 문자열 파싱
            else
            {
                // path 저장
                tok_str = strtok(NULL, " \n");
                if (tok_str == NULL)
                {
                    perror("missing operand");
                    exit(1);
                }

                // 경로 생성
                char *path = (char *)malloc(MAX_CMD_SIZE);
                realpath(tok_str, path);

                // path가 /tmp/test보다 상위 경로면 에러
                int result = check_path(path);

                if (result == 0)
                {
                    printf("Access error: cannot change mode directory or file\n");
                    continue;
                }

                // mode 임시값에 저장
                char *temp = (char *)malloc(MAX_CMD_SIZE);
                strcpy(temp, mode);

                // mode를 +또는- 기준으로 분리
                // 먼저 user부터
                char *user = (char *)malloc(MAX_CMD_SIZE);
                int isAll = 0; // false

                // mode가 +또는 -로 시작하면 all
                if (temp[0] == '+' || temp[0] == '-')
                {
                    isAll = 1;
                    strcpy(user, "");
                }
                else
                {
                    char *tok_str2 = strtok(mode, "+-");
                    strcpy(user, tok_str2);
                }

                // 그 다음 permission
                char *permission = (char *)malloc(MAX_CMD_SIZE);
                if (isAll == 1)
                {
                    char *tok_str3 = strtok(mode, "+-");
                    strcpy(permission, tok_str3);
                }
                else
                {
                    char *tok_str3 = strtok(NULL, "+-");
                    strcpy(permission, tok_str3);
                }

                char operator= temp[strlen(user)];

                struct stat st;
                stat(path, &st);

                // 만약 user길이가 0이면 all
                if (strlen(user) == 0)
                {
                    for (int j = 0; j < strlen(permission); j++)
                    {
                        if (permission[j] == 'r')
                        {
                            // user, group, other 모두 r
                            if (operator== '+')
                            {
                                chmod(path, st.st_mode | S_IRUSR);
                                stat(path, &st);
                                chmod(path, st.st_mode | S_IRGRP);
                                stat(path, &st);
                                chmod(path, st.st_mode | S_IROTH);
                                stat(path, &st);
                            }
                            else
                            {
                                chmod(path, st.st_mode & ~S_IRUSR);
                                stat(path, &st);
                                chmod(path, st.st_mode & ~S_IRGRP);
                                stat(path, &st);
                                chmod(path, st.st_mode & ~S_IROTH);
                                stat(path, &st);
                            }
                        }
                        else if (permission[j] == 'w')
                        {
                            // user, group, other 모두 w
                            if (operator== '+')
                            {
                                chmod(path, st.st_mode | S_IWUSR);
                                stat(path, &st);
                                chmod(path, st.st_mode | S_IWGRP);
                                stat(path, &st);
                                chmod(path, st.st_mode | S_IWOTH);
                                stat(path, &st);
                            }
                            else
                            {
                                chmod(path, st.st_mode & ~S_IWUSR);
                                stat(path, &st);
                                chmod(path, st.st_mode & ~S_IWGRP);
                                stat(path, &st);
                                chmod(path, st.st_mode & ~S_IWOTH);
                                stat(path, &st);
                            }
                            stat(path, &st);
                        }
                        else if (permission[j] == 'x')
                        {
                            // user, group, other 모두 x
                            if (operator== '+')
                            {
                                chmod(path, st.st_mode | S_IXUSR);
                                stat(path, &st);
                                chmod(path, st.st_mode | S_IXGRP);
                                stat(path, &st);
                                chmod(path, st.st_mode | S_IXOTH);
                                stat(path, &st);
                            }
                            else
                            {
                                chmod(path, st.st_mode & ~S_IXUSR);
                                stat(path, &st);
                                chmod(path, st.st_mode & ~S_IXGRP);
                                stat(path, &st);
                                chmod(path, st.st_mode & ~S_IXOTH);
                                stat(path, &st);
                            }
                            stat(path, &st);
                        }
                    }
                }
                // 앞글자가 +-로 시작하지 않는 경우
                else
                {
                    for (int i = 0; i < strlen(user); i++)
                    {
                        if (user[i] == 'u')
                        {
                            for (int j = 0; j < strlen(permission); j++)
                            {
                                if (permission[j] == 'r')
                                {
                                    if (operator== '+')
                                    {
                                        chmod(path, st.st_mode | S_IRUSR);
                                    }
                                    else
                                    {
                                        chmod(path, st.st_mode & ~S_IRUSR);
                                    }
                                    stat(path, &st);
                                }
                                else if (permission[j] == 'w')
                                {
                                    if (operator== '+')
                                    {
                                        chmod(path, st.st_mode | S_IWUSR);
                                    }
                                    else
                                    {
                                        chmod(path, st.st_mode & ~S_IWUSR);
                                    }
                                    stat(path, &st);
                                }
                                else if (permission[j] == 'x')
                                {
                                    if (operator== '+')
                                    {
                                        chmod(path, st.st_mode | S_IXUSR);
                                    }
                                    else
                                    {
                                        chmod(path, st.st_mode & ~S_IXUSR);
                                    }
                                    stat(path, &st);
                                }
                            }
                        }
                        else if (user[i] == 'g')
                        {
                            for (int j = 0; j < strlen(permission); j++)
                            {
                                if (permission[j] == 'r')
                                {
                                    if (operator== '+')
                                    {
                                        chmod(path, st.st_mode | S_IRGRP);
                                    }
                                    else
                                    {
                                        chmod(path, st.st_mode & ~S_IRGRP);
                                    }
                                    stat(path, &st);
                                }
                                else if (permission[j] == 'w')
                                {
                                    if (operator== '+')
                                    {
                                        chmod(path, st.st_mode | S_IWGRP);
                                    }
                                    else
                                    {
                                        chmod(path, st.st_mode & ~S_IWGRP);
                                    }
                                    stat(path, &st);
                                }
                                else if (permission[j] == 'x')
                                {
                                    if (operator== '+')
                                    {
                                        chmod(path, st.st_mode | S_IXGRP);
                                    }
                                    else
                                    {
                                        chmod(path, st.st_mode & ~S_IXGRP);
                                    }
                                    stat(path, &st);
                                }
                            }
                        }
                        else if (user[i] == 'o')
                        {
                            for (int j = 0; j < strlen(permission); j++)
                            {
                                if (permission[j] == 'r')
                                {
                                    if (operator== '+')
                                    {
                                        chmod(path, st.st_mode | S_IROTH);
                                    }
                                    else
                                    {
                                        chmod(path, st.st_mode & ~S_IROTH);
                                    }
                                    stat(path, &st);
                                }
                                else if (permission[j] == 'w')
                                {
                                    if (operator== '+')
                                    {
                                        chmod(path, st.st_mode | S_IWOTH);
                                    }
                                    else
                                    {
                                        chmod(path, st.st_mode & ~S_IWOTH);
                                    }
                                    stat(path, &st);
                                }
                                else if (permission[j] == 'x')
                                {
                                    if (operator== '+')
                                    {
                                        chmod(path, st.st_mode | S_IXOTH);
                                    }
                                    else
                                    {
                                        chmod(path, st.st_mode & ~S_IXOTH);
                                    }
                                    stat(path, &st);
                                }
                            }
                        }
                    }
                }
                free(temp);
                free(user);
                free(permission);
                free(path);

                printf("Success chmod\n");
            }
        }
        // cat
        else if (strcmp(tok_str, "cat") == 0)
        {
            tok_str = strtok(NULL, " \n");
            char *path = make_path(tok_str);

            FILE *fp = fopen(path, "r");
            if (fp == NULL)
            {
                perror("fopen error");
                exit(1);
            }

            char buf[MAX_CMD_SIZE];
            while (fgets(buf, MAX_CMD_SIZE, fp) != NULL)
            {
                printf("%s", buf);
            }

            free(path);
            fclose(fp);
            printf("\nSuccess cat\n");
        }

        // cp using mmap, memcpy, ftruncate
        else if (strcmp(tok_str, "cp") == 0)
        {
            // old_name부터 검사
            tok_str = strtok(NULL, " \n");
            char *old_name = tok_str;
            // 경로 생성
            char *path = (char *)malloc(MAX_CMD_SIZE);
            realpath(old_name, path);

            // path가 /tmp/test보다 상위 경로면 에러
            int result = check_path(path);

            if (result == 0)
            {
                printf("Access error: cannot copy directory or file\n");
                continue;
            }

            // new_name 검사
            tok_str = strtok(NULL, " \n");
            char *new_name = tok_str;

            // 경로 생성
            char *path2 = (char *)malloc(MAX_CMD_SIZE);
            realpath(new_name, path2);

            // path가 /tmp/test보다 상위 경로면 에러
            result = check_path(path2);

            if (result == 0)
            {
                printf("Access error: cannot copy directory or file\n");
                continue;
            }

            // result = link(path, path2);

            int fd;
            caddr_t addr;
            struct stat statbuf;

            // 기존 파일 검사
            if (stat(path, &statbuf) == -1)
            {
                perror("stat");
                exit(1);
            }

            if ((fd = open(path, O_RDWR)) == -1)
            {
                if (result == -1)
                {
                    perror("link error");
                    exit(1);
                }
            }

            // 기존 파일 내용을 addr에 매핑
            addr = mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)0);

            if (addr == MAP_FAILED)
            {
                perror("mmap");
                exit(1);
            }

            // 새로운 파일을 생성
            int newfile_fd, pagesize, length;
            caddr_t newfile_addr;
            pagesize = sysconf(_SC_PAGE_SIZE);

            // 새로운 파일을 user, group, other 모두 읽기 쓰기 가능하게 (0666) 생성
            if ((newfile_fd = open(path2, O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1)
            {
                perror("open");
                exit(1);
            }

            // 새롭게 생성한 파일은 비어있어서 mmap으로 메모리 매핑 불가능. 그래서 ftruncate로 파일 크기를 패이지 크기로 증가시킴
            if (ftruncate(newfile_fd, (off_t)filesize(path)) == -1)
            {
                perror("fturncate");
                exit(1);
            }

            // 새롭게 복사할 파일 기술자에 메모리 매핑해줌
            newfile_addr = mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, newfile_fd, (off_t)0);
            if (newfile_addr == MAP_FAILED)
            {
                perror("mmap");
                exit(1);
            }

            // 이제 memcpy로 내용 복사하자
            memcpy(newfile_addr, addr, filesize(path));

            close(fd);
            close(newfile_fd);

            free(path);
            free(path2);

            printf("Success copy file\n");
        }
        // mv
        else if (strcmp(tok_str, "mv") == 0)
        {
            // old_name부터 검사
            tok_str = strtok(NULL, " \n");
            char *old_name = tok_str;

            // 경로 생성
            char *path = (char *)malloc(MAX_CMD_SIZE);
            realpath(old_name, path);

            // path가 /tmp/test보다 상위 경로면 에러
            result = check_path(path);

            if (result == 0)
            {
                printf("Access error: cannot move directory or file\n");
                continue;
            }

            tok_str = strtok(NULL, " \n");
            char *new_name = tok_str;

            // new_name 검사
            // 경로 생성
            char *path2 = (char *)malloc(MAX_CMD_SIZE);
            realpath(new_name, path2);

            // path가 /tmp/test보다 상위 경로면 에러
            int result = check_path(path2);

            if (result == 0)
            {
                printf("Access error: cannot move directory or file\n");
                continue;
            }

            result = rename(path, path2);

            if (result == -1)
            {
                perror("rename error");
                exit(1);
            }

            free(path);
            free(path2);

            printf("Success move file\n");
        }
        else if (strcmp(tok_str, "echo") == 0)
        {
            tok_str = strtok(NULL, " \n");

            // $로 시작하면 환경변수로 바꿔주기
            if (tok_str[0] == '$')
            {
                // 환경변수 이름 가져오기
                char *env_name = (char *)malloc(MAX_CMD_SIZE);
                strcpy(env_name, tok_str + 1);

                // 환경변수 값 가져오기
                char *env_value = getenv(env_name);

                // 환경변수 값으로 바꿔주기
                strcpy(tempString, env_value);
                strcat(tempString, tok_str + strlen(env_name) + 1);

                tok_str = strtok(tempString, "\n");
            }

            char *str = tok_str;

            printf("%s\n", str);
        }
        // LS=ls -al과 같이 들어오면 환경변수 설정
        else if (strstr(tok_str, "=") != NULL)
        {
            printf("set environment variable\n");

            putenv(tempString);
        }
        // env 출력
        else if (strcmp(tok_str, "env") == 0)
        {
            char **env = environ;
            while (*env)
            {
                printf("%s\n", *env);
                env++;
            }
        }
        // unset
        else if (strcmp(tok_str, "unset") == 0)
        {
            tok_str = strtok(NULL, " \n");
            unsetenv(tok_str);

            printf("Success unset environment variable\n");
        }
        // ps
        else if (strcmp(tok_str, "ps") == 0)
        {
            DIR *proc_dir;
            struct dirent *entry;

            // /proc 디렉토리 열기
            proc_dir = opendir("/proc");
            if (proc_dir == NULL)
            {
                perror("opendir");
                exit(1);
            }

            // 헤더 출력
            printf("%-5s %-25s %s\n", "PID", "NAME", "CMDLINE");

            // 프로세스 정보 읽기
            while ((entry = readdir(proc_dir)) != NULL)
            {
                if (entry->d_type == DT_DIR)
                {
                    // 디렉토리인지 확인하고 이름이 숫자로만 이루어져 있는지 확인
                    size_t len = strlen(entry->d_name);
                    int is_pid = 1;
                    for (size_t i = 0; i < len; i++)
                    {
                        if (!isdigit(entry->d_name[i]))
                        {
                            is_pid = 0;
                            break;
                        }
                    }

                    if (is_pid)
                    {
                        char stat_path[256];
                        snprintf(stat_path, sizeof(stat_path), "/proc/%s/stat", entry->d_name);

                        FILE *stat_file = fopen(stat_path, "r");
                        if (stat_file != NULL)
                        {
                            // stat 파일에서 프로세스 정보 읽기
                            int pid;
                            char comm[256];
                            char state;
                            fscanf(stat_file, "%d %s %c", &pid, comm, &state);
                            fclose(stat_file);

                            char cmdline_path[256];

                            // 프로세스 Pid, 이름(comm), cmdline
                            printf("%-5d %-25s ", pid, comm);

                            // cmdline 파일 열기
                            snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%s/cmdline", entry->d_name);
                            FILE *cmdline_file = fopen(cmdline_path, "r");
                            if (cmdline_file != NULL)
                            {
                                char cmdline[256];
                                fgets(cmdline, sizeof(cmdline), cmdline_file);
                                fclose(cmdline_file);

                                // cmdline에 개행 문자가 있으면 널 문자로 바꿔서 출력
                                char *pos = strchr(cmdline, '\n');
                                if (pos != NULL)
                                {
                                    *pos = '\0';
                                }

                                printf("%s\n", cmdline);
                            }
                        }
                    }
                }
            }

            closedir(proc_dir);
        }
        // kill
        else if (strcmp(tok_str, "kill") == 0)
        {
            int signal_num = atoi(strtok(NULL, " \n"));
            int pid = atoi(strtok(NULL, " \n"));

            printf("kill %d to %d process\n", signal_num, pid);
            kill(pid, signal_num);
        }

        else
        {
            // 존재하지 않는 커맨드
            printf("No such command\n");
        }

    } while (1);

    free(command);

    return 0;
}