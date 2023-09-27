#ifndef CPU_H_
#define CPU_H_

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


extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;
extern int socket_kernel;
extern int socket_memoria;

t_config* iniciar_config(void);
t_log* iniciar_logger(void);
void terminar_programa(t_log* logger, t_config* config);
int conectar_memoria(char* ip, char* puerto);
void* manejar_interrupciones(void* args);
void manejar_peticiones_instruccion();
void manejar_peticion_al_cpu();

t_instruccion *recibir_instruccion_memoria(int);
void recibir_interrupcion(int);
void devolver_a_kernel(t_contexto_ejec* contexto, op_code code, int socket_cliente);

void manejar_instruccion_set(t_contexto_ejec** contexto,t_instruccion* instruccion);
void setear_registro(t_contexto_ejec** contexto,char* registro, int valor);

int obtener_valor_del_registro(char* registro_a_leer, t_contexto_ejec** contexto_actual);
void manejar_instruccion_sub(t_contexto_ejec** contexto_actual,t_instruccion* instruccion);
void manejar_instruccion_sum(t_contexto_ejec** contexto_actual,t_instruccion* instruccion);

#endif /* CPU_H_ */
