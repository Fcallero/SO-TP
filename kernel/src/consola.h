#ifndef SRC_CONSOLA_H_
#define SRC_CONSOLA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <readline/readline.h>


#include <global/global.h>
#include <global/utils_cliente.h>
#include <global/utils_server.h>

typedef struct {
	op_code opcode;
	char* parametros[3];
}t_comando;

void* levantar_consola();
t_comando* armar_comando(char* cadena);
t_comando* cargar_opcode(char* cadena, t_comando* comando);
void iniciar_proceso(t_comando* comando);
void finalizar_proceso();
void detener_planificacion();
void iniciar_planifiacion();
void multiprogramacion();
void proceso_estado();

#endif /* SRC_CONSOLA_H_ */
