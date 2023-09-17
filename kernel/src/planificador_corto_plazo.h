#ifndef SRC_PLANIFICADOR_CORTO_PLAZO_H_
#define SRC_PLANIFICADOR_CORTO_PLAZO_H_

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
#include <commons/temporal.h>
#include<semaphore.h>
#include<commons/collections/dictionary.h>

#include <global/global.h>
#include <global/utils_cliente.h>
#include <global/utils_server.h>

#include "planificador_largo_plazo.h"// para traerme variables globales

void *planificar_nuevos_procesos_corto_plazo(void *arg);
void planificar_corto_plazo_fifo();
void planificar_corto_plazo_prioridades();
void planificar_corto_plazo_round_robbin();

void enviar_contexto_de_ejecucion_a(t_contexto_ejec* contexto_a_ejecutar, op_code opcode, int socket_cliente);
void crear_contexto_y_enviar_a_CPU(t_pcb* proceso_a_ejecutar);


#endif /* SRC_PLANIFICADOR_CORTO_PLAZO_H_ */
