#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include <unistd.h>
#include <errno.h>

#define BUFSIZE 1000000

unsigned short checksum2(const char *buf, unsigned size);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
ssize_t rio_readn(int fd, void *usrbuf, size_t n);

int main(int argc, char **argv) {
    uint8_t op, shift;
    uint32_t length;
    int i, port, sock;
    char host[16];
    unsigned char *message;
    unsigned char *read_message;
    unsigned char *str_buffer;
    int str_len1, str_len2;
    struct sockaddr_in server_addr;
    unsigned short check_sum;

    message = (unsigned char *) malloc(sizeof(unsigned char) * BUFSIZE);
    memset(message, 0, BUFSIZE);
    read_message = (unsigned char *) malloc(sizeof(unsigned char) * BUFSIZE);
    memset(read_message, 0, BUFSIZE);
    str_buffer = (unsigned char *) malloc(sizeof(unsigned char) * (BUFSIZE-8));
    memset(str_buffer, 0, BUFSIZE-8);

    if(argc != 9){
        fprintf(stderr, "invalid argument\n");
        exit(1);
    }

    for (i=1;i<argc;i++) {
        if (argv[i][0] == '-') {
            switch(argv[i][1]) {
                case 'h':  
                    strcpy(host, argv[i+1]); 
                    break;
                case 'p':
                    port = atoi(argv[i+1]);
                    break;
                case 'o':
                    op = (uint8_t) atoi(argv[i+1]);
                    break;
                case 's':
                    shift = (uint8_t) atoi(argv[i+1]);
                    break;
                default:
                    fprintf(stderr, "invalid option\n");
                    exit(1);
            }
        } else {
            continue;
        } 
    }
    if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{//소켓 생성과 동시에 소켓 생성 유효검사
		fprintf(stderr, "can't create socket\n");
		exit(0);
	} 

	server_addr.sin_family = AF_INET;
	//주소 체계를 AF_INET 로 선택
	server_addr.sin_addr.s_addr = inet_addr(host);
	//32비트의 IP주소로 변환
	server_addr.sin_port = htons(port);
	//daytime 서비스 포트 번호 
    
	if(connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{//서버로 연결요청
		fprintf(stderr, "can't connect.\n");
		exit(1);
	}

    fprintf(stderr, "connected\n");


    while(1) {
        //represent and assemble data as the protocol says.
        memcpy(message, &op, 1);
        memcpy(message+1, &shift, 1);
        if (fread(str_buffer, 1, BUFSIZE-8, stdin) <= 0) { 
            break;
        }
        //if (fgets(str_buffer, BUFSIZE-8, stdin) == NULL) { 
        length = htonl((uint32_t)strlen(str_buffer)+8);
        memcpy(message+4, &length, 4);
        memcpy(message+8, str_buffer, strlen(str_buffer));
        unsigned short check_sum = checksum2(message, ntohl(length));
        memcpy(message+2, &check_sum, 2);

        /*
        printf("message 4byte : %x\n", *(uint32_t *)message);
        printf("message 8byte : %x\n", *(uint32_t *)(message+4));
        printf("message 12byte : %s\n", message+8);
        */
        
        //printf("double checksum : %u\n", checksum2(message, ntohl(length)));

        rio_writen(sock, message, ntohl(length));
        str_len2 = rio_readn(sock, read_message, ntohl(length));
        printf("%s", read_message+8);
        memset(message, 0, BUFSIZE);
        memset(str_buffer, 0, BUFSIZE-8); 
        memset(read_message, 0, BUFSIZE);
    }

    close(sock);

    return 0;
}

unsigned short checksum2(const char *buf, unsigned size)
{
    unsigned long long sum = 0;
    const unsigned long long *b = (unsigned long long *) buf;

    unsigned t1, t2;
    unsigned short t3, t4;

    /* Main loop - 8 bytes at a time */
    while (size >= sizeof(unsigned long long))
    {
        unsigned long long s = *b++;
        sum += s;
        if (sum < s) sum++;
        size -= 8;
    }

    /* Handle tail less than 8-bytes long */
    buf = (const char *) b;
    if (size & 4)
    {
        unsigned s = *(unsigned *)buf;
        sum += s;
        if (sum < s) sum++;
        buf += 4;
    }

    if (size & 2)
    {
        unsigned short s = *(unsigned short *) buf;
        sum += s;
        if (sum < s) sum++;
        buf += 2;
    }

    if (size)
    {
        unsigned char s = *(unsigned char *) buf;
        sum += s;
        if (sum < s) sum++;
    }

    /* Fold down to 16 bits */
    t1 = sum;
    t2 = sum >> 32;
    t1 += t2;
    if (t1 < t2) t1++;
    t3 = t1;
    t4 = t1 >> 16;
    t3 += t4;
    if (t3 < t4) t3++;

    return ~t3;
}
    
/* $begin rio_writen */
ssize_t rio_readn(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
    if ((nread = read(fd, bufp, nleft)) < 0) {
        if (errno == EINTR) /* Interrupted by sig handler return */
        nread = 0;      /* and call read() again */
        else
        return -1;      /* errno set by read() */ 
    } 
    else if (nread == 0)
        break;              /* EOF */
    nleft -= nread;
    bufp += nread;
    }
    return (n - nleft);         /* Return >= 0 */
}

ssize_t rio_writen(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
    if ((nwritten = write(fd, bufp, nleft)) <= 0) {
        if (errno == EINTR)  /* Interrupted by sig handler return */
        nwritten = 0;    /* and call write() again */
        else
        return -1;       /* errno set by write() */
    }
    nleft -= nwritten;
    bufp += nwritten;
    }
    return n;
}
