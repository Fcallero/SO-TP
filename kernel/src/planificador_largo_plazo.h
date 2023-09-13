

#ifndef SRC_PLANIFICADOR_LARGO_PLAZO_H_
#define SRC_PLANIFICADOR_LARGO_PLAZO_H_

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
#include <commons/temporal.h>
#include<semaphore.h>
#include<commons/collections/dictionary.h>

#include <global/global.h>
#include <global/utils_cliente.h>
#include <global/utils_server.h>
#include "kernel.h"


extern t_dictionary* recurso_bloqueado;

extern t_dictionary* colas_de_procesos_bloqueados_para_cada_archivo;

extern t_queue* cola_new;
extern t_queue* cola_ready;
extern t_pcb* proceso_ejecutando;
extern t_temporal* rafaga_proceso_ejecutando;
extern char* algoritmo_planificacion;

extern sem_t m_cola_ready;
extern sem_t m_cola_new;
extern sem_t despertar_corto_plazo;
extern sem_t m_proceso_ejecutando;
extern sem_t m_recurso_bloqueado;
extern sem_t m_cola_de_procesos_bloqueados_para_cada_archivo;

void inicializar_colas_y_semaforos();
void *planificar_nuevos_procesos_largo_plazo(void *arg);
void agregar_proceso_a_ready(int conexion_memoria,  char* algoritmo_planificacion);
void agregar_cola_new(t_pcb* pcb_proceso);

char* listar_pids_cola_ready(void);
void pasar_a_ready(t_pcb* proceso_bloqueado);
int calcular_procesos_en_memoria(int procesos_en_ready);

#endif /* SRC_PLANIFICADOR_LARGO_PLAZO_H_ */
