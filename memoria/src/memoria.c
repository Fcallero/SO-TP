#include "memoria.h"


int socket_memoria;
int socket_fs;

int main(int argc, char* argv[]) {

	//Declaraciones de variables para config:

	char* ip_filesystem;
	char* puerto_filesystem;
	char* puerto_escucha;
	int tam_memoria;
	int tam_pagina;
	char* path_instrucciones;
	int retardo_respuesta;
	char* algoritmo_remplazo;

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
	ip_filesystem = config_get_string_value(config, "IP_FILESYSTEM");
	puerto_filesystem = config_get_string_value(config, "PUERTO_FILESYSTEM");

	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

	tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
	tam_pagina = config_get_int_value(config, "TAM_PAGINA");

	path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");
	retardo_respuesta = config_get_int_value(config, "RETARDO_RESPUESTA");

	algoritmo_remplazo = config_get_string_value(config, "ALGORITMO_REEMPLAZO");

	// Control archivo configuracion
	if(!ip_filesystem || !puerto_filesystem || !puerto_escucha || !tam_memoria || !tam_pagina || !path_instrucciones || !retardo_respuesta || !algoritmo_remplazo){

		log_error(logger, "Error al recibir los datos del archivo de configuracion de la memoria");
		terminar_programa(logger, config);
	}

/*-------------------------------CONEXIONES MEMORIA---------------------------------------------------------------*/


	// Realizar las conexiones y probarlas

	int result_conexion_fs = conectar_fs(ip_filesystem, puerto_filesystem);

	if(result_conexion_fs == -1){
		log_error(logger, "No se pudo conectar con el modulo Filesystem !!");
		terminar_programa(logger, config);
	}

	log_info(logger, "Memoria se conecto con el modulo Filesystem correctamente");

	//Esperar conexion de FS
	socket_memoria = iniciar_servidor(puerto_escucha);

	log_info(logger, "La memoria esta lista para recibir peticiones");

	manejar_pedidos_memoria();

	terminar_programa(logger, config);
    return 0;
}

//Iniciar archivo de log y de config

t_log* iniciar_logger(void){
	t_log* nuevo_logger = log_create("memoria.log", "Memoria", true, LOG_LEVEL_INFO);
	return nuevo_logger;
}

t_config* iniciar_config(void){
	t_config* nueva_config = config_create("memoria.config");
	return nueva_config;
}


//Finalizar el programa

 void terminar_programa(t_log* logger, t_config* config){
	log_destroy(logger);
	config_destroy(config);
	close(socket_fs);
	close(socket_memoria);
}

//conexiones
int conectar_fs(char* ip, char* puerto){

	socket_fs = crear_conexion(ip, puerto);

	//enviar handshake
	enviar_mensaje("OK", socket_fs, HANDSHAKE);

	op_code cod_op = recibir_operacion(socket_fs);

	if(cod_op != HANDSHAKE){
		return -1;
	}

	int size;
	char* buffer = recibir_buffer(&size, socket_fs);


	if(strcmp(buffer, "OK") != 0){
		return -1;
	}

	return 0;
}

// atiende las peticiones de kernel, cpu y filesystem de forma concurrente
void manejar_pedidos_memoria(){

	while(1){
		pthread_t thread;
		uint64_t cliente_fd = (uint64_t) esperar_cliente(socket_memoria);

		t_arg_atender_cliente* argumentos_atender_cliente = malloc(sizeof(t_arg_atender_cliente));
		argumentos_atender_cliente->cliente_fd = cliente_fd;


		pthread_create(&thread, NULL, atender_cliente, (void*) argumentos_atender_cliente);

		pthread_detach(thread);
	}

}

//peticiones de kernel, cpu y filesystem
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
			case INICIAR_PROCESO:
				crear_proceso(cliente_fd);
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

//Funciones kernel TODO (luego separar en un include)

void crear_proceso(uint64_t cliente_fd){
	t_instruccion* instruccion = recibir_instruccion(cliente_fd);
	log_info(logger, "Instruccion recibida con exito \n");
	log_info(logger, instruccion->opcode);
	log_info(logger, instruccion->parametros[0]);
	log_info(logger, instruccion->parametros[1]);
	log_info(logger, instruccion->parametros[2]);
}






