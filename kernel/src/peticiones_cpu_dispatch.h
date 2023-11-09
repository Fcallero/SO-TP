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

void manejar_sleep(int socket_cliente);
void apropiar_recursos(int socket_cliente, char** recursos, int* recurso_disponible, int cantidad_de_recursos);
void desalojar_recursos(int socket_cliente, char** recursos, int* recurso_disponible, int cantidad_de_recursos);
void finalinzar_proceso(int socket_cliente);
void deteccion_de_deadlock();

#endif /* SRC_PETICIONES_CPU_DISPATCH_H_ */
