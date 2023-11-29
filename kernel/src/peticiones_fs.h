#ifndef SRC_PETICIONES_FS_H_
#define SRC_PETICIONES_FS_H_
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
#include "planificador_corto_plazo.h"
#include "peticiones_cpu_dispatch.h"

void desbloquear_por_espera_a_fs(int pid, char* nombre_archivo);

void enviar_a_fs_crear_o_abrir_archivo (int socket_cpu, int socket_filesystem);
void enviar_a_fs_truncar_archivo(int socket_cpu, int socket_filesystem);
void reposicionar_puntero(int cliente_fd);
void leer_archivo(int socket_cpu);
void escribir_archivo(int socket_cpu);
void cerrar_archivo(int cliente_fd);

void enviar_instruccion(t_instruccion* instruccion, int socket_a_enviar, int opcode);

#endif /* SRC_PETICIONES_FS_H_ */
