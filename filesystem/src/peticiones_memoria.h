
#ifndef SRC_PETICIONES_MEMORIA_H_
#define SRC_PETICIONES_MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"
#include "peticiones_kernel.h"

extern int socket_fs;
extern FILE* bloques;
extern t_bitarray* bitarray_bloques;
extern int tam_bloques;
extern char**array_bloques;

void reservar_bloques(int cliente_fd);
void marcar_bloques_libres(int cliente_fd);
void devolver_contenido_pagina(int cliente_fd);

void guardar_en_puntero(uint32_t puntero, void *contenido);

#endif /* SRC_PETICIONES_MEMORIA_H_ */

