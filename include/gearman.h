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

/** @file
 *  @brief header for the gearman module
 *
 *  @{
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <libgearman/gearman.h>

typedef void*( mod_gm_worker_fn)(gearman_job_st *job, void *context, size_t *result_size, gearman_return_t *ret_ptr);

gearman_client_st *current_client;
gearman_client_st *current_client_dup;
gearman_job_st *current_gearman_job;

int create_client( gm_server_t * server_list[GM_LISTSIZE], gearman_client_st * client);
int create_client_dup( gm_server_t * server_list[GM_LISTSIZE], gearman_client_st * client);
int create_worker( gm_server_t * server_list[GM_LISTSIZE], gearman_worker_st * worker);
int add_job_to_queue( gearman_client_st *client, gm_server_t * server_list[GM_LISTSIZE], char * queue, char * uniq, char * data, int priority, int retries, int transport_mode, int send_now );
int worker_add_function( gearman_worker_st * worker, char * queue, gearman_worker_fn *function);
void *dummy( gearman_job_st *, void *, size_t *, gearman_return_t * );
void free_client(gearman_client_st *client);
void free_worker(gearman_worker_st *worker);

/**
 * @}
 */
