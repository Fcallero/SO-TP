#include "memoria.h"

int socket_cpu;
int socket_kernel;
int socket_memoria;
int socket_fs;
int grado_max_multiprogramacion;

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

		algoritmo_remplazo = config_get_string_value(config, "ALGORITMO_REMPLAZO");

		// Control archivo configuracion
		if(!ip_filesystem || !puerto_filesystem || !puerto_escucha || !tam_memoria || !tam_pagina || !path_instrucciones || !retardo_respuesta || !algoritmo_remplazo){
			log_error(logger, "Error al recibir los datos del archivo de configuracion de la memoria");
			terminar_programa(logger, config);
		}

	/*-------------------------------CONEXIONES MEMORIA---------------------------------------------------------------*/

		//Esperar conexion de FS
		int server_fd = iniciar_servidor(puerto_escucha);

			log_info(logger, "La memoria esta lista para recibir peticiones");

		// Realizar las conexiones y probarlas

		int result_conexion_memoria = conectar_fs(ip_filesystem, puerto_filesystem);

		if(result_conexion_memoria  == -1){
			log_error(logger, "No se pudo conectar con el modulo Memoria !!");
			terminar_programa(logger, config);}

		log_info(logger, "El Filesystem se conecto con el modulo Memoria correctamente");



    return 0;
}

//Iniciar archivo de log y de config

t_log* iniciar_logger(void){
	t_log* nuevo_logger = log_create("kernel.log", "Kernel", true, LOG_LEVEL_INFO);
	return nuevo_logger;}

t_config* iniciar_config(void){
	t_config* nueva_config = config_create("kernel.config");
	return nueva_config;}


//Finalizar el programa

 void terminar_programa(t_log* logger, t_config* config){
	log_destroy(logger);
	config_destroy(config);
	close(socket_cpu);
	close(socket_fs);
	close(socket_memoria);}

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
