#ifndef SRC_PETICIONES_KERNEL_H_
#define SRC_PETICIONES_KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "filesystem.h"

extern int tam_bloque;
extern uint32_t *bits_fat; //Array con tabla FAT
extern char* path_fcb;
extern int socket_fs;
extern FILE* bloques;
extern char** array_bloques;

void abrir_archivo(uint64_t cliente_fd);
void crear_archivo(uint64_t cliente_fd);
void cerrar_archivo();
void truncar_archivo(uint64_t cliente_fd);
void leer_archivo();
void escribir_archivo();

#endif /* SRC_PETICIONES_KERNEL_H_ */
