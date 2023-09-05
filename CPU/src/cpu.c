#include "cpu.h"

int socket_cpu;
int socket_kernel;
int socket_memoria;
int socket_fs;
int grado_max_multiprogramacion;

int main(int argc, char* argv[]) {

	//Declaraciones de variables para config:

	char* ip_memoria;
	int puerto_memoria;
	int puerto_escucha_dispatch;
	int puerto_escucha_interrupt;

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
	puerto_memoria = config_get_int_value(config, "PUERTO_MEMORIA");

	puerto_escucha_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	puerto_escucha_interrupt = config_get_int_value(config, "PUERTO_ESCUCHA_INTERUPT");


	// Control archivo configuracion
	if(!ip_memoria || !puerto_memoria || !puerto_escucha_interrupt || !puerto_escucha_dispatch ){
		log_error(logger, "Error al recibir los datos del archivo de configuracion del CPU");
		terminar_programa(logger, config);
	}

/*-------------------------------CONEXIONES KERNEL---------------------------------------------------------------*/

	// Realizar las conexiones y probarlas
	int result_conexion_memoria = conectar_memoria(ip_memoria, puerto_memoria);

	if(result_conexion_memoria  == -1){
		log_error(logger, "No se pudo conectar con el modulo Memoria !!");
		terminar_programa(logger, config);}

	log_info(logger, "El Kernel se conecto con el modulo Memoria correctamente");



} //Fin del main

/*-----------------------DECLARACION DE FUNCIONES--------------------------------------------*/

//Iniciar archivo de log y de config

t_log* iniciar_logger(void){
	t_log* nuevo_logger = log_create("cpu.log", "CPU", true, LOG_LEVEL_INFO);
	return nuevo_logger;}

t_config* iniciar_config(void){
	t_config* nueva_config = config_create("cpu.config");
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
 		return -1;	}

 	int size;
 	char* buffer = recibir_buffer(&size, socket_memoria);


 	if(strcmp(buffer, "OK") != 0){
 		return -1;	}

 	return 0;
 }
