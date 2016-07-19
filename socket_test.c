/*
 * BiffSocko & possibly someone else - I can't remember
 * server.c
 * this is a tcp/ip server template.  It has all the basics of a tcp server
 * that only needs slight modifications to be integrated to any server program.
 * server.c will basically bind/listen/accept/fork/kill child processes/etc.
 * all you have to do is plug in your read/write functions in the fork() section
 * of child processes, located below.
 *
 * Exit Codes
 * 0    Success
 * 1    Wrong Command Line Parameters
 * 2    Unable to open socket()
 * 3    Can't bind() to port
 * 4    Unable to listen() to port
 * 5    Can't accept() connections
 * 6    Can't fork()
 * 99   SIGINT
 * all other abnormal return exit codes from connection functions which do the
 * recev/send stuff for this program
 *
 * COMPILE:
 * gcc -lpthread -o socket_test  socket_test.c
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <pthread.h>

#define PORT 2030
#define BACKLOG 200
#define NUM 1024

/*****************************************************************************
 *              Global Variables (yucky)
 *****************************************************************************/
int sd;                         /* socket discriptor for socket function */

/*****************************************************************************
 *              Function Definitions
 *****************************************************************************/
void closer(int);                             /* this will handle SIGINT */
void *client_function(void *sconPtr);         /* function for the thread */
/*****************************************************************************
 * Structure definition (needed to pass the accept() descriptor)
 *****************************************************************************/
struct serverCon {
           int connectionDesc;
};


int main (int argc, char *argv[])
{
        struct sockaddr_in home_addr;          /* network address structures */
        pthread_t *client;                     /* new client connections thread */
        int con;                               /* number of connections */
        int accept_sd;                         /* socket discriptor for accept function */
        unsigned int alen;                     /* from adderlen */
        struct sockaddr_in connecting_addr;    /* network address structures */
        struct serverCon *sconPtr;             /* pointer to the accept() socket struct */

        /*****************************************************************************
         * check for correct usage
         *****************************************************************************/
        if(argc != 1){
                fprintf(stderr, "%s does not accept arguments\n", argv[0]);
                exit(1);
        }

        /*****************************************************************************
         * open socket
         *****************************************************************************/
        if(!(sd=socket(AF_INET, SOCK_STREAM, 0))){
                fprintf(stderr, "%s is unable to create a socket. exiting\n", argv[0]);
                exit(2);
        }

        /*****************************************************************************
         * do the sockaddr_in thing for home_addr
         *****************************************************************************/
        home_addr.sin_family = AF_INET;
        home_addr.sin_port = htons(PORT);
        home_addr.sin_addr.s_addr = INADDR_ANY;
        bzero(&(home_addr.sin_zero), 0);

        /*****************************************************************************
         * binding to port
         *****************************************************************************/
        if((bind(sd, (struct sockaddr *)&home_addr, sizeof(struct sockaddr))) == -1){
                fprintf(stderr, "%s could not bind to port. exiting\n", argv[0]);
                close(sd);
                exit(3);
        }

        printf("network server running on port %d\n", PORT);

        /*****************************************************************************
         * listen to the port
         *****************************************************************************/
        if((listen(sd, BACKLOG)) == -1){
                fprintf(stderr, "%s unable to listen to port. exiting\n", argv[0]);
                close(sd);
                exit(4);
        }

        /*****************************************************************************
         * clean up runaway processes & catch signals for closing out nicely
         *****************************************************************************/
         signal(SIGCHLD, SIG_IGN);
         signal(SIGINT, closer);

        /*****************************************************************************
         * this is the part that does the work, we will accept() the connection,
         * and then fork() off and run the *connection functions*
         *****************************************************************************/
        con=0;
        /* hey save some space for me!! */
        client = malloc (NUM * sizeof *client);
        while(1){
                /* accept the connecting socket */
                alen=sizeof(connecting_addr);
                if(!(accept_sd=accept(sd, (struct sockaddr *) &connecting_addr, &alen))){
                        fprintf(stderr, "program is unable to accept connections. exiting\n");
                        close(sd);
                        exit(5);
                }

                /* start a snazzy new thread */
                sconPtr = (struct serverCon *)malloc(sizeof(struct serverCon));
                sconPtr->connectionDesc = accept_sd;
                 if((pthread_create( &client[con],NULL,client_function,(void *)sconPtr))!= 0){
                        printf("can't create new thread .. exiting\n");
                        close(sd);
                        exit(1);
                }

                pthread_detach( client[con]);
                /*pthread_join( client[con],NULL); */
                con++;
        }
        exit(0);
}

/*****************************************************************************
 * client_function()
 * does the reading and writing for the network connection
 *****************************************************************************/
void *client_function(void *sconPtr){
#define PARMPTR ((struct serverCon *) sconPtr)
        char *buffer=malloc(NUM);

        read(PARMPTR->connectionDesc, buffer, NUM);
        write(PARMPTR->connectionDesc,buffer,strlen(buffer));

        close(PARMPTR->connectionDesc);
        memset(buffer,0,NUM);
        free(PARMPTR);
        free(buffer);
        return NULL;
}

/*****************************************************************************
 *  closer()
 * this will take care of closing up the master socket descriptor upon
 * SIGINT or other signals.  Eventually, this will also log termination
 * errors to syslog
 *****************************************************************************/
void closer(int SIGX)
{

        printf("exiting on Signal %d\n", SIGX);
        close(sd);
        exit(99);
}
