#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define SERVER "./Server_pipe"

int main()
{
    int client_fd, server_fd;
    char inmsg[80];
    char outmsg[80];

    // "Client_pipe_" + pid 를 만들기
    char CLIENT_PIPE[50];
    pid_t pid = getpid();

    snprintf(CLIENT_PIPE, sizeof(CLIENT_PIPE), "./Client_pipe_%d", pid);

    // 클라이언트 파이프 생성
    if (mkfifo(CLIENT_PIPE, 0666) == -1)
    {
        perror("mkfifo client pipe");
        exit(1);
    }

    // 서버 파이프 열기
    if ((server_fd = open(SERVER, O_WRONLY)) == -1)
    {
        perror("open to write");
        exit(1);
    }

    // Connect 메시지 보내기
    snprintf(outmsg, sizeof(outmsg), "#%s", CLIENT_PIPE);
    if (write(server_fd, outmsg, strlen(outmsg) + 1) == -1)
    {
        perror("write");
        exit(1);
    }

    // 클라이언트 파이프 열기
    if ((client_fd = open(CLIENT_PIPE, O_RDONLY)) == -1)
    {
        perror("open to read");
        exit(1);
    }

    while (1)
    {
        // 자식 프로세스는 읽고 부모 프로세스는 쓴다.
        switch (fork())
        {
        case -1:
            perror("fork");
            exit(1);
        case 0:
            // 자식 프로세스는 읽는다.
            if (read(client_fd, inmsg, sizeof(inmsg)) > 0)
            {
                printf("\nReceived from server: %s\n", inmsg);
            }
            break;
        default:
            // 부모 프로세스는 쓴다.
            printf("Send to server: ");
            fgets(outmsg, sizeof(outmsg), stdin);
            if (write(server_fd, outmsg, strlen(outmsg) + 1) == -1)
            {
                perror("write");
                exit(1);
            }
            wait(NULL);
            break;
        }
    }

    close(client_fd);
}