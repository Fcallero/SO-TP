#include "peticiones_cpu_dispatch.h"

sem_t esperar_proceso_ejecutando;

//este es un hilo que se levantaria al bloquear por io como en el tp anterior
//no estan creadas las estructuras necesarias ni el como lo haremos
//asique dejo esta colgada para cuando llegue el momento
//carinios con enie UwU

void poner_a_ejecutar_otro_proceso(){

	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando = NULL;
	sem_post(&m_proceso_ejecutando);

	sem_post(&despertar_corto_plazo);
}

char* listar_recursos_disponibles(int* recursos_disponibles, int cantidad_de_recursos){
		char** recursos_disponibles_string_array = string_array_new();

		for(int i =0; i< cantidad_de_recursos; i++){
			int diponibilidad_recurso_n = recursos_disponibles[i];

			string_array_push(&recursos_disponibles_string_array, string_itoa(diponibilidad_recurso_n));
			string_array_push(&recursos_disponibles_string_array,"," );
		}

		string_array_pop(recursos_disponibles_string_array);

		return pasar_a_string(recursos_disponibles_string_array) ;
}


void* simular_sleep(void* arg){
	uint32_t *tiempo_sleep = (uint32_t*) arg;

	sem_wait(&m_proceso_ejecutando);
	t_pcb* proceso_en_sleep = proceso_ejecutando;
	sem_post(&m_proceso_ejecutando);

	sem_post(&esperar_proceso_ejecutando);

	esperar_por((*tiempo_sleep) *1000);

	sem_wait(&m_proceso_ejecutando);
	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_en_sleep->PID, "BLOC","READY");


	pasar_a_ready(proceso_en_sleep);
	sem_post(&m_proceso_ejecutando);

	return NULL;
}

void manejar_sleep(int socket_cliente){
	t_contexto_ejec* contexto = (t_contexto_ejec*) recibir_contexto_de_ejecucion(socket_cliente);

	t_instruccion* instruccion = contexto->instruccion;

	pthread_t hilo_simulacion;

	sem_init(&esperar_proceso_ejecutando, 0, 0);

	uint32_t tiempo_sleep = atoi(instruccion->parametros[0]);

	pthread_create(&hilo_simulacion, NULL, simular_sleep, (void *) &tiempo_sleep);

	pthread_detach(hilo_simulacion);

	//se actualiza el program_counter por las dudas
	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando->program_counter = contexto->program_counter;
	sem_post(&m_proceso_ejecutando);


	sem_wait(&esperar_proceso_ejecutando);

	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", contexto->pid, "EXEC","BLOC");
	log_info(logger, "PID: %d - Bloqueado por: SLEEP", contexto->pid);


	poner_a_ejecutar_otro_proceso();

	//destruyo el contexto de ejecucion
	contexto_ejecucion_destroy(contexto);
}

// si no lo encuentra devuelve -1
int obtener_indice_recurso(char** recursos, char* recurso_a_buscar){

	int indice_recurso = 0;
	int tamanio_recursos = string_array_size(recursos);

	if(string_array_is_empty(recursos)){
		return -1;
	}



	while(indice_recurso < tamanio_recursos && strcmp(recurso_a_buscar, recursos[indice_recurso]) != 0 ){
		indice_recurso++;
	}
	if(indice_recurso == tamanio_recursos){
		return -1;
	}

	return indice_recurso;
}

void bloquear_proceso_por_recurso(t_pcb* proceso_a_bloquear, char* nombre_recurso){

	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_a_bloquear->PID, "EXEC","BLOC");
	log_info(logger, "PID: %d - Bloqueado por: %s", proceso_a_bloquear->PID, nombre_recurso);

	t_queue* cola_bloqueados = dictionary_get(recurso_bloqueado,nombre_recurso);

	queue_push(cola_bloqueados,proceso_a_bloquear);

}


void apropiar_recursos(int socket_cliente, char** recursos, int* recurso_disponible, int cantidad_de_recursos){
	t_contexto_ejec* contexto = (t_contexto_ejec*) recibir_contexto_de_ejecucion(socket_cliente);

	t_instruccion* instruccion = contexto->instruccion;


	log_info(logger, "Inicio Wait al recurso %s",instruccion->parametros[0]);

	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando->program_counter = contexto->program_counter;
	sem_post(&m_proceso_ejecutando);

	int indice_recurso = obtener_indice_recurso(recursos, instruccion->parametros[0]);


	// si no existe el recurso finaliza
	if(indice_recurso == -1){

		sem_wait(&m_proceso_ejecutando);
		t_paquete* paquete = crear_paquete(FINALIZAR_PROCESO_MEMORIA);
		agregar_a_paquete_sin_agregar_tamanio(paquete, &(proceso_ejecutando->PID), sizeof(int));
		enviar_paquete(paquete, socket_memoria);

		eliminar_paquete(paquete);

		log_info(logger, "FInaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_ejecutando->PID);
		log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_ejecutando->PID, "EXEC","EXIT");
		sem_post(&m_proceso_ejecutando);

		contexto_ejecucion_destroy(contexto);
		//TODO DESTRUIR PCB aca
		poner_a_ejecutar_otro_proceso();

		return;
	}

	recurso_disponible[indice_recurso] -= 1;

	if(recurso_disponible[indice_recurso] <= 0){
		sem_wait(&m_proceso_ejecutando);
		bloquear_proceso_por_recurso(proceso_ejecutando, recursos[indice_recurso]);
		sem_post(&m_proceso_ejecutando);

		char* recursos_disponibles_string = listar_recursos_disponibles(recurso_disponible, cantidad_de_recursos);
		log_info(logger, "PID: %d - Wait: %s - Instancias: [%s]", contexto->pid,recursos[indice_recurso], recursos_disponibles_string);


		poner_a_ejecutar_otro_proceso();

		free(recursos_disponibles_string);
		contexto_ejecucion_destroy(contexto);

		return;
	}

	char* recursos_disponibles_string = listar_recursos_disponibles(recurso_disponible, cantidad_de_recursos);
	log_info(logger, "PID: %d - Wait: %s - Instancias: [%s]", contexto->pid,recursos[indice_recurso], recursos_disponibles_string);

	free(recursos_disponibles_string);

	// continua ejecutandose el mismo proceso
	enviar_contexto_de_ejecucion_a(contexto, PETICION_CPU, socket_cliente);

	//destruyo el contexto de ejecucion
	contexto_ejecucion_destroy(contexto);
}

void desalojar_recursos(int socket_cliente,char** recursos, int* recurso_disponible, int cantidad_de_recursos){
	t_contexto_ejec* contexto = (t_contexto_ejec*) recibir_contexto_de_ejecucion(socket_cliente);

	t_instruccion* instruccion = contexto->instruccion;

	log_info(logger, "Inicio Signal al recurso %s",instruccion->parametros[0]);

	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando->program_counter = contexto->program_counter;
	sem_post(&m_proceso_ejecutando);

	int indice_recurso = obtener_indice_recurso(recursos, instruccion->parametros[0]);

	// si no existe el recurso finaliza
	if(indice_recurso == -1){

		//TODO pedir_finalizar_las_estructuras_de_memoria();
		sem_wait(&m_proceso_ejecutando);
		t_paquete* paquete = crear_paquete(FINALIZAR_PROCESO_MEMORIA);
		agregar_a_paquete_sin_agregar_tamanio(paquete, &(proceso_ejecutando->PID), sizeof(int));
		enviar_paquete(paquete, socket_memoria);

		eliminar_paquete(paquete);

		log_info(logger, "FInaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_ejecutando->PID);
		log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_ejecutando->PID, "EXEC","EXIT");
		sem_post(&m_proceso_ejecutando);

		contexto_ejecucion_destroy(contexto);
		//TODO DESTRUIR PCB aca
		poner_a_ejecutar_otro_proceso();
		return;
	}

	recurso_disponible[indice_recurso] += 1;

	t_queue* cola_bloqueados= (t_queue*) dictionary_get(recurso_bloqueado,recursos[indice_recurso]);

	//t_pcb* proceso_desbloqueado = queue_size(cola_bloqueados);

	int cantidad_procesos_bloqueados = queue_size(cola_bloqueados);

	if(cantidad_procesos_bloqueados != 0){
		t_pcb* proceso_desbloqueado = queue_pop(cola_bloqueados);

		log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_desbloqueado->PID, "BLOC","READY");

		//actualizo los recursos disponibles para que no se le actualize a otro proceso
		recurso_disponible[indice_recurso] -= 1;

		pasar_a_ready(proceso_desbloqueado);
	}


	char* recursos_disponibles_string = listar_recursos_disponibles(recurso_disponible, cantidad_de_recursos);
	log_info(logger, "PID: %d - Signal: %s - Instancias: [%s]", contexto->pid,recursos[indice_recurso], recursos_disponibles_string);

	free(recursos_disponibles_string);
	//continua ejecutandose el mismo proceso
	enviar_contexto_de_ejecucion_a(contexto, PETICION_CPU, socket_cliente);

	//destruyo el contexto de ejecucion
	contexto_ejecucion_destroy(contexto);
}


