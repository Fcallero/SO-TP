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

#include <global/utils_cliente.h>
#include <global/utils_server.h>
#include <global/global.h>
#include "peticiones_memoria.h"
#include "peticiones_kernel.h"

extern int socket_memoria;
extern int socket_fs;

typedef struct{
	uint64_t cliente_fd;
}t_arg_atender_cliente;

t_config* iniciar_config(void);
t_log* iniciar_logger(void);
void terminar_programa(t_log* logger, t_config* config);
int conectar_memoria(char* ip, char* puerto);
void manejar_peticiones();
void *atender_cliente(void* args);
#endif
