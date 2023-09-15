
#ifndef KERNEL_H_
#define KERNEL_H_

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
#include <readline/readline.h>

#include <global/global.h>
#include <global/utils_cliente.h>
#include <global/utils_server.h>

#include "peticiones_cpu_dispatch.h"
#include "peticiones_cpu_interrupt.h"
#include "planificador_largo_plazo.h"
#include "planificador_corto_plazo.h"


typedef struct{
	uint64_t cliente_fd;
}t_args_manejar_peticiones_modulos;

void* manejar_peticiones_modulos(void* args);

t_config* iniciar_config(void);
t_log* iniciar_logger(void);
void terminar_programa(t_log* logger, t_config* config);
int conectar_memoria(char* ip, char* puerto);
int conectar_fs(char* ip, char* puerto);
int conectar_cpu_dispatch(char* ip, char* puerto);
int conectar_cpu_interrupt(char* ip, char* puerto);
void levantar_consola();
void *escuchar_peticiones_cpu_dispatch(void* args);

#endif /* KERNEL_H_ */
