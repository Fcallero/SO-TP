#include "cpu_instrucciones.h"

int tamano_pagina, socket_memoria;

void setear_registro(t_contexto_ejec** contexto, char* registro, uint32_t valor);
uint32_t obtener_valor_del_registro(char* registro_a_leer, t_contexto_ejec** contexto_actual);
int traducir_direccion_logica(int direccion_logica, int pid);
void guardar_valor_en(int direccion_fisica, uint32_t valor_a_guardar,  int pid);
uint32_t leer_valor(int direccion_fisica,  int pid);

void manejar_instruccion_set(t_contexto_ejec** contexto, t_instruccion* instruccion)
{
	char* registro = strdup(instruccion->parametros[0]);
	uint32_t valor = atoi(instruccion->parametros[1]);
	setear_registro(contexto, registro, valor);
}

void setear_registro(t_contexto_ejec** contexto, char* registro, uint32_t valor)
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

	free(registro);
}

void manejar_instruccion_sum(t_contexto_ejec** contexto_actual,t_instruccion* instruccion)
{
	 char* registro_destino = strdup(instruccion->parametros[0]);
	 char* registro_origen = strdup(instruccion->parametros[1]);

	 int valor_destino = obtener_valor_del_registro(registro_destino, contexto_actual);
	 int valor_origen = obtener_valor_del_registro(registro_origen, contexto_actual);

	 valor_destino=valor_destino+valor_origen;

	 setear_registro(contexto_actual, registro_destino, valor_destino);
	 free(registro_origen);
}

void manejar_instruccion_sub(t_contexto_ejec** contexto_actual,t_instruccion* instruccion)
{
	 char* registro_destino = strdup(instruccion->parametros[0]);
	 char* registro_origen = strdup(instruccion->parametros[1]);

	 int valor_destino = obtener_valor_del_registro(registro_destino, contexto_actual);
	 int valor_origen = obtener_valor_del_registro(registro_origen, contexto_actual);

	 valor_destino=valor_destino-valor_origen;

	 setear_registro(contexto_actual, registro_destino, valor_destino);
	 free(registro_origen);
}

void manejar_instruccion_jnz(t_contexto_ejec** contexto_actual, t_instruccion* instruccion){
	int numero_instruccion = atoi(instruccion->parametros[1]);

	(*contexto_actual)->program_counter = numero_instruccion;
}

bool menjar_mov_in(t_contexto_ejec** contexto_actual, t_instruccion*  instruccion){
	char* registro = instruccion->parametros[0];
	int direccion_logica = atoi(instruccion->parametros[1]);

	int direccion_fisica = traducir_direccion_logica(direccion_logica, (*contexto_actual)->pid);

	if(direccion_fisica == -1){
		return true;
	}

	uint32_t valor_a_guardar = leer_valor(direccion_fisica, (*contexto_actual)->pid);

	setear_registro(contexto_actual, registro, valor_a_guardar);

	return false;
}

bool menjar_mov_out(t_contexto_ejec** contexto_actual, t_instruccion*  instruccion){
	int direccion_logica = atoi(instruccion->parametros[0]);
	char* registro = instruccion->parametros[1];

	uint32_t valor_a_guardar = obtener_valor_del_registro(registro, contexto_actual);

	int direccion_fisica = traducir_direccion_logica(direccion_logica, (*contexto_actual)->pid);

	if(direccion_fisica == -1){
		return true;
	}

	guardar_valor_en(direccion_fisica, valor_a_guardar, (*contexto_actual)->pid);

	return false;
}

uint32_t obtener_valor_del_registro(char* registro_a_leer, t_contexto_ejec** contexto_actual){
	uint32_t valor_leido = -1; //valor devuelto si se escribe mal el nombre del registro

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

int solicitar_marco(int numero_pagina){
	t_paquete* paquete = crear_paquete(ACCESO_A_PAGINA);

	agregar_a_paquete_sin_agregar_tamanio(paquete, &numero_pagina, sizeof(int));


	enviar_paquete(paquete, socket_memoria);
	eliminar_paquete(paquete);

	int size;
	void* buffer = recibir_buffer(&size, socket_memoria);
	int marco_pagina;

	op_code cod_op = recibir_operacion(socket_memoria);

	if(cod_op == ACCESO_A_PAGINA){
		memcpy(&marco_pagina, buffer, sizeof(int));

		free(buffer);
		return marco_pagina;

	} else { // sino significaria page fault
		return -1;
	}

}

int traducir_direccion_logica(int direccion_logica, int pid){

	int numero_pagina = floor(direccion_logica / tamano_pagina);

	int desplazamiento = direccion_logica - numero_pagina * tamano_pagina;

	int marco_pagina = solicitar_marco(numero_pagina);

	if(marco_pagina == -1){
		log_info(logger, "Page Fault PID: %d - Página: %d", pid, numero_pagina);
		return -1; // page fault
	} else {
		log_info(logger, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d", pid, numero_pagina, marco_pagina);
	}

	return marco_pagina + desplazamiento;
}

void guardar_valor_en(int direccion_fisica, uint32_t valor_a_guardar, int pid){
	t_paquete* paquete_a_enviar = crear_paquete(WRITE_MEMORY);

	t_instruccion* instruccion = malloc(sizeof(t_instruccion));

	//esto no se usa del otro lado, pero hay que hacerlo para evitar errores
	instruccion->opcode = 	string_new();
	string_append(&(instruccion->opcode), "MOV_OUT");
	instruccion->opcode_lenght = strlen(instruccion->opcode) +1;

	instruccion->parametros[0] = string_itoa(direccion_fisica);
	instruccion->parametro1_lenght = strlen(instruccion->parametros[0]) +1;

	char *bytes_a_escribir = string_itoa(sizeof(valor_a_guardar));
	instruccion->parametro2_lenght = strlen(bytes_a_escribir) + 1;
	instruccion->parametros[1] = bytes_a_escribir;

	instruccion->parametro3_lenght = 0;

	agregar_a_paquete_sin_agregar_tamanio(paquete_a_enviar, &pid, sizeof(int));
	agregar_a_paquete(paquete_a_enviar, instruccion->opcode, instruccion->opcode_lenght );

	agregar_a_paquete(paquete_a_enviar, instruccion->parametros[0], instruccion->parametro1_lenght);
	agregar_a_paquete(paquete_a_enviar, instruccion->parametros[1], instruccion->parametro2_lenght);

	agregar_a_paquete_sin_agregar_tamanio(paquete_a_enviar, &valor_a_guardar, sizeof(valor_a_guardar));


	enviar_paquete(paquete_a_enviar, socket_memoria);

	instruccion_destroy(instruccion);
	eliminar_paquete(paquete_a_enviar);

	int cod_op = recibir_operacion(socket_memoria);

	if(cod_op == WRITE_MEMORY){

		// recibo el OK de memoria
		char* mensaje = recibir_mensaje(socket_memoria);

		if(strcmp(mensaje,"OK") == 0){
			log_info(logger,"PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d ", pid, direccion_fisica, valor_a_guardar);
		} else {
			// si esto se llega a ejecutar, debe haber algun error en memoria o aca o algo raro falla
			log_error(logger,"PID: %d - no se pudo escribir en memoria, esto no deberia pasar", pid );
		}
	} else {
		log_error(logger,"Se recibio cod_op: %d esto no deberia pasar", cod_op );
	}
}

uint32_t leer_valor(int direccion_fisica, int pid){
	t_paquete* paquete_a_enviar = crear_paquete(READ_MEMORY);

	t_instruccion* instruccion = malloc(sizeof(t_instruccion));

	//esto no se usa del otro lado, pero hay que hacerlo para evitar errores
	instruccion->opcode = 	string_new();
	string_append(&(instruccion->opcode), "MOV_IN");
	instruccion->opcode_lenght = strlen(instruccion->opcode) +1;

	instruccion->parametros[0] = string_itoa(direccion_fisica);
	instruccion->parametro1_lenght = strlen(instruccion->parametros[0]) +1;

	char *bytes_a_escribir = string_itoa(sizeof(int));
	instruccion->parametro2_lenght = strlen(bytes_a_escribir) + 1;
	instruccion->parametros[1] = bytes_a_escribir;

	instruccion->parametro3_lenght = 0;

	agregar_a_paquete_sin_agregar_tamanio(paquete_a_enviar, &pid, sizeof(int));
	agregar_a_paquete(paquete_a_enviar, instruccion->opcode, sizeof(char)*instruccion->opcode_lenght );

	agregar_a_paquete(paquete_a_enviar, instruccion->parametros[0], instruccion->parametro1_lenght);
	agregar_a_paquete(paquete_a_enviar, instruccion->parametros[1], instruccion->parametro2_lenght);

	enviar_paquete(paquete_a_enviar, socket_memoria);

	instruccion_destroy(instruccion);
	eliminar_paquete(paquete_a_enviar);

	int cod_op = recibir_operacion(socket_memoria);

	char* valor_leido;

	if(cod_op == READ_MEMORY){
		// recibo el valor leido de memoria
		valor_leido = recibir_mensaje(socket_memoria);

		log_info(logger,"PID: %d - Acción: LEER - Dirección Física: %d - Valor: %s", pid, direccion_fisica, valor_leido);

	} else {
		log_error(logger,"Se recibio cod_op: %d esto no deberia pasar", cod_op );
	}

	return atoi(valor_leido);
}
