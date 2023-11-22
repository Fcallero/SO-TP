
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
#include <pthread.h>

#include <global/global.h>
#include <global/utils_cliente.h>
#include <global/utils_server.h>

extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;
extern int socket_kernel;
extern int socket_memoria;
extern int socket_fs;
extern int grado_max_multiprogramacion;


extern t_dictionary* recurso_bloqueado;
extern t_dictionary* colas_de_procesos_bloqueados_para_cada_archivo;
extern t_dictionary* colas_de_procesos_bloqueados_por_pf;
extern t_dictionary *tabla_global_de_archivos_abiertos;



extern t_queue *cola_new;
extern t_queue *cola_ready;
extern t_queue *cola_exit;
extern t_pcb *proceso_ejecutando;
extern char *algoritmo_planificacion;
extern int quantum;

extern sem_t m_cola_ready;
extern sem_t m_cola_new;
extern sem_t m_cola_exit;
extern sem_t despertar_corto_plazo;
extern sem_t despertar_planificacion_largo_plazo;
extern sem_t m_proceso_ejecutando;
extern sem_t m_recurso_bloqueado;
extern sem_t m_cola_de_procesos_bloqueados_para_cada_archivo;
extern sem_t espero_desalojo_CPU;
extern sem_t m_tabla_global_de_archivos_abiertos;

void inicializar_colas_y_semaforos();
void *planificar_nuevos_procesos_largo_plazo(void *arg);
void agregar_cola_new(t_pcb** pcb_proceso);

void pasar_a_ready(t_pcb* proceso_bloqueado);

void pcb_args_destroy(t_pcb* pcb_a_destruir);
void actualizar_estado_a_pcb(t_pcb* a_actualizar_estado, char* estado);

#endif /* SRC_PLANIFICADOR_LARGO_PLAZO_H_ */
