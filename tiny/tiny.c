/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

/*
-------------------------------------------------------
| 클라이언트 요청 → Accept() → doit()                     |
|                                                     |
| doit()                                              |
|  ├─ 읽기: 요청 라인 + 헤더                              |
|  ├─ URI 파싱 → 정적 or 동적                            |
|  ├─ 파일 존재 검사                                     |
|  ├─ serve_static() or serve_dynamic()               |
|  └─ 응답 전송 완료 → 소켓 닫기                           |
-------------------------------------------------------
|  이 코드의 전체 흐름은?                                  |
|   1. 클라이언트가 요청을 보냄. (GET /index.html HTTP/1.0) |
|   2. 서버가 요청을 읽고 파싱                              |
|   3. 요청에 맞는 파일 또는 CGI 실행                       |
|   4. 응답을 클라이언트에게 전송                            |
-------------------------------------------------------

*/

// 서버는 커맨드라인 인자로 포트 번호를 받아야 함.
// Open_listenfd(argv[1])으로 주어진 포트에 대해 리스닝 소켓 생성.
// 무한루프로 클라이언트 연결을 계속 수락.
int main(int argc, char **argv) { // 소켓 서버 생성 -> 클라이언트 연결 요청 수락 처리 -> 소켓 닫기 
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); // 포트에서 listen
  while (1) {
    clientlen = sizeof(clientaddr);

    // 쿨라이언트와 연결되면 doit() 함수로 HTTP 요청 처리를 맡기고, 처리 후 소켓 닫기.
    connfd = Accept(listenfd, (SA *)&clientaddr, // 클라이언트 연결 수락
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit // 요청 처리
    Close(connfd);  // line:netp:tiny:close // 연결 종료
  }
}

// 클라이언트 HTTP 요청을 처리하는 함수
void doit(int fd) {
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, fd);

  // HTTP 요청 라인 읽기: GET /index.html HTTP/1.0
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  if(strcasecmp(method, "GET")) {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);

  // 정적인지 동적인이 콘텐츠를 판단
  is_static = parse_uri(uri, filename, cgiargs); // URI 파싱 -> 정적이냐 동적이냐를 판단
  if(stat(filename, &sbuf) < 0) { // 파일이 존재하는지 확인

    // 잘못된 요청에 대해 HTML로 응답을 구성해 전송
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if(is_static) { // 존재하는 파일이 정적이면
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    // 정적 콘텐츠 처리 ex) HTML, 이미지 파일
    // 메모리에 파일 매핑(MMAP) -> 클라이언트에게 전송
    serve_static(fd, filename, sbuf.st_size);
  }
  else { // 존재하는 파일이 동적이면
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    // CGI 프로그램을 자식 프로세스로 fork해서 실행
    // QUERY_STRING 환경변수 설정
    // CGI 결과를 클라이언트에게 출력
    serve_dynamic(fd, filename, cgiargs);
  }
}

// 오류가 발생하게 되면 HTTP에서 에러 응답을 생성할 수 있도록 하는 함수
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
  char buf[MAXLINE], body[MAXBUF];

  sprintf(body, "<html><itle>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web Server</em>\r\n", body);

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

// 요청 헤더를 읽음 (\r\n이 나올때까지)
void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

// URI가 정적인지 동적인지 판단하고, CGI인자도 파싱
// /cgi-bin/이 포함되면 동적 요청
// 아니면 정적 파일로 요청
// 정적: /index.gtml -> ./index.html
// 동적: cgi-bin/add.cgi?x=1&y=2 -> filename = ./cgi-bin/add.cgi, cgiargs = x=1&y=2
int parse_uri(char *uri, char *filename, char *cgiargs) {
  char *ptr;

  if(!strstr(uri, "cgi-bin")) {
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if(uri[strlen(uri)-1] == '/') strcat(filename, "home.html");
    return 1;
  }
  else {
    ptr = index(uri, '?');
    if(ptr) {
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

// 파일을 메모리에 매핑하고(MMAP), HTTP 헤더와 파일 내용을 클라이언트에 보냄
void serve_static(int fd, char *filename, int filesize) {
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  get_filetype(filename, filetype); // Content-Type 결정
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers: \n");
  printf("%s", buf);

  srcfd = Open(filename, O_RDONLY, 0);

  // 파일을 mmap()으로 메모리에 매핑
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize); // 본문으로 응답 전송
  Munmap(srcp, filesize);
}

// 확장자를 기반으로 MIME 타입 설정
// .html -> text/html, .jpg -> image/jpeg 등
void get_filetype(char *filename, char *filetype) {
  if(strstr(filename, ".html")) strcpy(filetype, "text/html");
  else if(strstr(filename, ".gif")) strcpy(filetype, "image/gif");
  else if(strstr(filename, ".png")) strcpy(filetype, "image/png");
  else if(strstr(filename, ".jpg")) strcpy(filetype, "image/jpeg");
  else strcpy(filetype, "text/plain");
}

// CGI 프로그램 실행
// 환경 변수 설정 (QUERY_STRING)
// Dup2로 stdout을 클라이언트 소켓에 복제
// Execve로 CGI 프로그램 실행
void serve_dynamic(int fd, char *filename, char *cgiargs) {
  char buf[MAXLINE], *emptylist[] = { NULL };

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if(Fork() == 0) { // Fork() -> 자식 프로세스에서 환경 설정 및 execve()로 CGI 실행
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO); // 클라이언트 소켓을 stdout으로 바꾸기 -> CGI 출력이 클라이언트로 직접 감
    Execve(filename, emptylist, environ);
  }
  Wait(NULL);
}