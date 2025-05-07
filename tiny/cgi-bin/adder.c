/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p; // buf: 전체 쿼리 문자열을 받을 포인터 (QUERY_STRING 환경 변수) / p: 위치를 찾는 데 사용
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE]; // arg1, arg2: 문자열로 된 숫자 파라미터 / content: HTML 본문을 저장할 버퍼
  int n1=0, n2=0;

  if((buf = getenv("QUERY_STRING")) != NULL) {
    p = strchr(buf, '&'); // & 위치 찾기
    *p = '\0'; // 첫 번째 인자를 '\0'로 끊기
    strcpy(arg1, buf); // 첫 번째 숫자 복사
    strcpy(arg2, p+1); // 두 번째 숫자 복사
    n1 = atoi(arg1); // 문자열 -> 정수
    n2 = atoi(arg2);
  }

  // HTML 문자열을 누적해서 content에 저장 
  // sprintf를 반복해서 누적 문자열 생성
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sThe Internet addition portal. \r\n<p>", content);
  sprintf(content, "%sThe answer is : %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  // CGI 프로그램은 반드시 HTTP 헤더를 직접 출력해야 함
  // content-length는 브라우저가 응답을 정확히 읽는데 필요
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout); // stdout은 클라이언트에 보내는 응답 스트림이므로 반드시 flush 필요

  exit(0); // 종료
}
/* $end adder */
