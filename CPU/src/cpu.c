#include "cpu.h"

int socket_cpu_dispatch;
int socket_cpu_interrupt;
int socket_memoria;
bool hay_interrupcion_pendiente = false;

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
				case INTERRUPCION:
					recibir_interrupcion(cliente_fd);
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
				manejar_peticion_al_cpu(cliente_fd);
				break;
			case -1:
				log_error(logger, "El cliente se desconecto. Terminando servidor");
				return;
			default:
				log_warning(logger, "Operacion desconocida. No quieras meter la pata");
				break;
		}
	}

}


//FETCH DECODE, EXCEC y CHECK INTERRUPT
void manejar_peticion_al_cpu(int socket_kernel)
{
	t_contexto_ejec *contexto_actual = recibir_contexto_de_ejecucion(socket_kernel);
	bool continuar_con_el_ciclo_instruccion = true;

	while(continuar_con_el_ciclo_instruccion){

		//FETCH

		log_info(logger, "PID: %d - FETCH - Program Counter: %d", contexto_actual->pid, contexto_actual->program_counter);

		contexto_actual->instruccion = recibir_instruccion_memoria(contexto_actual->program_counter+1);

		t_instruccion *instruccion = contexto_actual->instruccion;

		if(instruccion->parametro1_lenght == 0){
			log_info(logger, "PID: %d - Ejecutando: %s ", contexto_actual->pid, instruccion->opcode);

		} else if(instruccion->parametro2_lenght == 0){
			log_info(logger, "PID: %d - Ejecutando: %s - %s", contexto_actual->pid, instruccion->opcode, instruccion->parametros[0]);

		}else if(instruccion->parametro3_lenght == 0){
			log_info(logger, "PID: %d - Ejecutando: %s - %s %s", contexto_actual->pid, instruccion->opcode, instruccion->parametros[0], instruccion->parametros[1]);

		} else {
			log_info(logger, "PID: %d - Ejecutando: %s - %s %s %s", contexto_actual->pid, instruccion->opcode, instruccion->parametros[0], instruccion->parametros[1], instruccion->parametros[2]);
		}


		//DECODE y EXECUTE

		if(strcmp(instruccion->opcode,"SET")==0)
		{
			manejar_instruccion_set(&contexto_actual, instruccion);
			continuar_con_el_ciclo_instruccion = false;
		}

		if(strcmp(instruccion->opcode,"SUM")==0)
		{
			//TODO arreglar este warning
			manejar_instruccion_sum(contexto_actual,instruccion);
			continuar_con_el_ciclo_instruccion = false;

		}
		if(strcmp(instruccion->opcode,"SUB")==0)
		{
			//TODO arreglar este warning
			manejar_instruccion_sub(contexto_actual,instruccion);
			continuar_con_el_ciclo_instruccion = false;
		}
		if(strcmp(instruccion->opcode,"JNZ")==0)
		{

		continuar_con_el_ciclo_instruccion = false;

		}

		if(strcmp(instruccion->opcode,"SLEEP")==0)
		{

			continuar_con_el_ciclo_instruccion = false;

		}

		if(strcmp(instruccion->opcode,"MOV_IN")==0)
		{
			continuar_con_el_ciclo_instruccion = false;
		}

		if(strcmp(instruccion->opcode,"MOV_OUT")==0)
		{

			continuar_con_el_ciclo_instruccion = false;

		}

		if(strcmp(instruccion->opcode,"F_OPEN")==0)
		{

			continuar_con_el_ciclo_instruccion = false;
		}
		if(strcmp(instruccion->opcode,"F_CLOSE")==0)
		{

			continuar_con_el_ciclo_instruccion = false;
		}
		if(strcmp(instruccion->opcode,"F_SEEK")==0)
		{

			continuar_con_el_ciclo_instruccion = false;
		}
		if(strcmp(instruccion->opcode,"F_READ")==0)
		{


			continuar_con_el_ciclo_instruccion = false;
		}
		if(strcmp(instruccion->opcode,"F_WRITE")==0)
		{


			continuar_con_el_ciclo_instruccion = false;
		}
		if(strcmp(instruccion->opcode,"F_TRUNCATE")==0)
		{

			continuar_con_el_ciclo_instruccion = false;
		}

		if(strcmp(instruccion->opcode,"WAIT")==0)
		{

			continuar_con_el_ciclo_instruccion = false;
		}
		if(strcmp(instruccion->opcode,"SIGNAL")==0)
		{

			continuar_con_el_ciclo_instruccion = false;
		}

		if(strcmp(instruccion->opcode,"EXIT")==0)
		{
			devolver_a_kernel(contexto_actual,FINALIZAR_PROCESO, socket_kernel);

			continuar_con_el_ciclo_instruccion = false;
		}

		contexto_actual->program_counter ++;

		if(hay_interrupcion_pendiente){
			continuar_con_el_ciclo_instruccion = false;
			devolver_a_kernel(contexto_actual, INTERRUPCION, socket_kernel);
		}
	}

	contexto_ejecucion_destroy(contexto_actual);
}

void recibir_interrupcion(int socket_kernel){

	char* mensaje = recibir_mensaje(socket_kernel);

	log_info(logger, "Interrupcion - Motivo: %s", mensaje);

	hay_interrupcion_pendiente = true;
}

void devolver_a_kernel(t_contexto_ejec* contexto, op_code code, int socket_cliente){

	t_paquete *paquete_contexto = crear_paquete(code);

	agregar_a_paquete_sin_agregar_tamanio(paquete_contexto, &(contexto->pid), sizeof(int));

	agregar_a_paquete_sin_agregar_tamanio(paquete_contexto, &(contexto->program_counter), sizeof(int));

	agregar_a_paquete_sin_agregar_tamanio(paquete_contexto, &(contexto->registros_CPU->AX), sizeof(uint32_t));
	agregar_a_paquete_sin_agregar_tamanio(paquete_contexto, &(contexto->registros_CPU->BX), sizeof(uint32_t));
	agregar_a_paquete_sin_agregar_tamanio(paquete_contexto, &(contexto->registros_CPU->CX), sizeof(uint32_t));
	agregar_a_paquete_sin_agregar_tamanio(paquete_contexto, &(contexto->registros_CPU->DX), sizeof(uint32_t));

	agregar_a_paquete(paquete_contexto, contexto->instruccion->opcode, sizeof(contexto->instruccion->opcode_lenght));
	agregar_a_paquete(paquete_contexto, contexto->instruccion->parametros[0], sizeof(contexto->instruccion->parametro1_lenght));
	agregar_a_paquete(paquete_contexto, contexto->instruccion->parametros[1], sizeof(contexto->instruccion->parametro2_lenght));
	agregar_a_paquete(paquete_contexto, contexto->instruccion->parametros[2], sizeof(contexto->instruccion->parametro3_lenght));


	enviar_paquete(paquete_contexto, socket_cliente);

	eliminar_paquete(paquete_contexto);
}

t_instruccion *recibir_instruccion_memoria(int program_counter){

	t_paquete *paquete_program_counter = crear_paquete(INSTRUCCION);

	agregar_a_paquete_sin_agregar_tamanio(paquete_program_counter, &program_counter, sizeof(int));

	enviar_paquete(paquete_program_counter, socket_memoria);

	t_instruccion* instruccion = recibir_instruccion(socket_memoria);

	eliminar_paquete(paquete_program_counter);
	return instruccion;
}

void enviar_instruccion_a_kernel(op_code code,int cliente_fd,t_instruccion* instruccion )
{
	t_paquete* paquete = crear_paquete(code);

	agregar_a_paquete(paquete, instruccion->opcode, sizeof(char)*instruccion->opcode_lenght );

	agregar_a_paquete(paquete, instruccion->parametros[0], instruccion->parametro1_lenght);
	agregar_a_paquete(paquete, instruccion->parametros[1], instruccion->parametro2_lenght);
	agregar_a_paquete(paquete, instruccion->parametros[2], instruccion->parametro3_lenght);
	enviar_paquete(paquete, cliente_fd);
}

void manejar_instruccion_set(t_contexto_ejec** contexto,t_instruccion* instruccion)
{
	char* registro = strdup(instruccion->parametros[0]);
	int valor = atoi(instruccion->parametros[1]);
	setear_registro(contexto, registro, valor);
}

void setear_registro(t_contexto_ejec** contexto,char* registro, int valor)
{
	if(strcmp(registro,"AX")==0)
	{
		(*contexto)->registros_CPU->AX=valor;
	}else if(strcmp(registro,"BX")==0)
	{
		(*contexto)->registros_CPU->BX=valor;
	}else if(strcmp(registro,"CX")==0)
	{
		(*contexto)->registros_CPU->CX=valor;
	}else if(strcmp(registro,"DX")==0)
	{
		(*contexto)->registros_CPU->DX=valor;
	}

}

void manejar_instruccion_sum(t_contexto_ejec** contexto_actual,t_instruccion* instruccion)
{
	 char* registro_destino = strdup(instruccion->parametros[0]);
	 char* registro_origen = strdup(instruccion->parametros[1]);

	 int valor_destino = obtener_valor_del_registro(registro_destino, contexto_actual);
	 int valor_origen = obtener_valor_del_registro(registro_origen, contexto_actual);

	 valor_destino=valor_destino+valor_origen;

	 setear_registro(contexto_actual, registro_destino, valor_destino);
}

void manejar_instruccion_sub(t_contexto_ejec** contexto_actual,t_instruccion* instruccion)
{
	 char* registro_destino = strdup(instruccion->parametros[0]);
	 char* registro_origen = strdup(instruccion->parametros[1]);

	 int valor_destino = obtener_valor_del_registro(registro_destino, contexto_actual);
	 int valor_origen = obtener_valor_del_registro(registro_origen, contexto_actual);

	 valor_destino=valor_destino-valor_origen;

	 setear_registro(contexto_actual, registro_destino, valor_destino);
}

int obtener_valor_del_registro(char* registro_a_leer, t_contexto_ejec** contexto_actual){
	int valor_leido;

	if(strcmp(registro_a_leer,"AX")==0)
		{

		valor_leido= (*contexto_actual)->registros_CPU->AX;

		}else if(strcmp(registro_a_leer,"BX")==0)
		{

			valor_leido= (*contexto_actual)->registros_CPU->BX;

		}else if(strcmp(registro_a_leer,"CX")==0)
		{

			valor_leido= (*contexto_actual)->registros_CPU->CX;

		}else if(strcmp(registro_a_leer,"DX")==0)
		{
			valor_leido= (*contexto_actual)->registros_CPU->DX;
		}

	return valor_leido;
}


