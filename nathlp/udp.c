/* Sample UDP server */

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>


int main(int argc, char**argv)
{
  int sockfd,n;
  struct sockaddr_in servaddr,cliaddr;
  socklen_t len;
  char mesg[1000];
  int rc;

  sockfd=socket(AF_INET,SOCK_DGRAM,0);

  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
  servaddr.sin_port=htons(32000);
  // bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

  // client
  bzero(&cliaddr, sizeof(cliaddr));
  cliaddr.sin_family = AF_INET;
  // cliaddr.sin_addr.s_addr = htonl("81.4.106.67"); // htonl函数不可靠啊
  inet_pton(PF_INET, "81.4.106.67", &cliaddr.sin_addr.s_addr);
  cliaddr.sin_port = htons(32000);

  // rc = connect(sockfd, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
  // printf("connect to server :%d\n", rc);

  for (;;) {
      bzero(&cliaddr, sizeof(cliaddr));
      cliaddr.sin_family = AF_INET;
      // cliaddr.sin_addr.s_addr = htonl("81.4.106.67");
      inet_pton(PF_INET, "81.4.106.67", &cliaddr.sin_addr.s_addr);
      cliaddr.sin_port = htons(32000);

      len = sizeof(cliaddr);
      strcpy(mesg, "register;opc;;\n");
      rc = sendto(sockfd, mesg, strlen(mesg), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
      // rc = write(sockfd, mesg, strlen(mesg));
      // rc = send(sockfd, mesg, strlen(mesg), 0);
      printf("-------------------------------------------------------%d,%d, %s\n", n, rc, strerror(errno));
      n = recvfrom(sockfd, mesg, 1000, 0,(struct sockaddr *)&cliaddr,&len);
      printf("-------------------------------------------------------%d,%d, %s\n", n, rc, strerror(errno));
      mesg[n] = 0;
      printf("Received the following:\n");
      printf("%s",mesg);
      printf("-------------------------------------------------------\n");
    }
}
