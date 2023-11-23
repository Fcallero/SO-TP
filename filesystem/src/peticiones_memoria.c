#include "peticiones_memoria.h"

//void agregar_bloques(t_fcb* fcb_a_actualizar, int bloques_a_agregar)

uint32_t asignar_bloque_libre_bitarray_desde(uint32_t puntero_inicio){

	while(bitarray_test_bit(bitarray_bloques,puntero_inicio)){
		puntero_inicio++;
	}

	bitarray_set_bit(bitarray_bloques, puntero_inicio);

	return puntero_inicio;
}

void liberar_bloque_bitarray(uint32_t puntero_bloque){
	bitarray_clean_bit(bitarray_bloques,puntero_bloque);
}

void guardar_en_puntero(uint32_t puntero, void *contenido){
	fseek(bloques,puntero*tam_bloque , SEEK_SET);
	fwrite(contenido, tam_bloque, 1, bloques);
}

void *leer_bloque_swap(uint32_t puntero){
	void *bloque = malloc(tam_bloque);
	fseek(bloques,puntero*tam_bloque , SEEK_SET);
	fread(bloque, tam_bloque, 1, bloques);
	return bloque;
}

void reservar_bloques(int cliente_fd){

	int size;
	void* buffer = recibir_buffer(&size, cliente_fd);
	int tamanio_proceso;

	memcpy(&tamanio_proceso,buffer,sizeof(int));

	int cant_bloques_a_asignar_swap = (tamanio_proceso / tam_bloque) + 1; //el +1 por si es comma algo


	//asigno bloques con el bitarray de bloques libres
	uint32_t punteros_x_paginas[cant_bloques_a_asignar_swap];

	char *basura = string_repeat('\0', tam_bloque);

	for(int i = 0; i<cant_bloques_a_asignar_swap; i++){
		uint32_t puntero_n_bloque =  asignar_bloque_libre_bitarray_desde(0);

		punteros_x_paginas[i]= puntero_n_bloque;

		//marcar \0 al los bloques en el archivo de bloques
		guardar_en_puntero(puntero_n_bloque, basura);
	}


	//aca devuelve un array de los punteros a los bloques de swap asignados

	t_paquete* paquete = crear_paquete(INICIAR_PROCESO);

	agregar_a_paquete_sin_agregar_tamanio(paquete, &cant_bloques_a_asignar_swap, sizeof(int));
	for(int i= 0; i<cant_bloques_a_asignar_swap; i++){
		agregar_a_paquete_sin_agregar_tamanio(paquete, &(punteros_x_paginas[i]), sizeof(uint32_t));
	}

	enviar_paquete(paquete, cliente_fd);

	eliminar_paquete(paquete);
	free(basura);
	free(buffer);
}



//aca recibe lista de los punteros a los bloques de swap que debe liberar (que liberar significa colocar \0 en los bloques, creo)"
void marcar_bloques_libres(int cliente_fd){
	int size;
	void* buffer = recibir_buffer(&size, cliente_fd);
	int desplazamiento = 0;
	int cant_paginas;

	memcpy(&cant_paginas, buffer, sizeof(int));
	desplazamiento+= sizeof(int);

	uint32_t punteros[cant_paginas];

	for(int i = 0; i<cant_paginas; i++){
		memcpy(&(punteros[i]), buffer+desplazamiento, sizeof(uint32_t));
		desplazamiento+=sizeof(uint32_t);

		liberar_bloque_bitarray(punteros[i]);
	}

	//libero bloques
	char *basura = string_repeat('\0', tam_bloque);
	for(int i = 0; i<cant_paginas; i++){
		guardar_en_puntero(punteros[i], basura);
	}

	free(basura);
	free(buffer);
}

void devolver_contenido_pagina(int cliente_fd){
	int size;
	void* buffer = recibir_buffer(&size, cliente_fd);
	uint32_t pos_swap_a_leer;

	memcpy(&pos_swap_a_leer, buffer, sizeof(uint32_t));

	void *contenido_bloque = leer_bloque_swap(pos_swap_a_leer);

	t_paquete *paquete = crear_paquete(LEER_CONTENIDO_PAGINA);
	agregar_a_paquete_sin_agregar_tamanio(paquete, contenido_bloque, tam_bloque);

	enviar_paquete(paquete, cliente_fd);

	eliminar_paquete(paquete);
	free(contenido_bloque);
	free(buffer);
}
