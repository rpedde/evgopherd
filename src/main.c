/*
 * Copyright (C) 2013 Ron Pedde (ron@pedde.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <pwd.h>
#include <grp.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <libdaemon/daemon.h>
#include <event.h>

#include "main.h"
#include "debug.h"

typedef struct client_t {
    int fd;
    int state;
    char *request;
    struct bufferevent *buf_ev;
} client_t;

/* Defines */
#define DEFAULT_CONFIGFILE "/etc/evgopherd.conf"
#define DEFAULT_DEBUGLEVEL 2

#define CLIENT_STATE_WAITING_REQUEST 0
#define CLIENT_STATE_WAITING_REPLY   1

#define MAX_REQUEST_SIZE 4096

/* Globals */
static int g_quitflag = 0;
gopher_conf_t config;

/* Forwards */
void handle_response(client_t *client);
static void handle_request(client_t *client);
static int setnonblock(int fd);
static int drop_privs(char *user);

/* finish off connection */
static void close_client(client_t *client);

/* read/write buffer events */
static void on_buf_error(struct bufferevent *bev, short what, void *arg);
static void on_buf_write(struct bufferevent *bev, void *arg);
static void on_buf_read(struct bufferevent *bev, void *arg);

/* signal and main socket events */
static void on_signal(int fd, short event, void *arg);      /* libdaemon signal fd */
static void on_accept(int fd, short event, void *arg);      /* server fd */
static void on_async_read(int fd, short event, void *arg);  /* ldap async pipe */

/**
 * print usage summary and exit
 *
 * @param name program name (from argv[0])
 */
void usage_quit(char *name) {
    fprintf(stderr, "Usage: %s [options]\n\n", name);
    fprintf(stderr, "Valid options:\n\n");
    fprintf(stderr, "  -d <level>        set debuglevel (0-5), 5 most verbose, default %d\n", DEFAULT_DEBUGLEVEL);
    fprintf(stderr, "  -c <configfile>   specifify a configfile.  (default: %s)\n", DEFAULT_CONFIGFILE);
    fprintf(stderr, "  -f                run in foreground (do not detach)\n");
    fprintf(stderr, "  -p <port>         port to listen on\n");
    fprintf(stderr, "  -s <dir>          directory to serve\n");
    fprintf(stderr, "  -k                kill running daemon\n");

    fprintf(stderr,"\n\n");

    exit(EXIT_FAILURE);
}

/**
 * drop privs to the specified user (and primary group)
 *
 * @param user user to drop privs to
 * @returns TRUE on success, FALSE otherwise
 */
static int drop_privs(char *user) {
    struct passwd *pw = NULL;

    if(getuid() == (uid_t)0) {
        if(atoi(user)) {
            pw = getpwuid((uid_t)atoi(user));
        } else {
            pw = getpwnam(user);
        }

        if(pw) {
            if(initgroups(user, pw->pw_gid) != 0 ||
               setgid(pw->pw_gid) != 0 ||
               setuid(pw->pw_uid) != 0) {
                ERROR("Could not drop privs to %s, gid=%d, uid=%d",
                      user, pw->pw_gid, pw->pw_uid);
                return FALSE;
            }
        } else {
            ERROR("Could not lookup user %s", user);
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * return the object to client
 *
 * @param client the response we want to send to this fd
 */
/* void handle_response(client_t *client) { */
/*     struct evbuffer *evb; */

/*     assert(client); */
/*     assert(client->buf_ev); */
/*     assert(client->fd); */

/*     if((!client) || (!client->buf_ev) || (!client->fd)) { */
/*         ERROR("Bad client address in handle_reponse"); */
/*         close_client(client); */
/*         return; */
/*     } */

/*     /\* we'll turn on the write events again and throw out the response *\/ */
/*     evb = evbuffer_new(); */

/*     evbuffer_add(evb, client->response, ntohs(client->response->response_len)); */
/*     DEBUG("Queueing %d bytes for write on fd %d", ntohs(client->response->response_len), */
/*           client->fd); */

/*     /\* write low-water should already be zero *\/ */
/*     bufferevent_enable(client->buf_ev, EV_WRITE); */
/*     bufferevent_write_buffer(client->buf_ev, evb); */
/*     evbuffer_free(evb); */
/* } */

/**
 * Spin out an error item to the client
 *
 * @param client placeholder with client request
 * @param text error message text
 */
static void handle_error(client_t *client, char *text) {
    struct evbuffer *evb;
    char *buffer;

    assert(client);
    assert(client->request);

    if((!client) || (!client->request)) {
        /* can't happen -- dispatcher catches this */
        close_client(client);
        return;
    }

    evb = evbuffer_new();

    buffer = (char*)malloc(strlen(text) + 10);  /* "3%s\t\t\t\n\r.\n\r", text */
    if(!buffer) {
        ERROR("malloc error");
        evbuffer_free(evb);
        close_client(client);
        return;
    }

    sprintf(buffer, "3%s\t\t\t\n\r.\n\r", text);
    evbuffer_add(evb, (void*)buffer, strlen(buffer));

    DEBUG("Queueing %d bytes for write on fd %d", strlen(buffer),
          client->fd);

    /* write low-water should already be zero */
    bufferevent_enable(client->buf_ev, EV_WRITE);
    bufferevent_write_buffer(client->buf_ev, evb);
    evbuffer_free(evb);

    return;
}

/**
 * We have a brand new request from a new client, so we'll
 * do the needful.
 *
 * @param client placeholder with client request.
 */
static void handle_request(client_t *client) {
    assert(client);
    assert(client->request);

    if(!client)
        return;

    if(!client->request) {
        ERROR("No client request on fd %d.  Aborting", client->fd);
        close_client(client);
    }

    /* we don't really care about read events any more, so we'll
     * disable those, but we'll keep the bufferevent around because
     * we'll eventually be pushing a write out to this fd. */
    bufferevent_disable(client->buf_ev, EV_READ);

    /* figure out what handler type the request is for
       and pass it through */
    handle_error(client, "Oh noes!");
}


/**
 * set a fd to nonblocking mode... the libevent stuff
 * wants non-blocking sockets
 *
 * @param fd fd to set nonblocking
 */
static int setnonblock(int fd) {
    int flags;

    flags = fcntl(fd, F_GETFL);
    if(flags < 0)
        return flags;

    flags |= O_NONBLOCK;
    if(fcntl(fd,F_SETFL, flags) < 0)
        return -1; /* errno is set */

    return 0;
}


/**
 * handle terminating a client connection
 *
 * @param client client connection to terminate
 */
static void close_client(client_t *client) {
    int fd;

    assert(client);

    if(!client)
        return;

    fd = client->fd;

    if(fd) {
        DEBUG("Closing fd %d", fd);

        shutdown(client->fd, SHUT_RDWR);
        close(client->fd);
    } else {
        ERROR("Probably bad client in close_client");
    }

    if(client->buf_ev) {
        bufferevent_disable(client->buf_ev, EV_READ);
        bufferevent_disable(client->buf_ev, EV_WRITE);
        bufferevent_free(client->buf_ev);
        client->buf_ev = NULL;
    }

    if(client->request) {
        free(client->request);
        client->request = NULL;
    }

    /* if(client->response) { */
    /*     free(client->response); */
    /*     client->response = NULL; */
    /* } */

    free(client);

    DEBUG("Closed fd %d", fd);
}


/**
 * Something strange happened.  Either the client disconnected when we weren't
 * expecting it, or we had some kind of polling error on the fd.  Either way,
 * the thing to do is probably punt the connection.
 */
static void on_buf_error(struct bufferevent *bev, short what, void *arg) {
    client_t *client = (client_t *)arg;

    DEBUG("Caught an error on event buffer");

    if(what & EVBUFFER_EOF) {
        DEBUG("Client closed connection on fd %d", client->fd);
    } else {
        ERROR("Socket error on fd %d", client->fd);
    }

    close_client(client);
}


/**
 * Set to a zero watermark, so gets called when all data for this
 * socket is written out.  In this case, we're done.
 */
static void on_buf_write(struct bufferevent *bev, void *arg) {
    /* we finished our write.  We done. */
    client_t *client = (client_t *)arg;

    DEBUG("Finished write on fd %d", client->fd);
    close_client(client);
}

/**
 * handle outstanding reads on the client socket
 */
static void on_buf_read(struct bufferevent *bev, void *arg) {
    client_t *client = (client_t *)arg;
    size_t buffer_left, bytes_read;
    char *end;

    assert(client);
    assert(client->fd > 0);
    assert(client->request);

    if((!client) || (!client->request) || (client->fd < 1)) {
        ERROR("Bad client struct in on_buf_read");
        close_client(client);
        return;
    }

    DEBUG("Read event (state %d) on fd %d", client->state, client->fd);

    buffer_left = MAX_REQUEST_SIZE - strlen(client->request);

    if(buffer_left <= 0) {
        ERROR("Out of request space on fd %d.  Aborting.", client->fd);
        close_client(client);
    }

    bytes_read = bufferevent_read(client->buf_ev,
                                  (void*)&client->request[strlen(client->request)],
                                  buffer_left - 1);

    if(bytes_read == -1) {
        if (errno != EINTR) {
            /* if EINTR, we'll just pick it back up on the next epoll */
            ERROR("Read error on fd %d", client->fd);
            close_client(client);
        }

        return;
    }


    DEBUG("Read %d bytes on fd %d", bytes_read, client->fd);

    end = client->request;
    while(*end && (*end != '\n') && (*end != '\r')) {
        end++;
    }

    if(*end) {
        *end = '\0';
        client->state = CLIENT_STATE_WAITING_REPLY;
        DEBUG("Got client request on fd %d: %s", client->fd, client->request);

        /* hand this off to set up a response object */
        handle_request(client);
    } else {
        DEBUG("Partial request read on fd %d", client->fd);
    }
}

/**
 * read a signal from the libdaemon signal queue (event dispatch routine).
 *
 * @param fd fd signalled (via libevent)
 * @param event event (read/write, etc... from libevent)
 * @param arg opaqe pointer (from libevent), underlying event struct
 */
static void on_signal(int fd, short event, void *arg) {
    //    struct event *ev = arg;
    int sig;

    while((sig = daemon_signal_next()) > 0) {
        switch(sig) {
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
            INFO("Got signal -- terminating");
            g_quitflag = 1;
            break;
        case SIGHUP:
            INFO("Got HUP");
            break;
        case SIGPIPE:
            INFO("Got SIGPIPE");
            break;
        }
    }

    if(sig < 0) {
        ERROR("daemon_signal_next() failed: %s.  Aborting", strerror(errno));
        g_quitflag = 1;
    }
}

/**
 * handle the case of an accept on the gopher server socket
 */
static void on_accept(int fd, short event, void *arg) {
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(struct sockaddr_in);
    client_t *client = NULL;

    DEBUG("Incoming connection...");

    client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
    if(client_fd == -1) {
        ERROR("Accept failed: %s", strerror(errno));
        return;
    }

    DEBUG("Accepted connection on fd %d", client_fd);

    if(setnonblock(client_fd) < 0) {
        ERROR("Can't set client socket nonblocking.  Terminating");
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        return;
    }

    client = (client_t *)calloc(1, sizeof(client_t));
    if(!client) {
        ERROR("Malloc error in on_accept");
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        return;
    }

    /* set up read/write events */
    client->fd = client_fd;
    client->buf_ev = bufferevent_new(client_fd, on_buf_read,
                                     on_buf_write, on_buf_error, (void*)client);
    client->state = CLIENT_STATE_WAITING_REQUEST;

    client->request = (char*)calloc(1, MAX_REQUEST_SIZE);
    if(!client->request) {
        ERROR("Malloc error in on_accept");
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        free(client);
        return;
    }

    bufferevent_enable(client->buf_ev, EV_READ);
}


/**
 * this is what the child process does continuously.  If
 * the child process dies, then it gets respawned by the
 * watchdog to maintain continuity.
 */
static int do_child_process(void) {
    int server_sockfd;
    struct sockaddr_in server_address;
    struct event_base *pbase = NULL;
    struct event evsignal;   /* libdaemon's signal fd */
    struct event evaccept;   /* server socket */
    int retval = 1;

    /* if(lookup_config.drop_core) { */
    /*     const struct rlimit rlim = { */
    /*         RLIM_INFINITY, */
    /*         RLIM_INFINITY */
    /*     }; */

    /*     setrlimit(RLIMIT_CORE, &rlim); */
    /*     prctl(PR_SET_DUMPABLE, 1); */
    /* } */

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sockfd == -1) {
        ERROR("Cannot create server socket: %s", strerror(errno));
        goto finish;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(config.port);

    if(bind(server_sockfd, (struct sockaddr*)&server_address, (socklen_t)sizeof(server_address)) < 0) {
        ERROR("Bind error: %s", strerror(errno));
        goto finish;
    }

    if(listen(server_sockfd, config.socket_backlog) < 0) {
        ERROR("Listen error: %s", strerror(errno));
        goto finish;
    }

    if(setnonblock(server_sockfd) < 0) {
        ERROR("Could not set server socket to non-blocking: %s", strerror(errno));
        goto finish;
    }

    /* FIXME: drop privs */

    /* set up events */
    pbase = event_init();
    if(!pbase) {
        ERROR("Could not get event_base.  Failing");
        goto finish;
    }

    /* set up event for libdaemon's signal fd */
    event_set(&evsignal, daemon_signal_fd(), EV_READ | EV_PERSIST, on_signal, &evsignal);
    event_add(&evsignal, NULL);

    /* set up events for listening AF_INET socket */
    event_set(&evaccept, server_sockfd, EV_READ | EV_PERSIST, on_accept, &evaccept);
    event_add(&evaccept, NULL);


    while(!g_quitflag) {
        event_base_loop(pbase, EVLOOP_ONCE);
    }

    retval = 0;

 finish:
    if(pbase) {
        event_del(&evaccept);
        event_del(&evsignal);
    }

    if(server_sockfd != -1) {
        shutdown(server_sockfd, SHUT_RDWR);
        close(server_sockfd);
    }

    exit(retval);
}

/**
 * Read the config file, determine if we should daemonize or not,
 * and set up libevent dispatches.
 */
int main(int argc, char *argv[]) {
    int cmdline_debug_level = 0;
    int foreground = 0;
    int option;
    pid_t pid;
    int kill=0;
    int ret;

    /* set some sane config defaults */
    memset((void*)&config, 0, sizeof(gopher_conf_t));

    config.port = 70;
    config.base_dir = ".";
    config.socket_backlog = 5;
    config.config_file = DEFAULT_CONFIGFILE;

    while((option = getopt(argc, argv, "d:c:fp:s:k")) != -1) {
        switch(option) {
        case 'd':
            cmdline_debug_level = atoi(optarg);
            if(cmdline_debug_level < 1 || cmdline_debug_level > 5) {
                ERROR("Debug level must be 1-5 (default %d)", DEFAULT_DEBUGLEVEL);
                usage_quit(argv[0]);
            }
            break;
        case 'c':
            config.config_file = optarg;
            break;
        case 'f':
            foreground = 1;
            break;
        case 'p':
            config.port = atoi(optarg);
            break;
        case 's':
            config.base_dir = optarg;
        case 'k':
            kill = 1;
            break;
        default:
            usage_quit(argv[0]);
        }
    }

    debug_level(cmdline_debug_level ? cmdline_debug_level : DEFAULT_DEBUGLEVEL);

    /* kill before config, so we don't have to specify a config file */
    daemon_pid_file_ident = daemon_ident_from_argv0(argv[0]);

    /* kill? */
    if(kill) {
        if((ret = daemon_pid_file_kill_wait(SIGTERM, 5)) < 0) {
           ERROR("Failed to kill daemon: %s", strerror(errno));
        }
        exit(ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
    }

    if((pid = daemon_pid_file_is_running()) >= 0) {
        ERROR("Daemon already running as pid %u", pid);
        exit(EXIT_FAILURE);
    }

    /* TODO: read the config file and verify sufficient config */

    debug_level(cmdline_debug_level);

    /* daemonize, or check for background daemon */
    if(!foreground) {
        debug_output(DBG_OUTPUT_SYSLOG, "evgopherd");

        if(daemon_retval_init() < 0) {
            ERROR("Could not set up daemon pipe");
            exit(EXIT_FAILURE);
        }

        if((pid = daemon_fork()) < 0) {
            daemon_retval_done();
            ERROR("Forking error: %s", strerror(errno));
            exit(EXIT_FAILURE);
        } else if (pid) { /* parent */
            if((ret = daemon_retval_wait(5)) < 0) {
                ERROR("Could not receive startup retval from daemon process: %s", strerror(errno));
                exit(EXIT_FAILURE);
            }

            if(ret > 0)
                ERROR("Daemon declined to start.  Error: %d\n", ret);

            exit(ret == 0 ? EXIT_SUCCESS :EXIT_FAILURE);
        }
    }

    if(daemon_signal_init(SIGINT, SIGTERM, SIGQUIT, SIGHUP, SIGPIPE, 0) < 0) {
        ERROR("Could not set up signal handlers: %s", strerror(errno));
        goto finish;
    }

    if(!foreground) {
        if(daemon_pid_file_create() < 0) {
            ERROR("Could not create pidfile: %s", strerror(errno));
            if(!foreground)
                daemon_retval_send(2);
            goto finish;
        }

        /* should close handles here */
        daemon_retval_send(0); /* started up to the point that we can rely on syslog */
    }

    WARN("Daemon started");

    /* need to fork off a watchdog process */
    while(!g_quitflag) {
        /* fork and wait */
        pid_t pid = fork();

        if(pid == -1) {
            ERROR("Error forking child process.  Aborting");
            g_quitflag = 1;
        } else if(pid == 0) { /* child */
            do_child_process();
        } else { /* parent */
            int status;
            waitpid(pid, &status, 0);
            if(WIFEXITED(status) && WEXITSTATUS(status)) {
                /* exited with error */
                ERROR("Error initializing child process.  Aborting");
                g_quitflag=1;
            } else if(!WIFEXITED(status)) {
                ERROR("Child process (%d) crashed.  Restarting.", pid);
            } else {
                /* graceful exit... we've obviously terminated */
                g_quitflag = 1;
            }
        }
    }

    WARN("Daemon exiting gracefully");

 finish:
    daemon_signal_done();
    daemon_pid_file_remove();

    return EXIT_SUCCESS;
}
