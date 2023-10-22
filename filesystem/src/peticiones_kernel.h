#ifndef SRC_PETICIONES_KERNEL_H_
#define SRC_PETICIONES_KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include "filesystem.h"


extern int socket_fs;
extern FILE* bloques;
extern t_bitarray* bitarray_bloques_libres;//para modificar la fat

void abrir_archivo();
void crear_archivo();
void cerrar_archivo();
void truncar_archivo();
void leer_archivo();
void escribir_archivo();


#endif /* SRC_PETICIONES_KERNEL_H_ */
