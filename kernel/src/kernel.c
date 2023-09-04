#include "kernel.h"

int socket_cpu;
int socket_kernel;
int socket_memoria;
int socket_fs;
int grado_max_multiprogramacion;

int main(int argc, char* argv[]) {

	//Declaraciones de variables para config:

	char* ip_memoria;
	int puerto_memoria;
	char* ip_filesystem;
	int puerto_filesystem;
	char* ip_cpu;
	int puerto_cpu_dispatch;
	int puerto_cpu_interrupt;
	char* algoritmo_planificacion;
	int quantum;
	char** recursos;
	char** instancias_recursos;
	int grado_multiprogramacion;

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

	ip_filesystem = config_get_string_value(config, "IP_FILESYSTEM");
	puerto_filesystem = config_get_string_value(config, "PUERTO_FILESYSTEM");

	ip_cpu = config_get_string_value(config, "IP_CPU");
	puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU");
	puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU");

	algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	quantum = config_get_int_value(config, "ESTIMACION_INICIAL");
	grado_multiprogramacion = config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION");

	recursos = config_get_array_value(config, "RECURSOS");
	instancias_recursos = config_get_array_value(config, "INSTANCIAS_RECURSOS");

	// Control archivo configuracion
	if(!ip_memoria || !puerto_memoria || !ip_filesystem || !puerto_filesystem || !ip_cpu || !puerto_cpu_dispatch || !puerto_cpu_interrupt || !algoritmo_planificacion || !quantum || !recursos || !instancias_recursos){
		log_error(logger, "Error al recibir los datos del archivo de configuracion del kernel");
		terminar_programa(logger, config);
	}

/*-------------------------------CONEXIONES KERNEL---------------------------------------------------------------*/

	// Realizar las conexiones y probarlas
	int result_conexion_memoria = conectar_memoria(ip_memoria, puerto_memoria);

	if(result_conexion_memoria  == -1){
		log_error(logger, "No se pudo conectar con el modulo Memoria !!");
		terminar_programa(logger, config);}

	log_info(logger, "El Kernel se conecto con el modulo Memoria correctamente");



	int result_conexion_filesystem = conectar_fs(ip_filesystem, puerto_filesystem);

	if(result_conexion_filesystem == -1){
		log_error(logger, "No se pudo conectar con el modulo filesystem !!");
		terminar_programa(logger, config);}

	log_info(logger, "El Kernel se conecto con el modulo Filesystem correctamente");


	int result_conexion_cpu_dispatch = conectar_cpu(ip_cpu, puerto_cpu_dispatch);

	if(result_conexion_cpu_dispatch == -1){
		log_error(logger, "No se pudo conectar con el dispatch de la CPU !!");
		terminar_programa(logger, config);}

	log_info(logger, "El Kernel se conecto con el dispatch de la CPU correctamente");


	int result_conexion_cpu_dispatch = conectar_cpu(ip_cpu, puerto_cpu_interrupt);

	if(result_conexion_cpu_dispatch == -1){
		log_error(logger, "No se pudo conectar con el interrupt de la CPU !!");
		terminar_programa(logger, config);}

	log_info(logger, "El Kernel se conecto con el interrupt de la CPU correctamente");


} //Fin del main

/*-----------------------DECLARACION DE FUNCIONES--------------------------------------------*/

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


// CREAR CONEXIONES --------------------------------------------------------------------

int conectar_memoria(char* ip, char* puerto){

	socket_memoria = crear_conexion(ip, puerto);

	//enviar handshake
	enviar_mensaje("OK", socket_memoria, HANDSHAKE);

	op_code cod_op = recibir_operacion(socket_memoria);
	if(cod_op != HANDSHAKE){
		return -1;
	}

	int size;
	char* buffer = recibir_buffer(&size, socket_memoria);


	if(strcmp(buffer, "OK") != 0){
		return -1;
	}

	return 0;
}

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

int conectar_cpu(char* ip, char* puerto){

	socket_cpu = crear_conexion(ip, puerto);

	//enviar handshake
	enviar_mensaje("OK", socket_cpu, HANDSHAKE);

	op_code cod_op = recibir_operacion(socket_cpu);

	if(cod_op != HANDSHAKE){
		return -1;
	}

	int size;
	char* buffer = recibir_buffer(&size, socket_cpu);


	if(strcmp(buffer, "OK") != 0){
		return -1;
	}


	return 0;
}


