#include "memoria.h"

int socket_memoria;
int socket_fs;
void* espacio_usuario;
int tam_pagina;
t_tabla_de_paginas* tabla_de_paginas;
char* path_instrucciones;

int main(int argc, char *argv[]) {

	//Declaraciones de variables para config:

	char *ip_filesystem;
	char *puerto_filesystem;
	char *puerto_escucha;
	int tam_memoria;
	int retardo_respuesta;
	char* algoritmo_reemplazo;

	/*------------------------------LOGGER Y CONFIG--------------------------------------------------*/

	// Iniciar archivos de log y configuracion:
	t_config *config = iniciar_config();
	logger = iniciar_logger();

	// Verificacion de creacion archivo config
	if (config == NULL) {
		log_error(logger,
				"No fue posible iniciar el archivo de configuracion !!");
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

	algoritmo_reemplazo = config_get_string_value(config, "ALGORITMO_REEMPLAZO");

	// Control archivo configuracion
	if(!ip_filesystem || !puerto_filesystem || !puerto_escucha || !tam_memoria || !tam_pagina || !path_instrucciones || !retardo_respuesta || !algoritmo_reemplazo){

		log_error(logger,
				"Error al recibir los datos del archivo de configuracion de la memoria");
		terminar_programa(logger, config);
	}

	/*-------------------------------CONEXIONES MEMORIA---------------------------------------------------------------*/

	// Realizar las conexiones y probarlas
	int result_conexion_fs = conectar_fs(ip_filesystem, puerto_filesystem);

	if (result_conexion_fs == -1) {
		log_error(logger, "No se pudo conectar con el modulo Filesystem !!");
		terminar_programa(logger, config);
	}

	log_info(logger,
			"Memoria se conecto con el modulo Filesystem correctamente");

	//Esperar conexion de FS
	socket_memoria = iniciar_servidor(puerto_escucha);

	log_info(logger, "La memoria esta lista para recibir peticiones");
	/*-------------------------------- CREAR STRUCTS -------------------------------*/

	//espacio de usuario
	espacio_usuario = malloc(tam_memoria);
	lista_instrucciones_porPID=dictionary_create();
	//memoria de instrucciones


	//manejo de peticiones
	manejar_pedidos_memoria(algoritmo_reemplazo,retardo_respuesta,tam_pagina,tam_memoria);

	terminar_programa(logger, config);
	return 0;
} //FIN DEL MAIN

//Iniciar archivo de log y de config

t_log* iniciar_logger(void) {
	t_log *nuevo_logger = log_create("memoria.log", "Memoria", true,
			LOG_LEVEL_INFO);
	return nuevo_logger;
}

t_config* iniciar_config(void) {
	t_config *nueva_config = config_create("memoria.config");
	return nueva_config;
}

//Finalizar el programa

void terminar_programa(t_log *logger, t_config *config) {
	log_destroy(logger);
	config_destroy(config);
	close(socket_fs);
	close(socket_memoria);
}

//conexiones
int conectar_fs(char *ip, char *puerto) {

	socket_fs = crear_conexion(ip, puerto);

	//enviar handshake
	enviar_mensaje("OK", socket_fs, HANDSHAKE);

	op_code cod_op = recibir_operacion(socket_fs);

	if (cod_op != HANDSHAKE) {
		return -1;
	}

	int size;
	char *buffer = recibir_buffer(&size, socket_fs);

	if (strcmp(buffer, "OK") != 0) {
		return -1;
	}

	return 0;
}

// atiende las peticiones de kernel, cpu y filesystem de forma concurrente
void manejar_pedidos_memoria(char* algoritmo_reemplazo,int retardo_respuesta,int tam_pagina,int tam_memoria){

	while (1) {
		pthread_t thread;
		uint64_t cliente_fd = (uint64_t) esperar_cliente(socket_memoria);

		t_arg_atender_cliente *argumentos_atender_cliente = malloc(
				sizeof(t_arg_atender_cliente));
		argumentos_atender_cliente->cliente_fd = cliente_fd;
		argumentos_atender_cliente->algoritmo_reemplazo = algoritmo_reemplazo;
		argumentos_atender_cliente->retardo_respuesta = retardo_respuesta;
		argumentos_atender_cliente->tam_pagina = tam_pagina;
		argumentos_atender_cliente->tam_memoria = tam_memoria;

		pthread_create(&thread, NULL, atender_cliente, (void*) argumentos_atender_cliente);

		pthread_detach(thread);
	}

}

// en el handshake con CPU le envia el tamanio de pagina para la traduccion de direcciones logicas
void enviar_tam_pagina(int socket_cliente){
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);

	if(strcmp(buffer, "OK") == 0){
		t_paquete* paquete = crear_paquete(HANDSHAKE_TAM_MEMORIA);
		agregar_a_paquete_sin_agregar_tamanio(paquete, &tam_pagina, sizeof(tam_pagina));

		enviar_paquete(paquete, socket_cliente);

		eliminar_paquete(paquete);
	} else {
		enviar_mensaje("ERROR", socket_cliente, HANDSHAKE_TAM_MEMORIA);
	}


	free(buffer);
}

//peticiones de kernel, cpu y filesystem
void* atender_cliente(void *args) {
	t_arg_atender_cliente *argumentos = (t_arg_atender_cliente*) args;

	uint64_t cliente_fd = argumentos->cliente_fd;
	char* algoritmo_reemplazo = argumentos->algoritmo_reemplazo;
	uint64_t retardo_respuesta = argumentos->retardo_respuesta;
	uint64_t tam_pagina = argumentos->tam_pagina;
	uint64_t tam_memoria = argumentos->tam_memoria;

	while (1) {
		int cod_op = recibir_operacion(cliente_fd);

		switch(cod_op){
			case MENSAJE:
				recibir_mensaje(cliente_fd);
				break;
			case HANDSHAKE:
				recibir_handshake(cliente_fd);
				break;
			case HANDSHAKE_TAM_MEMORIA:
				enviar_tam_pagina(cliente_fd);
				break;
			case INICIAR_PROCESO:
				 leer_pseudo(cliente_fd);
				break;
			case INSTRUCCION:
				enviar_instruccion_a_cpu(cliente_fd,retardo_respuesta);
				break;
			case FINALIZAR_PROCESO_MEMORIA:
				finalizar_proceso(cliente_fd);
				break;
			case ACCESO_A_PAGINA:
				devolver_marco();
				break;
			case PAGE_FAULT:
				manejar_pagefault();
				break;
			case READ_MEMORY:
				break;
			case WRITE_MEMORY:
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



/* Estas mover a otros archivos en un futuro */


void finalizar_proceso(int cliente_fd){
	//recibe la orden de la consola de kernel
	int size;
	void* buffer = recibir_buffer(&size, cliente_fd);
	int pid;
	memcpy(&pid,buffer,sizeof(int));

	/*op_code opcode = recibir_operacion(cliente_consola);
	if(opcode != FINALIZAR_PROCESO_MEMORIA)
	{
		log_error(logger, "Error de Operacion! codigo de operacion recibido: %d", opcode);
	}

	t_paquete *paquete = recibir_paquete(cliente_consola);
	int pid;
	memcpy(pid,paquete->buffer,sizeof(int));

	free(pcb_a_eliminar);
	//

	//mando a filesystem
	enviar_mensaje("Marcar como libre las paginas correspondiente del SWAP", server_filesystem,FINALIZAR_PROCESO_FS);

	*/

}

void devolver_marco(){

}

void manejar_pagefault(){

}
