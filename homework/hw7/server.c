#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SERVER "./Server_pipe"

int main()
{
    // client_fd_list: client_fd를 저장하는 배열
    int client_fd_list[10];

    int server_fd, client_fd;
    char msg[80];

    printf(" === Server === \n");
    // 서버 파이프 생성
    if (mkfifo(SERVER, 0666) == -1)
    {
        perror("mkfifo server pipe");
        exit(1);
    }

    // 서버 파이프 열기
    if ((server_fd = open(SERVER, O_RDONLY)) == -1)
    {
        perror("open to read");
        exit(1);
    }

    // 클라이언트에서 종료하면 Disconnect 출력
    while (1)
    {
        // 메시지 읽기
        if (read(server_fd, msg, sizeof(msg)) > 0)
        {
            // 메시지가 #으로 시작하면
            if (msg[0] == '#')
            {
                // 메시지가 #./Client_pipe_pid 형식이면
                // client_fd_list에 fd를 추가한다.
                memmove((char *)msg, msg + 1, strlen(msg));

                if ((client_fd = open(msg, O_WRONLY)) == -1)
                {
                    perror("open client pipe to write");
                    exit(1);
                }
                else
                {
                    client_fd_list[client_fd] = client_fd;
                }
                printf("[Connected] %s\n", msg);
            }
            else
            {
                // 메시지가 connect: Client_pipe_1234 형식이 아니면
                // client_fd_list에 있는 모든 fd에 메시지를 전송한다.
                for (int i = 0; i < 10; i++)
                {
                    if (client_fd_list[i] != 0)
                    {
                        write(client_fd_list[i], msg, strlen(msg) + 1);
                    }
                }
            }
        }
    }

    close(server_fd);
    close(client_fd);
}
