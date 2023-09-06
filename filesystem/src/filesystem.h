#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/node.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <pthread.h>

#include <utils/utils_cliente.h>
#include <utils/utils_server.h>
#include <global.h>


extern int socket_cpu;
extern int socket_kernel;
extern int socket_memoria;
extern int socket_fs;
extern int grado_max_multiprogramacion;

t_config* iniciar_config(void);
t_log* iniciar_logger(void);
void terminar_programa(t_log* logger, t_config* config);

#endif
