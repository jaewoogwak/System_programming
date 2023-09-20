#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_CMD_SIZE (128)


void append(char *dst, char c) {
    char *p = dst;
    while (*p != '\0') p++; // 문자열 끝 탐색
    *p = c;
    *(p+1) = '\0';  
}

// 두 경로 중 같은 부분을 찾는 함수
char* findCommonPrefix(const char* str1, const char* str2) {
    int len1 = strlen(str1);
    int len2 = strlen(str2);
    int minLen = (len1 < len2) ? len1 : len2;

    // printf("str1: %s\n", str1);
    // printf("str2: %s\n", str2);

    char* commonPrefix = (char*)malloc(minLen + 1);
    strcpy(commonPrefix, "");
    int i = 0;
    int k = 0;

    // 같은 부분 찾아서 commonPrefix에 저장
    while (i < len1) {
        if (str1[i] == str2[k]) {
            append(commonPrefix, str1[i]);
            i+=1;
            k+=1;
        } else {
            strcpy(commonPrefix, "");
            i+=1;
            k=0;
        }
        // printf("res %s \n", commonPrefix);
    }
    return commonPrefix;
}

// 경로 만들어주는 함수
char *make_path(char* tok_str) {
    char *cp = getcwd(NULL, BUFSIZ);
    char *prefix = findCommonPrefix(cp, tok_str);

    // printf("result: %s\n", prefix);
    int len = strlen(prefix);
    int idx = len;
    char *remains = (char*) malloc(MAX_CMD_SIZE);
    strcpy(remains, "");

    // 두 경로 중 같은 부분 빼고 남은 부분 remains에 저장
    // ex) A: /tmp/test/wo B: /tmp/test/wo/hi -> remains: /hi
    while (idx < strlen(tok_str)) {
        append(remains, tok_str[idx]);
        idx+=1;
    }

    // /가 없으면 붙여주기
    if ( remains[0] != '/') {
        char *temp = (char*) malloc(MAX_CMD_SIZE);
        strcpy(temp, "/");
        strcat(temp, remains);
        strcpy(remains, temp);

        free(temp);
    }

    // 현재 경로에다가 remains 붙여서 최종 경로 만들기
    char *path = (char*) malloc(MAX_CMD_SIZE);
    strcpy(path, cp);
    strcat(path, remains);

    free(cp);
    free(prefix);
    free(remains);

    // printf("path %s \n", path);
    return path;
}

int main(int argc, char **argv)
{
	char *command, *tok_str;
	char *current_dir = (char*) malloc(MAX_CMD_SIZE);
    strcpy(current_dir, "/");
    char *REAL_PATH_CONSTANT = getcwd(NULL, BUFSIZ);
    
    // 처음에 ~ 경로로 가서 경로 전체 가져오기
    char *tmp = getcwd(NULL, BUFSIZ);

    // *tmp에다가 /tmp/test 붙이기
    strcat(tmp, "/tmp/test");

    // 그걸 REAL_PATH_CONSTANT에 복사
    strcpy(REAL_PATH_CONSTANT, tmp);
    char *real_path = REAL_PATH_CONSTANT;

	command = (char*) malloc(MAX_CMD_SIZE);

	if (command == NULL) {
		perror("malloc");
		exit(1);
	}

    // 만약 현재 경로에 /tmp/test가 없다면 / 경로에 만들어주기
    DIR *dp;
    dp = opendir(REAL_PATH_CONSTANT);

    if (dp == NULL) {
        int result = mkdir("tmp", 0777);
        if (result == -1) {
            perror("mkdir error");
            exit(1);
        }
        result = chdir("tmp");
        if (result == -1) {
            perror("chdir error");
            exit(1);
        }
        result = mkdir("test", 0777);
        if (result == -1) {
            perror("mkdir error");
            exit(1);
        }
        printf("Success make initial directory(/tmp/test)\n");

    }

    // 현재 경로를 /tmp/test로 바꿔주기
    int result = chdir(REAL_PATH_CONSTANT);

    if (result == -1) {
        perror("chdir error");
        exit(1);
    }

	do {
		printf("%s $ ", current_dir);
        
		if (fgets(command, MAX_CMD_SIZE-1, stdin) == NULL) break;

		tok_str = strtok(command, " \n");
        if (tok_str == NULL) continue;

		if (strcmp(tok_str, "quit") == 0) {
			break;
        
        } else if (strcmp(tok_str, "help") == 0) {
            printf("------------------------------\n");
            printf("help: print all commands\n");
            printf("cd <path>: change directory\n");
            printf("mkdir: make directory\n");
            printf("rmdir: remove directory\n");
            printf("rename: rename file\n");
            printf("ls: list files\n");
            printf("quit: exit program\n");
            printf("------------------------------\n");            
        } else if (strcmp(tok_str, "cd") == 0) {
            tok_str = strtok(NULL, " \n");

            // tok_str이 ~이면 /tmp/test로 바꿔주기
            if (strcmp(tok_str, "~") == 0 || strcmp(tok_str, "/") == 0) {                
                int result = chdir(REAL_PATH_CONSTANT);
                strcpy(current_dir, "/");
                if (result == -1) {
                    perror("chdir error");
                    exit(1);
                }
                continue;  
            } 
            int result = chdir(tok_str);

            if (result == -1) {
                perror("chdir error");
                exit(1);
            }

            real_path = getcwd(NULL, BUFSIZ);
            char *position = strstr(real_path, "/tmp/test");
            
            if (position != NULL) {
                // target_path의 위치 계산
                int index = position - real_path;
                // printf("/tmp/test의 위치: %d\n", index);

                // real_path에서 index+8 위치부터 끝까지를 target_path로 복사
                char *target_path = (char*) malloc(MAX_CMD_SIZE);
                strcpy(target_path, real_path + index + 9);
                

                // current_dir에 target_path 복사
                if (strcmp(target_path, "") == 0) {
                    // current_dir = "/";
                    strcpy(current_dir, "/");
                } else {
                    // current_dir = target_path;
                    strcpy(current_dir, target_path);
                }
                
                free(target_path);
                
                printf("Success change directory\n");

            } else {
                printf("Cannot change directory\n");
                strcpy(current_dir, "/");
                chdir(REAL_PATH_CONSTANT);
                // real_path도 되돌리기
                real_path = REAL_PATH_CONSTANT;
            }

        } else if (strcmp(tok_str, "pwd") == 0) {
            printf("%s \n", current_dir);
            
        } else if (strcmp(tok_str, "mkdir") == 0) {
            tok_str = strtok(NULL, " \n");

            // tok_str이 " "이면 missing operand
            if (tok_str == NULL) {
                perror("missing operand");
                continue;
            }

            char *path = make_path(tok_str);

            result = mkdir(path, 0777);
            if (result == -1) {
                perror("mkdir error");
                exit(1);
            }
            free(path);
            printf("Success make directory\n");
            
        } else if (strcmp(tok_str, "rmdir") == 0) {
            tok_str = strtok(NULL, " \n");

            // tok_str이 " "이면 missing operand
            if (tok_str == NULL) {
                perror("missing operand");
                continue;
            }

            char *path = make_path(tok_str);

            result = rmdir(path);
            if (result == -1) {
                perror("rmdir error");
                exit(1);
            }
            free(path);
            printf("Success remove directory\n");
        } else if (strcmp(tok_str, "rename") == 0) {
            tok_str = strtok(NULL, " \n");
            char *old_name = tok_str;

            tok_str = strtok(NULL, " \n");
            char *new_name = tok_str;

            int result = rename(old_name, new_name);

            if (result == -1) {
                perror("rename error");
                exit(1);
            }

            printf("Success rename file\n");
        } else if (strcmp(tok_str, "ls") == 0) {
            DIR * dp;
            struct dirent *dent;

            dp = opendir(".");

            if (dp == NULL) {
                perror("opendir error");
                exit(1);
            }

            printf("File name           File type    File size\n");
            printf("------------------------------------------------\n");

            while ((dent = readdir(dp))) {
                printf("%-20s", dent->d_name);
                if (dent->d_type == DT_DIR) {
                    printf("%-13s", "directory");
                } else if (dent->d_type == DT_REG) {
                    printf("%-13s", "regular");
                } else {
                    printf("%-13s", "unknown");
                }
                printf("%-10d\n", dent->d_reclen);

                
            }
            closedir(dp);
            printf("Success list files\n");
        }

		else {
            // 존재하지 않는 커맨드
            printf("No such command\n");		
		}
	} while (1);

	free(command);

	return 0;
}