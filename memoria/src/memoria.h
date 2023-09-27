#ifndef MEMORIA_H_
#define MEMORIA_H_

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

#include <global/global.h>
#include <global/utils_cliente.h>
#include <global/utils_server.h>



extern int socket_memoria;
extern int socket_fs;

typedef struct{
	uint64_t cliente_fd;
}t_arg_atender_cliente;

t_config* iniciar_config(void);
t_log* iniciar_logger(void);
void terminar_programa(t_log* logger, t_config* config);
void crear_proceso();

int conectar_fs(char* ip, char* puerto);
void manejar_pedidos_memoria();
void *atender_cliente(void* args);
#endif
