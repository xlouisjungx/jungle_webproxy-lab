#include "csapp.h"

int main(int argc, char **argv) // 명령행 인자 개수 / 인자 문자열 배열
{
    int clientfd; // 서버와 통신할 클라이언트 소켓 파일 티스크립터
    char *host, *port, buf[MAXLINE]; // host, port는 연결할 서버의 주소 및 포트, buf는 메시지를 담을 버퍼
    rio_t rio; // rio_t 구조체, Robust I/O를 위한 버퍼링 구조체

    if(argc != 3) { // 인자가 총 3개 인지 확인한다. = 실행파일 + host + port
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1]; // host 값 추출
    port = argv[2]; // port 값 추출

    clientfd = Open_clientfd(host, port); // 서버 연결 시도
    Rio_readinitb(&rio, clientfd); // 

    while(Fgets(buf, MAXLINE, stdin) != NULL) { // 사용자 입력을 buf에 읽음
        Rio_writen(clientfd, buf, strlen(buf)); // 서버로 메시지를 전송
        Rio_readlineb(&rio, buf, MAXLINE); //서버의 응답을 읽어 buf에 저장
        Fputs(buf, stdout); // buf에 읽은 응답 출력
    }
    Close(clientfd);
    exit(0);
}