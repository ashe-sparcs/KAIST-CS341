#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#define BUFSIZE 1000000

unsigned short checksum2(const char *buf, unsigned size);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
void caesar_shift(unsigned char *str_buffer, uint8_t op, uint8_t shift);

int main(int argc, char **argv) {
    int serv_sock;
    int str_len;
    uint8_t op, shift;
    uint32_t length;
    int i, port;
    char host[16];
    unsigned char *message;
    unsigned char *write_message;
    unsigned char *str_buffer;
    int str_len1, str_len2;
    unsigned short check_sum;

    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    unsigned int clnt_addr_size;

    fprintf(stderr, "start of server program\n");
    

    message = (unsigned char *) malloc(sizeof(unsigned char) * BUFSIZE);
    memset(message, 0, BUFSIZE);
    write_message = (unsigned char *) malloc(sizeof(unsigned char) * BUFSIZE);
    memset(write_message, 0, BUFSIZE);
    str_buffer = (unsigned char *) malloc(sizeof(unsigned char) * (BUFSIZE-8));
    memset(str_buffer, 0, BUFSIZE-8);

    if (strcmp(argv[1], "-p") || argc != 3) {
        fprintf(stderr, "invalid usage\n");
        exit(1);
    }

    if((serv_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{//소켓 생성과 동시에 소켓 생성 유효검사
		fprintf(stderr, "can't create server socket\n");
		exit(1);
	} 
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (bind(serv_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1){
		fprintf(stderr, "bind() error\n");
        exit(1);
    }
    
    if (listen(serv_sock, 5) == -1) {
        fprintf(stderr, "listen error\n");
        exit(1);
    }

    clnt_addr_size = sizeof(clnt_addr);
    int pid, new;
    static int counter = 0;

	while(1)	
	{
        fprintf(stderr, "in outer while loop\n");
        new = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);

        if ((pid = fork()) == -1)
        {
            fprintf(stderr, "pid is -1\n");
            close(new);
            continue;
        }
        else if(pid > 0)
        {
            fprintf(stderr, "pid is positive\n");
            close(new);
            counter++;
            fprintf(stderr, "counter : %d\n", counter);
            continue;
        }
        else if(pid == 0)
        {
            counter++;
            fprintf(stderr, "counter : %d\n", counter);
            int op, shift;
            while(1) {
                str_len2 = rio_readn(new, message, BUFSIZE);
                fprintf(stderr, "str_len2 : %d\n", str_len2);
                if (str_len2 <= 0) { 
                    fprintf(stderr, "broke\n");
                    break;
                }
                memcpy(&op, message, 1);
                memcpy(&shift, message+1, 1);
                memcpy(&length, message+4, 4);
                memcpy(str_buffer, message+8, ntohl(length)-8);
                unsigned short check_sum = checksum2(message, ntohl(length));
                if (check_sum != 0) {
                    fprintf(stderr, "corrupted\n");
                    close(new);
                    //continue;
                    break;
                }

                caesar_shift(str_buffer, op, shift);
                fprintf(stderr, "%s\n", str_buffer);

                
                memset(message+2, 0, 2);
                memset(message+8, 0, strlen(str_buffer));
                memcpy(message+8, str_buffer, strlen(str_buffer));
                unsigned short check_sum2 = checksum2(write_message, ntohl(length));
                memcpy(message+2, &check_sum2, 2);

                /*
                memcpy(write_message, &op, 1);
                memcpy(write_message+1, &shift, 1);
                memcpy(write_message+4, &length, 4);
                memcpy(write_message+8, str_buffer, strlen(str_buffer));
                memcpy(write_message+2, &check_sum2, ntohl(length));
                */
                printf("double checksum : %u\n", checksum2(message, ntohl(length)));

                printf("write_message 4byte : %x\n", *(uint32_t *)message);
                printf("write_message 8byte : %x\n", *(uint32_t *)(message+4));
                printf("write_message 12byte : %s\n", message+8);

                rio_writen(new, message, ntohl(length));
                memset(message, 0, BUFSIZE);
                memset(str_buffer, 0, BUFSIZE-8); 
                memset(write_message, 0, BUFSIZE);
            }
            close(new);
            break;
        }
	}
    close(serv_sock);
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

void caesar_shift(unsigned char *str_buffer, uint8_t op, uint8_t shift) {
    int i;

    shift = shift % ('z' - 'a' + 1);
    if (op == 1) {
        shift = -shift;
    }
    for (i=0; i < strlen(str_buffer); i++) {
        unsigned char str_buffer_i = str_buffer[i];
        if (str_buffer_i > 64 && str_buffer_i < 91) {
            str_buffer[i] = str_buffer_i + shift + 'a' - 'A';
        } else if (str_buffer_i > 96 && str_buffer_i < 123) {
            str_buffer[i] = str_buffer_i + shift;
        } else {
            
        }
    }    
}
