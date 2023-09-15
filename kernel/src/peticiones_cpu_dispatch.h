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


#include <global/global.h>
#include <global/utils_cliente.h>
#include <global/utils_server.h>

#include "planificador_largo_plazo.h"

typedef struct
{
	int tiempo_io;
	int grado_max_multiprogramacion;

} t_argumentos_simular_sleep;

void* simular_sleep(void* arg);


#endif /* SRC_PETICIONES_CPU_DISPATCH_H_ */
