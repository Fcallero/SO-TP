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
#include <commons/bitarray.h>
#include <commons/txt.h>
#include <sys/mman.h>

#include <global/utils_cliente.h>
#include <global/utils_server.h>
#include <global/global.h>
#include "peticiones_memoria.h"
#include "peticiones_kernel.h"

int socket_fs;
FILE* bloques;
t_bitarray* bitarray_bloques_libres;//para modificar la fat

typedef struct{
	uint64_t cliente_fd;
}t_arg_atender_cliente;


t_config* iniciar_config(void);
t_log* iniciar_logger(void);
void terminar_programa(t_log* logger, t_config* config);
void manejar_peticiones();
FILE* levantar_archivo_binario(char* path_archivo);
void *atender_cliente(void* args);
#endif
