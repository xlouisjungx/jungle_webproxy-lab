#include "csapp.h"

void echo(int connfd);

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if(argc != 2) { // 올바르게 입력하였는지 검사 = 실행파일 + 포트번호
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]); // 수신 대기 상태로 전환

    while(1) { // 연결 요청이 들어오면 수락하고, 클라이언트 주소 정보를 clientaddr에 저장함.
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        // 접속한 클라이언트의 호스트명=IP와 포트를 문자열로 변환해 출력.
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s %s)\n", client_hostname, client_port);

        // echo함수를 통해 실제 데이터를 송수신하고 종료.
        echo(connfd);
        Close(connfd);
    }
    exit(0);
}

// 클라이언트로부터 문자열을 받고, 그대로 다시 보내는 역함을 하는 함수.
void echo(int connfd) {
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd); // connfd를 기반으로 RIO 버퍼를 초기화. (RIO는 버퍼 기반 안전 입출력.)
    while((n= Rio_readlineb(&rio, buf, MAXLINE)) != 0) { // 클라이언트가 보낸 문자열을 읽어 출력한 뒤, 그대로 다시 전송 = 에코.
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
    } // 클라이언트가 EOF 전송 시 종료.
}