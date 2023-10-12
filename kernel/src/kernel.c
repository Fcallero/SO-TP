#include "kernel.h"


int socket_cpu_dispatch;
int socket_cpu_interrupt;
int socket_kernel;
int socket_memoria;
int socket_fs;
int grado_max_multiprogramacion;
char** recursos;
int* recursos_disponibles;
int quantum;
char** instancias_recursos;

int main(int argc, char* argv[]) {

	//Declaraciones de variables para config:

	char* ip_memoria;
	char* puerto_memoria;
	char* ip_filesystem;
	char* puerto_filesystem;
	char* ip_cpu;
	char* puerto_cpu_dispatch;
	char* puerto_cpu_interrupt;


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
	puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
	puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");

	algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	quantum = config_get_int_value(config, "QUANTUM");
	grado_max_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION_INI");

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
		terminar_programa(logger, config);
	}

	log_info(logger, "El Kernel se conecto con el modulo Memoria correctamente");



	int result_conexion_filesystem = conectar_fs(ip_filesystem, puerto_filesystem);

	if(result_conexion_filesystem == -1){
		log_error(logger, "No se pudo conectar con el modulo filesystem !!");
		terminar_programa(logger, config);
	}

	log_info(logger, "El Kernel se conecto con el modulo Filesystem correctamente");


	//PUERTO INTERRUPT
	int result_conexion_cpu_interrupt = conectar_cpu_interrupt(ip_cpu, puerto_cpu_interrupt);

	if(result_conexion_cpu_interrupt == -1){
		log_error(logger, "No se pudo conectar con el interrupt de la CPU !!");
		terminar_programa(logger, config);
	}

	log_info(logger, "El Kernel se conecto con el interrupt de la CPU correctamente");

	//PUERTO DISPATCH
	int result_conexion_cpu_dispatch = conectar_cpu_dispatch(ip_cpu, puerto_cpu_dispatch);

	if(result_conexion_cpu_dispatch == -1){
		log_error(logger, "No se pudo conectar con el dispatch de la CPU !!");
		terminar_programa(logger, config);
	}

	log_info(logger, "El Kernel se conecto con el dispatch de la CPU correctamente");


	// creo array de int de recursos disponibles

	int cantidad_de_recursos = string_array_size(instancias_recursos);
	recursos_disponibles = malloc(sizeof(int)*cantidad_de_recursos);

	if(cantidad_de_recursos!=0){
		for(int i = 0; i< cantidad_de_recursos; i++ ){
			recursos_disponibles[i] = atoi(instancias_recursos[i]);
		}
	}

	inicializar_colas_y_semaforos();

	// inicializo diccionarios para recursos bloqueados
	recurso_bloqueado = dictionary_create();
	colas_de_procesos_bloqueados_para_cada_archivo = dictionary_create();

	void _iterar_recursos(char* nombre_recurso){
		t_queue* cola_bloqueados = queue_create();

		dictionary_put(recurso_bloqueado, nombre_recurso, cola_bloqueados);
	}

	string_iterate_lines(recursos, _iterar_recursos);


	//levanto 4 hilos para recibir peticiones de forma concurrente de los modulos

	pthread_t hilo_peticiones_cpu_dispatch, hilo_peticiones_cpu_interrupt, hilo_peticiones_memoria, hilo_peticiones_filesystem, hilo_planificador_largo_plazo, hilo_planificador_corto_plazo;

	t_args_manejar_peticiones_modulos* args_dispatch = malloc(sizeof(t_args_manejar_peticiones_modulos));
	t_args_manejar_peticiones_modulos* args_interrupt = malloc(sizeof(t_args_manejar_peticiones_modulos));
	t_args_manejar_peticiones_modulos* args_memoria = malloc(sizeof(t_args_manejar_peticiones_modulos));
	t_args_manejar_peticiones_modulos* args_filesystem = malloc(sizeof(t_args_manejar_peticiones_modulos));

	args_dispatch->cliente_fd = socket_cpu_dispatch;
	args_interrupt->cliente_fd = socket_cpu_interrupt;
	args_memoria->cliente_fd = socket_memoria;
	args_filesystem->cliente_fd = socket_fs;


	pthread_create(&hilo_planificador_corto_plazo, NULL, planificar_nuevos_procesos_corto_plazo, NULL);
	pthread_create(&hilo_planificador_largo_plazo, NULL, planificar_nuevos_procesos_largo_plazo, NULL);
	pthread_create(&hilo_peticiones_cpu_dispatch, NULL, escuchar_peticiones_cpu_dispatch, args_dispatch);
	pthread_create(&hilo_peticiones_cpu_interrupt, NULL, manejar_peticiones_modulos, args_interrupt);
	pthread_create(&hilo_peticiones_memoria, NULL, manejar_peticiones_modulos, args_memoria);
	pthread_create(&hilo_peticiones_filesystem, NULL, manejar_peticiones_modulos, args_filesystem);



	pthread_detach(hilo_peticiones_cpu_dispatch);
	pthread_detach(hilo_peticiones_cpu_interrupt);
	pthread_detach(hilo_peticiones_memoria);
	pthread_detach(hilo_peticiones_filesystem);
	pthread_detach(hilo_planificador_largo_plazo);


	//espero peticiones por consola
	levantar_consola();


//	terminar_programa(logger, config);
	free(args_dispatch);
	free(args_interrupt);
	free(args_memoria);
	free(args_filesystem);

} //Fin del main

/*-----------------------DECLARACION DE FUNCIONES--------------------------------------------*/

//Iniciar archivo de log y de config

t_log* iniciar_logger(void){
	// con "false" le decis que no muestres log logs por stdout
	t_log* nuevo_logger = log_create("kernel.log", "Kernel", false, LOG_LEVEL_INFO);
	return nuevo_logger;
}

t_config* iniciar_config(void){
	t_config* nueva_config = config_create("kernel.config");
	return nueva_config;
}


//Finalizar el programa

 void terminar_programa(t_log* logger, t_config* config){
	log_destroy(logger);
	config_destroy(config);
	close(socket_cpu_dispatch);
	close(socket_cpu_interrupt);
	close(socket_fs);
	close(socket_memoria);
 }


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

int conectar_cpu_dispatch(char* ip, char* puerto){

	socket_cpu_dispatch = crear_conexion(ip, puerto);

	//enviar handshake
	enviar_mensaje("OK", socket_cpu_dispatch, HANDSHAKE);
	op_code cod_op = recibir_operacion(socket_cpu_dispatch);

	if(cod_op != HANDSHAKE){
		return -1;	}

	int size;
	char* buffer = recibir_buffer(&size, socket_cpu_dispatch);

	if(strcmp(buffer, "OK") != 0){
		return -1;
	}

	return 0;
}

int conectar_cpu_interrupt(char* ip, char* puerto){

	socket_cpu_interrupt = crear_conexion(ip, puerto);

	//enviar handshake
	enviar_mensaje("OK", socket_cpu_interrupt, HANDSHAKE);
	op_code cod_op = recibir_operacion(socket_cpu_interrupt);

	if(cod_op != HANDSHAKE){
		return -1;
	}

	int size;
	char* buffer = recibir_buffer(&size, socket_cpu_interrupt);

	if(strcmp(buffer, "OK") != 0){
		return -1;
	}

	return 0;
}

//aca se maneja las peticiones de todos los modulos menos los de CPU
void* manejar_peticiones_modulos(void* args){

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
}

// ---------------------------- Peticiones CPU ----------------------------------------------

void *escuchar_peticiones_cpu_dispatch(void* args){

	uint64_t cliente_fd = (uint64_t) args;
	int cantidad_de_recursos = string_array_size(instancias_recursos);

	while(1){
		int cod_op = recibir_operacion(cliente_fd);

			switch (cod_op) {
				case MENSAJE:
					break;
				case HANDSHAKE:
					break;
				case CREAR_PROCESO:
					break;
				case FINALIZAR_PROCESO:
					break;
				case BLOQUEAR_PROCESO:
					break;
				case APROPIAR_RECURSOS:
					apropiar_recursos(cliente_fd, recursos, recursos_disponibles, cantidad_de_recursos);
					break;
				case DESALOJAR_RECURSOS:
					desalojar_recursos(cliente_fd, recursos, recursos_disponibles, cantidad_de_recursos);
					break;
				case DESALOJAR_PROCESO:
					break;
				case SLEEP:
					manejar_sleep(cliente_fd);
					break;
				case ABRIR_ARCHIVO:
					break;
				case CERRAR_ARCHIVO:
					break;
				case TRUNCAR_ARCHIVO:
					break;
				case APUNTAR_ARCHIVO:
					break;
				case LEER_ARCHIVO:
					break;
				case ESCRIBIR_ARCHIVO:
					break;
				case CREAR_ARCHIVO:
					break;
				case ACCESO_A_PAGINA:
					break;
				case PAGE_FAULT:
					break;
				case NUEVO_PROCESO_MEMORIA:
					break;
				case FINALIZAR_PROCESO_MEMORIA:
					break;
				case READ_MEMORY:
					break;
				case WRITE_MEMORY:
					break;
				case -1:
					log_error(logger, "La CPU se desconecto. Terminando servidor ");
					free(recursos_disponibles);
					return NULL;
				default:
					log_warning(logger,"CPU Operacion desconocida. No quieras meter la pata. cod_op: %d", cod_op );
					break;
			}
	}

	return NULL;
}

