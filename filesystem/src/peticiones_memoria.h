
#ifndef SRC_PETICIONES_MEMORIA_H_
#define SRC_PETICIONES_MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"

extern int socket_fs;
extern FILE* bloques;
extern t_bitarray* bitarray_bloques_libres;//para modificar la fat

void reservar_bloques();
void marcar_bloques_libres();


#endif /* SRC_PETICIONES_MEMORIA_H_ */

