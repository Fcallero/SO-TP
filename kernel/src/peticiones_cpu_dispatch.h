#ifndef SRC_PETICIONES_CPU_DISPATCH_H_
#define SRC_PETICIONES_CPU_DISPATCH_H_

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
#include <pthread.h>


#include <global/global.h>
#include <global/utils_cliente.h>
#include <global/utils_server.h>

#include "planificador_largo_plazo.h"
#include "planificador_corto_plazo.h"

extern t_dictionary* recurso_bloqueado;
extern t_dictionary *matriz_recursos_asignados;
extern t_dictionary *matriz_recursos_pendientes;
extern int *recursos_totales;
extern char	**recursos;
extern int cant_recursos;

void poner_a_ejecutar_otro_proceso();

void manejar_sleep(int socket_cliente);
void apropiar_recursos(int socket_cliente, char** recursos, int* recurso_disponible, int cantidad_de_recursos);
void desalojar_recursos(int socket_cliente, char** recursos, int* recurso_disponible, int cantidad_de_recursos);
void finalinzar_proceso(int socket_cliente);
void deteccion_de_deadlock();

t_list *duplicar_lista_recursos(t_list *a_duplicar);
int obtener_indice_recurso(char** recursos, char* recurso_a_buscar);
void incrementar_recurso_en_matriz(t_dictionary **matriz, char *nombre_recurso, char *pid, int cantidad_de_recursos);
void decrementar_recurso_en_matriz(t_dictionary **matriz, char *nombre_recurso, char *pid, int cantidad_de_recursos);

void destroy_lista_de_recursos(t_list* lista_recursos);
void destroy_proceso_en_matriz(t_dictionary *matriz, char *pid);
void destroy_matriz(t_dictionary *matriz);
void enviar_a_fs_truncar_archivo(int socket_cpu, int socket_filesystem);
void enviar_a_fs_crear_o_abrir_archivo (int socket_cpu, int socket_filesystem);
void manejar_page_fault(int socket_cliente);

void enviar_instruccion(t_instruccion* instruccion, int socket_a_enviar, int opcode);

void desbloquear_por_espera_a_fs(int pid, char* nombre_archivo);

#endif /* SRC_PETICIONES_CPU_DISPATCH_H_ */
