/******************************************************************************
 *
 * mod_gearman - distribute checks with gearman
 *
 * Copyright (c) 2010 Sven Nierlein - sven.nierlein@consol.de
 *
 * This file is part of mod_gearman.
 *
 *  mod_gearman is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  mod_gearman is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with mod_gearman.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "common.h"
#include "utils.h"
#include "gearman_utils.h"


/* get worker/jobs data from gearman server */
int get_gearman_server_data(mod_gm_server_status_t *stats, char ** message, char ** version, char * hostnam, int port) {
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char * cmd;
    char buf[GM_BUFFERSIZE];
    char * line;
    char * output;
    char * output_c;
    char * name;
    char * total;
    char * running;
    char * worker;
    mod_gm_status_function_t *func;

    *message = malloc(GM_BUFFERSIZE);
    *version = malloc(GM_BUFFERSIZE);
    snprintf(*message, GM_BUFFERSIZE, "%s", "" );
    snprintf(*version, GM_BUFFERSIZE, "%s", "" );

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( sockfd < 0 ) {
        snprintf(*message, GM_BUFFERSIZE, "failed to open socket: %s\n", strerror(errno));
        return( STATE_CRITICAL );
    }

    server = gethostbyname(hostnam);
    if( server == NULL ) {
        snprintf(*message, GM_BUFFERSIZE, "failed to resolve %s\n", hostnam);
        return( STATE_CRITICAL );
    }
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port);
    if (connect(sockfd,(const struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
        snprintf(*message, GM_BUFFERSIZE, "failed to connect to %s:%i - %s\n", hostnam, (int)port, strerror(errno));
        close(sockfd);
        return( STATE_CRITICAL );
    }

    cmd = "status\nversion\n";
    n = write(sockfd,cmd,strlen(cmd));
    if (n < 0) {
        snprintf(*message, GM_BUFFERSIZE, "failed to send to %s:%i - %s\n", hostnam, (int)port, strerror(errno));
        close(sockfd);
        return( STATE_CRITICAL );
    }

    n = read( sockfd, buf, GM_BUFFERSIZE-1 );
    buf[n] = '\x0';
    if (n < 0) {
        snprintf(*message, GM_BUFFERSIZE, "error reading from %s:%i - %s\n", hostnam, (int)port, strerror(errno));
        close(sockfd);
        return( STATE_CRITICAL );
    }

    output = strdup(buf);
    output_c = output;
    while ( (line = strsep( &output, "\n" )) != NULL ) {
        gm_log( GM_LOG_TRACE, "%s\n", line );
        if(!strcmp( line, ".")) {
            if((line = strsep( &output, "\n" )) != NULL) {
                gm_log( GM_LOG_TRACE, "%s\n", line );
                if(line[0] == 'O') {
                    strncpy(*version, line+3, 10);
                } else {
                    snprintf(*version, GM_BUFFERSIZE, "%s", line);
                }
                gm_log( GM_LOG_TRACE, "extracted version: '%s'\n", *version );
            }

            /* sort our array by queue name */
            qsort(stats->function, stats->function_num, sizeof(mod_gm_status_function_t*), struct_cmp_by_queue);

            close(sockfd);
            free(output_c);
            return( STATE_OK );
        }
        name = strsep(&line, "\t");
        if(name == NULL)
            break;
        total   = strsep(&line, "\t");
        if(total == NULL)
            break;
        running = strsep(&line, "\t");
        if(running == NULL)
            break;
        worker  = strsep(&line, "\x0");
        if(worker == NULL)
            break;
        func = malloc(sizeof(mod_gm_status_function_t));
        func->queue   = strdup(name);
        func->running = atoi(running);
        func->total   = atoi(total);
        func->worker  = atoi(worker);
        func->waiting = func->total - func->running;

        /* skip the dummy queue if its empty */
        if(!strcmp( name, "dummy") && func->total == 0) {
            free(func->queue);
            free(func);
            continue;
        }

        stats->function[stats->function_num++] = func;
        gm_log( GM_LOG_DEBUG, "%i: name:%-20s worker:%-5i waiting:%-5i running:%-5i\n", stats->function_num, func->queue, func->worker, func->waiting, func->running );
    }

    snprintf(*message, GM_BUFFERSIZE, "got no valid data from %s:%i\n", hostnam, (int)port);
    free(output_c);
    close(sockfd);
    return( STATE_UNKNOWN );
}


/* free a status structure */
void free_mod_gm_status_server(mod_gm_server_status_t *stats) {
    int x;

    for(x=0; x<stats->function_num;x++) {
        free(stats->function[x]->queue);
        free(stats->function[x]);
    }

    for(x=0; x<stats->worker_num;x++) {
        free(stats->worker[x]->ip);
        free(stats->worker[x]->id);
        free(stats->worker[x]);
    }

    free(stats);
}


/* qsort struct comparision function for queue name */
int struct_cmp_by_queue(const void *a, const void *b) {
    mod_gm_status_function_t **pa = (mod_gm_status_function_t **)a;
    mod_gm_status_function_t *ia  = (mod_gm_status_function_t *)*pa;

    mod_gm_status_function_t **pb = (mod_gm_status_function_t **)b;
    mod_gm_status_function_t *ib  = (mod_gm_status_function_t *)*pb;

    return strcmp(ia->queue, ib->queue);
}
