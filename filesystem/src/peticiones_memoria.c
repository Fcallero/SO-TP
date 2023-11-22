#include "peticiones_memoria.h"

//void agregar_bloques(t_fcb* fcb_a_actualizar, int bloques_a_agregar)

void reservar_bloques(int tam_bloque,int cliente_fd){

	int size;
	void* buffer = recibir_buffer(&size, cliente_fd);
	int tamanio_proceso;

	memcpy(&tamanio_proceso,buffer,sizeof(int));


	//crear bloques


//buffer recibe tamanio del proceso
	//



}


void marcar_bloques_libres(){



}
