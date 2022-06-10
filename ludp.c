

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>

#define IP_PORT_DELIMITER ":"
#define DEFAULT_MESS_SIZE 1024

#define SIGNALS_BLOCK SIGINT // | SIGTERM

int slocal_fd;

volatile int exit_flag = 0;

int check_to_int(const char *str);
int config_signal(void (*s_handler) (int, siginfo_t *, void *), int sig_number, ...);

void exit_handler(int signum, siginfo_t *si, void *v);


int main(int argc, char** argv) {

    if (argc < 2) {
        fprintf(stderr, "Error: more arguments are needed\n");
        exit(-1);
    }

    /* saving a pointer to a string storing our port */
    char *port_ptr = strstr(argv[1], IP_PORT_DELIMITER);
    if (port_ptr == NULL) {
        fprintf(stderr, "Error: no port was specified\n");
        exit(-1);
    } else
        port_ptr++;

    /* checking the entered buffer size */
    size_t mess_size = 0;
    if (argc > 2) {
        if (check_to_int(argv[2]) < 0)
            mess_size = DEFAULT_MESS_SIZE;
        else {
            int ms = atoi(argv[2]);
            if (ms < 0) {
                fprintf(stderr, "Error: the buffer size cannot be less 0 \n");
                exit(-1);
            }
            else
                mess_size = ms;
        }
    } else
        mess_size = DEFAULT_MESS_SIZE;


    struct sockaddr_in servout,
                        servlocal;


    bzero(&servlocal, sizeof(servlocal));
    servlocal.sin_family = AF_INET;
    servlocal.sin_port = htons(atoi(port_ptr));

    if (argv[1][0] == IP_PORT_DELIMITER[0]) {
        printf("ip not exist\n");
        servlocal.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else {
        if (inet_pton(AF_INET, argv[1], &servlocal.sin_addr) < 0) {
            fprintf(stderr, "Error ip convert: %s", strerror(errno));
            exit(-1);
        }
    }


    if ((slocal_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "Error sockect: %s", strerror(errno));
        exit(-1);
    }
    fcntl(slocal_fd, F_SETFL, O_NONBLOCK);


    if (bind(slocal_fd, (struct sockaddr *)&servlocal, sizeof(servlocal)) < 0) {
        fprintf(stderr, "Error bind: %s", strerror(errno));
        close(slocal_fd);
        exit(-1);
    }

    if (config_signal(exit_handler, 2, SIGINT, SIGTERM) == -1) {
        fprintf(stderr, "Error set signals proccess exit: %s", strerror(errno));
        close(slocal_fd);
        exit(-1);
    }

    size_t byte_size;
    char recv_mess[mess_size];

    socklen_t servout_len = sizeof servout;
    char ipstr[INET6_ADDRSTRLEN];
    void *paddr;

    fd_set recieve_fds;
    FD_ZERO(&recieve_fds);
    FD_SET(slocal_fd, &recieve_fds);


    while (1) {
        select(slocal_fd + 1, &recieve_fds, NULL, NULL, NULL);

        if (exit_flag) {
            close(slocal_fd);
            exit(0);
        } else {
            if ((byte_size = recvfrom(slocal_fd, recv_mess, mess_size, 0, (struct sockaddr *)&servout, &servout_len)) > 0) {
                paddr = &servout.sin_addr;
                inet_ntop(servout.sin_family, paddr, ipstr, sizeof ipstr);
                printf("from host [%s:%d]:\n[message]%s[message]\n", ipstr, ntohs(servout.sin_port), recv_mess);
            }
        }
    }

}


int check_to_int(const char *str) {
    size_t slen = strlen(str);
    int dot_flag = 0;

    size_t i = 0;
    if (str[0] == '-')
        i = 1;

    for(; i < slen; i++)
        if (!(str[i] >= '0' && str[i] <= '9')) {
            if (str[i] == '.') {
                if (dot_flag)
                    return -1;
                else
                    dot_flag = 1;
            } else {
                return -1;
            }
        }

    return 0;
}


int config_signal(void (*s_handler) (int, siginfo_t *, void *), int sig_number, ...) {
    if (s_handler == NULL)
        return 0;

    struct sigaction sa;
    bzero(&sa, sizeof(sa));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = s_handler;

    va_list vptr;
    va_start(vptr, sig_number);

    for (int i = 0; i < sig_number; i++) {
        if (sigaction(va_arg(vptr, int), &sa, NULL) == -1)
            return -1;
    }

    va_end(vptr);

    return 0;

}

void exit_handler(int signum, siginfo_t *si, void *v) {
    exit_flag = 1;
}




