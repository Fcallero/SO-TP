#include "filesystem.h"

int socket_memoria;
int socket_fs;
FILE* fat;
t_bitarray* bitarray_bloques_libres;
FILE* bloques;

int main(int argc, char* argv[]) {
	//Declaraciones de variables para config:

		char* ip_memoria;
		char* puerto_memoria;
		char* puerto_escucha;
		char* path_fat;
		char* path_bloques;
		char* path_fcb;
		int cant_bloques_total;
		int cant_bloques_swap;
		int tam_bloque;
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

	/*-------------------------------CONEXIONES KERNEL---------------------------------------------------------------*/

		// Realizar las conexiones y probarlas
//		int result_conexion_memoria = conectar_memoria(ip_memoria, puerto_memoria);
//
//		if(result_conexion_memoria  == -1){
//			log_error(logger, "No se pudo conectar con el modulo Memoria !!");
//			terminar_programa(logger, config);
//		}
//		log_info(logger, "El Filesystem se conecto con el modulo Memoria correctamente");

		//Esperar conexion de Kernel
		socket_fs = iniciar_servidor(puerto_escucha);

		log_info(logger, "Filesystem esta listo para recibir peticiones");

		//Levantar estructuras de FS
		int tamanio_fat = (cant_bloques_total - cant_bloques_swap) * sizeof(uint32_t);
		fat = levantar_archivo_binario(path_fat);

		//Truncar fat para que tenga el tamaño correcto

		//trunco el archivo para que tenga el tamaño del bitmap
		int fat_fd = fileno(fat);
		ftruncate(fat_fd, tamanio_fat);

		//	osea si modifico algo en el bits_bitmap, tambien se moficica en el archivo
		char* bits_fat = mmap(NULL, tamanio_fat, PROT_WRITE, MAP_SHARED, fat_fd, 0);

		bitarray_bloques_libres = bitarray_create_with_mode(bits_fat, tamanio_fat, MSB_FIRST);

		bloques = levantar_archivo_binario(path_bloques);

		int bloques_fd = fileno(bloques);

		ftruncate(bloques_fd, tam_bloque);

//manejar_peticiones_kernel(logger, socket_fs, socket_memoria, bloques, superbloque);
		manejar_peticiones();

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
	close(socket_memoria);
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

 // conexiones
int conectar_memoria(char* ip, char* puerto){

	socket_memoria = crear_conexion(ip, puerto);

	//enviar handshake
	enviar_mensaje("OK", socket_memoria, HANDSHAKE);

	op_code cod_op = recibir_operacion(socket_memoria);
	if(cod_op != HANDSHAKE){
		return -1;	}

	int size;
	char* buffer = recibir_buffer(&size, socket_memoria);


	if(strcmp(buffer, "OK") != 0){
		return -1;
	}

	return 0;
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
				abrir_archivo();
				break;
			case CREAR_ARCHIVO:
				crear_archivo();
				break;
			case CERRAR_ARCHIVO:
				cerrar_archivo();
				break;
			case TRUNCAR_ARCHIVO:
				truncar_archivo();
					break;
			case LEER_ARCHIVO:
				leer_archivo();
				break;
			case ESCRIBIR_ARCHIVO:
				escribir_archivo();
				break;
			case NUEVO_PROCESO_FS:
				reservar_bloques();
				break;
			case FINALIZAR_PROCESO_FS:
				marcar_bloques_libres();
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
