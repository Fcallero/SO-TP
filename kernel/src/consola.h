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


void levantar_consola();


#endif /* SRC_CONSOLA_H_ */
