/*
 * @file imgfs_server.c
 * @brief ImgFS server part, main
 *
 * @author Konstantinos Prasopoulos
 */

#include "util.h"
#include "imgfs.h"
#include "http_net.h"
#include "imgfs_server_service.h"
#include <signal.h>
#include <stdlib.h> // abort()

/********************************************************************/
static void signal_handler(int sig_num _unused)
{
    server_shutdown();
    exit(0);
}

/********************************************************************/
static void set_signal_handler(void)
{
    struct sigaction action;
    if (sigemptyset(&action.sa_mask) == -1) {
        perror("sigemptyset() in set_signal_handler()");
        abort();
    }
    action.sa_handler = signal_handler;
    action.sa_flags   = 0;
    if ((sigaction(SIGINT,  &action, NULL) < 0) ||
        (sigaction(SIGTERM,  &action, NULL) < 0)) {
        perror("sigaction() in set_signal_handler()");
        abort();
    }
}

/*********************************************************************/


int main (int argc, char *argv[])
{

    set_signal_handler();
    int err = server_startup(argc, argv);

    if (err < 0) {
        fprintf(stderr, "http_init() failed\n");
        fprintf(stderr, "%s\n", ERR_MSG(err));
        server_shutdown();
        return err;
    }


    //loop on http_receive() as long as there are no error
    while ((err = http_receive()) == ERR_NONE);
    fprintf(stderr, "http_receive() failed\n");
    fprintf(stderr, "%s\n", ERR_MSG(err));

    server_shutdown();
    return err;
}
