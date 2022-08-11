



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>




#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(dll, "ws2_32.dll")
//#pragma comment(lib, "ws2_32")

typedef SOCKET socket_t;
typedef int socksize_t;
typedef SOCKADDR sockaddr_t;
typedef SOCKADDR_IN sockaddr_in_t;

#else

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

typedef int socket_t;
typedef socklen_t socksize_t;
typedef struct sockaddr sockaddr_t;
typedef struct sockaddr_in sockaddr_in_t;

#endif /* _WIN32 */





#define IP_PORT_DELIMITER ":"
#define DEFAULT_MESS_SIZE 1024

//#define SIGNALS_BLOCK SIGINT /* | SIGTERM */

#define INET_ADDRLEN 46





int check_to_int(const char *str);
int config_signal(void (*s_handler) (int), int sig_number, ...);
void err_exit(socket_t sck, int eval);

#ifdef _WIN32
BOOL exit_handler(DWORD ctrlevnt);
#else
void exit_handler(int signum);
#endif // _WIN32


#define def_eprintf(format, ...) \
            fprintf(stderr, format, __VA_ARGS__ + 0);

#define def_err_exit(sck, eval, format, ...) { \
            fprintf(stderr, format, __VA_ARGS__ + 0); \
            err_exit(sck, eval); }



socket_t slocal_fd;


/* сокет, для отправки данных, на главный сокет, с целью завершить работу программы
 * (только для windows)
 */
#ifdef _WIN32
socket_t sexit_fd;
#endif // _WIN32

sockaddr_in_t servout;
sockaddr_in_t servlocal;


#ifdef _WIN32
WSADATA wsaData;
#endif

volatile int exit_flag = 0;



int main(int argc, char** argv) {

//#ifdef _WIN32
//    if ( WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//        fprintf(stderr, "WSAStartup error: %d\n", WSAGetLastError());
//        Sleep(2000);
//        exit(-1);
//    }
//#endif // _WIN32
//
//
//    HANDLE rdEvent;
//    rdEvent = WSACreateEvent();
//    printf("OK cras\n");
//    Sleep(3000);
//    return 0;

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

    printf("ptr port: %s\n", port_ptr);

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



#ifdef _WIN32
    if ( WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup error: %d\n", WSAGetLastError());
        Sleep(2000);
        exit(-1);
    }
#endif // _WIN32


    memset(&servlocal, 0, sizeof(servlocal));
    servlocal.sin_family = AF_INET;
    servlocal.sin_port = htons(atoi(port_ptr));

    if ( argv[1][0] == *IP_PORT_DELIMITER ) {
        printf("ip not exist\n");
        servlocal.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else {

        size_t ipplen = strlen(argv[1]);
        char *pch = argv[1];
        size_t i = 0;
        for (; i < ipplen; i++) {
            if (*pch == *IP_PORT_DELIMITER)
                break;
            pch++;
        }

        char ipstr_addr[i + 1];
        memcpy(ipstr_addr, argv[1], i);
        ipstr_addr[i] = 0;
        printf("ptr ip: %s\n", ipstr_addr);
        if (
        #ifdef _WIN32
            (servlocal.sin_addr.s_addr = inet_addr(ipstr_addr)) == INADDR_NONE
        #else
            inet_pton(AF_INET, ipstr_addr, &servlocal.sin_addr) < 0
        #endif /* _WIN32 */
            ) {
            fprintf(stderr, "Error ip convert: %s\n", strerror(errno));
            exit(-1);
        }
    }


    slocal_fd = socket(AF_INET, SOCK_DGRAM, 0);

    #ifdef _WIN32
    sexit_fd = socket(AF_INET, SOCK_DGRAM, 0);
    #endif // _WIN32

    if (
#ifdef _WIN32
        slocal_fd == INVALID_SOCKET
#else
        slocal_fd < 0
#endif // _WIN32
        ) {

        fprintf(stderr, "Error sockect: %s\n", strerror(errno));
        exit(-1);
    }
//    fcntl(slocal_fd, F_SETFL, O_NONBLOCK);




    printf("greg\n");
    if ( bind(slocal_fd, (sockaddr_t *)&servlocal, (socksize_t) sizeof(servlocal))
                #ifdef _WIN32
                    == SOCKET_ERROR
                #else
                    < 0
                #endif /* _WIN32 */
                )  {
            fprintf(stderr, "Error ip convert: %s\n", strerror(errno));
            def_err_exit(slocal_fd, -1, "Error bind: %s\n", strerror(errno));
    }

    printf("-%d-\n", WSAGetLastError());

    printf("greg\n");
#ifdef _WIN32
    /* clearing the previous signal handlers */
    if ( SetConsoleCtrlHandler(exit_handler, TRUE) == 0 )
        def_err_exit(slocal_fd, -1, "Error set console handler: error code: %d\n", GetLastError());
    printf("set signal win OK\n");
#else
    if ( config_signal(exit_handler, 3, SIGINT, SIGTERM, SIGBREAK) == -1 )
        def_err_exit(slocal_fd, -1, "Error set signals proccess exit: %s\n", strerror(errno));
#endif


    size_t byte_size;
    char recv_mess[mess_size];

    socksize_t servout_len = (socksize_t) sizeof(servout);
    char ipstr[INET_ADDRLEN];
    void *paddr = NULL;


#ifdef _WIN32
    WSAEVENT eventArr[1];
    WSAEVENT rdEvent;
    WSANETWORKEVENTS netevents;
    DWORD eventTotal = 1;

    rdEvent = WSACreateEvent();
    printf("OK cras\n");

    if (rdEvent == WSA_INVALID_EVENT) {
        printf("ERROR#@#@\n");
        Sleep(1000);
        def_err_exit(slocal_fd, -1, "Error creat event win: error code: %ld\n", WSAGetLastError());
    }


    if ( WSAEventSelect(slocal_fd, rdEvent, FD_READ | FD_ROUTING_INTERFACE_CHANGE ) == SOCKET_ERROR )
        def_err_exit(slocal_fd, -1, "Error event select win: error code: %ld\n", WSAGetLastError());

    *eventArr = rdEvent;
#else
    fd_set recieve_fds;
    FD_ZERO(&recieve_fds);
    FD_SET(slocal_fd, &recieve_fds);
#endif // _WIN32

    DWORD event;

    printf("fd settings OK\n");
    Sleep(1000);
    while (1) {
    #ifdef _WIN32
        if ( (event = WSAWaitForMultipleEvents(eventTotal, eventArr, FALSE, WSA_INFINITE, FALSE)) == WSA_WAIT_FAILED ) {

            def_err_exit(slocal_fd, -1, "Error wait event win: error code: %ld\n", WSAGetLastError());
        } else
            printf("wait return %ld", event);

//        WSAResetEvent(*eventArr);
        if ( WSAEnumNetworkEvents(slocal_fd, *eventArr, &netevents) == SOCKET_ERROR )
            def_err_exit(slocal_fd, -1, "Error netwok events win: error code: %ld\n", WSAGetLastError());
    #else
        if (select(slocal_fd + 1, &recieve_fds, NULL, NULL, NULL) > 0) {

        } else {

        }
    #endif

        if (netevents.lNetworkEvents & FD_ROUTING_INTERFACE_CHANGE)
            printf("close socekt\n");

        Sleep(2000);

        if ( exit_flag ) {
            def_err_exit(slocal_fd, -1, "listen have been succesful stopped\n", "");
        } else {
            if ( (byte_size = recvfrom(slocal_fd, recv_mess, mess_size, 0, (sockaddr_t *)&servout, &servout_len)) > 0 ) {
                paddr = &servout.sin_addr;
            #ifdef _WIN32

            #else
                inet_ntop(servout.sin_family, paddr, ipstr, sizeof ipstr);
            #endif /* _WIN32 */

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


int config_signal(void (*s_handler) (int), int sig_number, ...) {
    if (s_handler == NULL)
        return 0;

    va_list vptr;
    va_start(vptr, sig_number);

    for (int i = 0; i < sig_number; i++) {
        if (signal(va_arg(vptr, int), s_handler) == SIG_ERR) {
            printf("error signal()\n");
            return -1;
        }
    }
    printf("signal set OK\n");
    va_end(vptr);

    return 0;
}

#ifdef _WIN32
BOOL exit_handler(DWORD ctrlevnt) {
    if (ctrlevnt == CTRL_C_EVENT || ctrlevnt == CTRL_BREAK_EVENT) {
        exit_flag = 1;

        printf("CTRL + C PRESSED\n");
        sendto(sexit_fd, NULL, 0, 0, (sockaddr_t *)&servlocal, sizeof(sockaddr_t));
        return TRUE;
    }

    return FALSE;
}
#else
void exit_handler(int signum) {
    exit_flag = 1;
}
#endif // _WIN32


void err_exit(socket_t sck, int eval) {


#ifdef _WIN32
    closesocket(sck);
    WSACleanup();
    Sleep(1000);
#else
    close(sck);
    sleep(1);
#endif
    printf("exit!!!\n");
    exit(eval);
}

