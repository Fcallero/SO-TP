#include "cpu.h"

int socket_cpu_dispatch;
int socket_cpu_interrupt;
int socket_kernel;
int socket_memoria;


int main(int argc, char* argv[]) {

	//Declaraciones de variables para config:

	char* ip_memoria;
	char* puerto_memoria;
	char* puerto_escucha_dispatch;
	char* puerto_escucha_interrupt;

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

	puerto_escucha_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");


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

	log_info(logger, "CPU se conecto con el modulo Memoria correctamente");


	//Esperar conexion de Kernel
	socket_cpu_dispatch = iniciar_servidor(puerto_escucha_dispatch);

	socket_cpu_interrupt = iniciar_servidor(puerto_escucha_interrupt);


	log_info(logger, "CPU esta lista para recibir peticiones o interrupciones");

	// levanto hilo para el puerto interrupt


	pthread_t thread_interrupt;
	uint64_t cliente_fd = (uint64_t) esperar_cliente(socket_cpu_interrupt);

	pthread_create(&thread_interrupt, NULL, manejar_interrupciones, (void*) cliente_fd);

	pthread_detach(thread_interrupt);


	//escucho peticiones para puerto dispatch
	manejar_peticiones_instruccion();

	terminar_programa(logger, config);
} //Fin del main

/*-----------------------DECLARACION DE FUNCIONES--------------------------------------------*/

//Iniciar archivo de log y de config

t_log* iniciar_logger(void){
	t_log* nuevo_logger = log_create("cpu.log", "CPU", true, LOG_LEVEL_INFO);
	return nuevo_logger;
}

t_config* iniciar_config(void){
	t_config* nueva_config = config_create("cpu.config");
	return nueva_config;
}


//Finalizar el programa

 void terminar_programa(t_log* logger, t_config* config){
	log_destroy(logger);
	config_destroy(config);
	close(socket_cpu_interrupt);
	close(socket_cpu_dispatch);
	close(socket_memoria);
 }


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
 		return -1;
 	}

 	return 0;
 }

 //interrupciones
void* manejar_interrupciones(void* args){
	uint64_t cliente_fd = (uint64_t) args;

	while(1){
			int cod_op = recibir_operacion(cliente_fd);

			switch(cod_op){
				case MENSAJE:
					recibir_mensaje(cliente_fd);
					break;
				case HANDSHAKE:
					recibir_handshake(cliente_fd);
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

//DISPATCH
void manejar_peticiones_instruccion(){
	uint64_t cliente_fd = (uint64_t) esperar_cliente(socket_cpu_dispatch);

	while(1){
		int cod_op = recibir_operacion(cliente_fd);

		switch(cod_op){
			case MENSAJE:
				recibir_mensaje(cliente_fd);
				break;
			case HANDSHAKE:
				recibir_handshake(cliente_fd);
				break;
			case PETICION_CPU:
				//no confundir con el del tp anterior
				//ahora hay un manejo de interrupciones arriba
				manejar_peticion_al_cpu();
			case -1:
				log_error(logger, "El cliente se desconecto. Terminando servidor");
				return;
			default:
				log_warning(logger, "Operacion desconocida. No quieras meter la pata");
				break;
		}
	}

}


/**
 * Fetch y Decode
 * aca ir agregando las funciones necesarias
 * ------comentar si se necesita compilar-------
 */
void manejar_peticion_al_cpu()
{

	bool continuar_con_el_ciclo_instruccion = true;

	while(continuar_con_el_ciclo_instruccion){


		//DECODE y EXECUTE

		if(strcmp(,"SET")==0)
		{
			continuar_con_el_ciclo_instruccion = false;
		}

		if(strcmp(,"SUM")==0)
		{

				continuar_con_el_ciclo_instruccion = false;

		}


		if(strcmp(,"SUB")==0)
		{

			continuar_con_el_ciclo_instruccion = false;
		}



		if(strcmp(,"JNZ")==0)
		{

		continuar_con_el_ciclo_instruccion = false;

		}


		if(strcmp(,"SLEEP")==0)
		{

		continuar_con_el_ciclo_instruccion = false;

		}

		if(strcmp(,"MOV_IN")==0)
		{

				continuar_con_el_ciclo_instruccion = false;

		}

		if(strcmp(,"MOV_OUT")==0)
		{

				continuar_con_el_ciclo_instruccion = false;

		}


		if(strcmp(,"F_OPEN")==0)
		{

			continuar_con_el_ciclo_instruccion = false;
		}
		if(strcmp(,"F_CLOSE")==0)

			continuar_con_el_ciclo_instruccion = false;
		}
		if(strcmp(,"F_SEEK")==0)
		{

			continuar_con_el_ciclo_instruccion = false;
		}
		if(strcmp(,"F_READ")==0)
		{


			continuar_con_el_ciclo_instruccion = false;
		}
		if(strcmp(,"F_WRITE")==0)
		{


			continuar_con_el_ciclo_instruccion = false;
		}
		if(strcmp(,"F_TRUNCATE")==0)
		{

			continuar_con_el_ciclo_instruccion = false;
		}

		if(strcmp(,"WAIT")==0)
		{

			continuar_con_el_ciclo_instruccion = false;
		}
		if(strcmp(,"SIGNAL")==0)
		{

			continuar_con_el_ciclo_instruccion = false;
		}

		if(strcmp(,"EXIT")==0)
		{

			continuar_con_el_ciclo_instruccion = false;
		}
	}
//aca destruir contexto de ejecucion cuando exista o similar

}
