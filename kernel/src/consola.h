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
#include<semaphore.h>


#include <global/global.h>
#include <global/utils_cliente.h>
#include <global/utils_server.h>
#include "planificador_largo_plazo.h"

extern sem_t despertar_planificacion_largo_plazo;

typedef struct {
	int opcode_lenght;
	char* opcode;
	int parametro1_lenght;
	int parametro2_lenght;
	int parametro3_lenght;
	char* parametros[3];
}t_comando;

void* levantar_consola();
t_instruccion* armar_comando(char* cadena);
t_instruccion* cargar_opcode(char* cadena, t_comando* comando);
void iniciar_proceso(t_instruccion* comando);
void finalizar_proceso();
void detener_planificacion();
void iniciar_planifiacion();
void multiprogramacion();
void proceso_estado();
void verificar_entrada_comando();
void enviar_comando_memoria(t_instruccion* comando, op_code code);
void parametros_lenght(t_instruccion* ptr_inst);

#endif /* SRC_CONSOLA_H_ */
