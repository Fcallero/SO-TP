#include "filesystem.h"

FILE* fat;
char* path_fcb;
int tam_bloque;
uint32_t *bits_fat; //Array con tabla FAT
//char **array_bloques; //Array bloques
t_bitarray *bitarray_bloques;
int primer_bloque_fat;

int main(int argc, char* argv[]) {
	//Declaraciones de variables para config:

		char* ip_memoria;
		char* puerto_memoria;
		char* puerto_escucha;
		char* path_fat;
		char* path_bloques;
		int cant_bloques_total;
		int cant_bloques_swap;
		int retardo_acceso_bloque;
		int retardo_acceso_fat;

	/*------------------------------LOGGER Y CONFIG--------------------------------------------------*/

		// Iniciar archivos de log y configuracion:
		t_config* config = iniciar_config();
		logger = iniciar_logger();

		// Verificacion de creacion archivo config
		if(config == NULL){
			log_error(logger, "No fue posible iniciar el archivo de configuracion !!");
			terminar_programa(logger, config);
		}

		// Carga de datos de config en variable y archivo
		ip_memoria = config_get_string_value(config, "IP_MEMORIA");
		puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");

		puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

		path_fat = config_get_string_value(config, "PATH_FAT");
		path_bloques = config_get_string_value(config, "PATH_BLOQUES");
		path_fcb = config_get_string_value(config, "PATH_FCB");

		cant_bloques_total = config_get_int_value(config, "CANT_BLOQUES_TOTAL");
		cant_bloques_swap = config_get_int_value(config, "CANT_BLOQUES_SWAP");
		tam_bloque = config_get_int_value(config, "TAM_BLOQUE");

		retardo_acceso_bloque = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");
		retardo_acceso_fat = config_get_int_value(config, "RETARDO_ACCESO_FAT");

		// Control archivo configuracion
		if(!ip_memoria || !puerto_memoria || !puerto_escucha || !path_fat || !path_bloques || !path_fcb || !cant_bloques_total || !cant_bloques_swap || !tam_bloque || !retardo_acceso_bloque || !retardo_acceso_fat){
			log_error(logger, "Error al recibir los datos del archivo de configuracion del Filesystem");
			terminar_programa(logger, config);
		}

		//Remplazo path con ruta base
/*
		path_fat = string_replace(path_bitmap, "~", "/home/utnso");
		path_bloques = string_replace(path_bloques, "~", "/home/utnso");
		path_fcb = string_replace(path_fcb, "~", "/home/utnso");
		*/

	/*-------------------------------CONEXIONES KERNEL---------------------------------------------------------------*/

		//Esperar conexion de Kernel
		socket_fs = iniciar_servidor(puerto_escucha);

		log_info(logger, "Filesystem esta listo para recibir peticiones");

		//Levantar diccionarios
		fcb_por_archivo = dictionary_create();


		//Levantar FAT y calcular el tamaño
		int tamanio_fat = (cant_bloques_total - cant_bloques_swap) * sizeof(uint32_t); //Expresado en bytes
		fat = levantar_archivo_binario(path_fat);


		//Obtengo el file descriptor de la FAT y trunco el archivo para que tenga el tamaño correcto
		int fat_fd = fileno(fat);
		ftruncate(fat_fd, tamanio_fat);

		// Mapeo la FAT a memoria y chequeo errores
		bits_fat = mmap(NULL, tamanio_fat, PROT_READ|PROT_WRITE, MAP_SHARED, fat_fd, 0);

		// Verificar si el mapeo fue exitoso
		if (bits_fat == MAP_FAILED) {
		    perror("Error al mapear la FAT a memoria");
		    // Manejar el error apropiadamente
		    exit(EXIT_FAILURE);
		}

		//Levanto archivo de bloques, un archivo que contiene todos los bloques de nuestro FS. Obtengo el FD y lo trunco al tamaño necesario
		bloques = levantar_archivo_binario(path_bloques);

		int bloques_fd = fileno(bloques);

		ftruncate(bloques_fd, cant_bloques_total * tam_bloque);

		//mapeo el archivo de bloques

		void *bit_bloques_swap = malloc(cant_bloques_swap);

		bitarray_bloques = bitarray_create_with_mode(bit_bloques_swap, cant_bloques_swap * tam_bloque, MSB_FIRST);

		/*
		char* bit_bloques = mmap(NULL, cant_bloques_total * tam_bloque, PROT_WRITE, MAP_SHARED, bloques_fd, 0);
		//Creo un array de bloques para manejar el archivo binario con la estructura correcta y les asigno por referencia los valores del mapeo de bloques
		char **array_bloques = malloc(cant_bloques_total * tam_bloque);
		for (int i = 0; i < cant_bloques_total; i++) {
	        array_bloques[i] = &bit_bloques[i * tam_bloque];
		}
*/
		primer_bloque_fat=cant_bloques_swap+1;//bloque 0 de la fat

		manejar_peticiones();


	munmap(bits_fat,  tamanio_fat);
	fclose(bloques);
	fclose(fat);
	terminar_programa(logger, config);
    return 0;
}

//Iniciar archivo de log y de config

t_log* iniciar_logger(void){
	t_log* nuevo_logger = log_create("filesystem.log", "Filesystem", true, LOG_LEVEL_INFO);
	return nuevo_logger;
}

t_config* iniciar_config(void){
	t_config* nueva_config = config_create("filesystem.config");
	return nueva_config;
}


//Finalizar el programa

 void terminar_programa(t_log* logger, t_config* config){
	log_destroy(logger);
	config_destroy(config);
	// bitarray_destroy(bitarray_bloques_libres);
	fclose(fat);
	fclose(bloques);
	close(socket_fs);
 }

 // levanta un archivo binario en base al path que recibe y lo devuelve
 // si no existe lo crea
 FILE* levantar_archivo_binario(char* path_archivo){

 	FILE* archivo = fopen(path_archivo, "ab+");

 	archivo = freopen(path_archivo, "rb+", archivo);


 	if(archivo == NULL){
 		log_error(logger, "No existe el archivo con el path: %s",path_archivo);
 		return NULL;
 	}

 	return archivo;
 }

// atiende las peticiones de kernel y memoria de forma concurrente
void manejar_peticiones(){
	while(1){
			pthread_t thread;
			uint64_t cliente_fd = (uint64_t) esperar_cliente(socket_fs);

			t_arg_atender_cliente* argumentos_atender_cliente = malloc(sizeof(t_arg_atender_cliente));
			argumentos_atender_cliente->cliente_fd = cliente_fd;


			pthread_create(&thread, NULL, atender_cliente, (void*) argumentos_atender_cliente);

			pthread_detach(thread);
		}
}


void *atender_cliente(void* args){
	t_arg_atender_cliente* argumentos = (t_arg_atender_cliente*) args;

	uint64_t cliente_fd = argumentos->cliente_fd;

	while(1){
		int cod_op = recibir_operacion(cliente_fd);

		switch(cod_op){
			case MENSAJE:
				recibir_mensaje(cliente_fd);
				break;
			case HANDSHAKE:
				recibir_handshake(cliente_fd);
				break;
			case ABRIR_ARCHIVO:
				abrir_archivo(cliente_fd);
				break;
			case CREAR_ARCHIVO:
				crear_archivo(cliente_fd);
				break;
			case CERRAR_ARCHIVO:
				// cerrar_archivo();
				break;
			case TRUNCAR_ARCHIVO:
				truncar_archivo(cliente_fd);
					break;
			case LEER_ARCHIVO:
				// leer_archivo();
				break;
			case ESCRIBIR_ARCHIVO:
				// escribir_archivo();
				break;
			case INICIAR_PROCESO:
				reservar_bloques(cliente_fd);
				break;
			case FINALIZAR_PROCESO_FS:
				marcar_bloques_libres(cliente_fd);
				break;
			case LEER_CONTENIDO_PAGINA:
				devolver_contenido_pagina(cliente_fd);
				break;
			case -1:
				log_error(logger, "El cliente se desconecto. Terminando servidor");
				return NULL;
			default:
				log_warning(logger, "Operacion desconocida. No quieras meter la pata");
				break;
		}
	}

	return NULL;
}

