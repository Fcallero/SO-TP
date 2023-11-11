#include "peticiones_cpu_dispatch.h"

sem_t esperar_proceso_ejecutando;

void poner_a_ejecutar_otro_proceso(){

	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando = NULL;
	sem_post(&m_proceso_ejecutando);

	sem_wait(&m_cola_ready);
	sem_wait(&m_cola_new);

	if(queue_size(cola_ready) == 0 && queue_size(cola_new) > 0){
		sem_post(&m_cola_ready);
		sem_post(&m_cola_new);
		sem_post(&despertar_planificacion_largo_plazo);
	} else {
		sem_post(&m_cola_ready);
		sem_post(&m_cola_new);
		sem_post(&despertar_corto_plazo);
	}
}

char* listar_recursos_disponibles(int* recursos_disponibles, int cantidad_de_recursos){
	char** recursos_disponibles_string_array = string_array_new();

	for(int i =0; i< cantidad_de_recursos; i++){
		int diponibilidad_recurso_n = recursos_disponibles[i];
		char *coma = malloc(3);
		strcpy(coma, ", ");

		string_array_push(&recursos_disponibles_string_array, string_itoa(diponibilidad_recurso_n));
		string_array_push(&recursos_disponibles_string_array,coma );
	}

	free(string_array_pop(recursos_disponibles_string_array));

	char* lista_recursos_disponibles = pasar_a_string(recursos_disponibles_string_array);

	string_array_destroy(recursos_disponibles_string_array);

	return lista_recursos_disponibles;
}


void* simular_sleep(void* arg){
	struct t_arg_tiempo{
		uint32_t tiempo_sleep;
	} *arg_tiempo = arg ;

	uint32_t tiempo_sleep = arg_tiempo->tiempo_sleep;

	sem_wait(&m_proceso_ejecutando);
	t_pcb* proceso_en_sleep = proceso_ejecutando;
	sem_post(&m_proceso_ejecutando);

	sem_post(&esperar_proceso_ejecutando);

	esperar_por((tiempo_sleep) *1000);

	sem_wait(&m_proceso_ejecutando);
	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_en_sleep->PID, "BLOC","READY");

	actualizar_estado_a_pcb(proceso_en_sleep, "READY");
	sem_post(&m_proceso_ejecutando);

	pasar_a_ready(proceso_en_sleep);

	sem_wait(&m_proceso_ejecutando);
	if(proceso_ejecutando == NULL){
		sem_post(&despertar_corto_plazo);
	}
	sem_post(&m_proceso_ejecutando);

	free(arg_tiempo);
	return NULL;
}

void manejar_sleep(int socket_cliente){
	t_contexto_ejec* contexto = (t_contexto_ejec*) recibir_contexto_de_ejecucion(socket_cliente);

	t_instruccion* instruccion = contexto->instruccion;

	pthread_t hilo_simulacion;

	sem_init(&esperar_proceso_ejecutando, 0, 0);

	uint32_t tiempo_sleep = atoi(instruccion->parametros[0]);

	struct t_arg_tiempo{
		uint32_t tiempo_sleep;
	} *arg_tiempo = malloc(sizeof(struct t_arg_tiempo)) ;

	arg_tiempo->tiempo_sleep = tiempo_sleep;

	pthread_create(&hilo_simulacion, NULL, simular_sleep, (void *) arg_tiempo);

	pthread_detach(hilo_simulacion);

	//se actualiza el program_counter por las dudas
	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando->program_counter = contexto->program_counter;
	sem_post(&m_proceso_ejecutando);


	sem_wait(&esperar_proceso_ejecutando);

	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", contexto->pid, "EXEC","BLOC");
	log_info(logger, "PID: %d - Bloqueado por: SLEEP", contexto->pid);


	sem_wait(&m_proceso_ejecutando);
	actualizar_estado_a_pcb(proceso_ejecutando, "BLOC");
	sem_post(&m_proceso_ejecutando);

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

	while(indice_recurso < tamanio_recursos && !string_equals_ignore_case(recursos[indice_recurso], recurso_a_buscar)){
		indice_recurso++;
	}
	if(indice_recurso == tamanio_recursos){
		return -1;
	}

	return indice_recurso;
}

t_recurso *obtener_recurso_con_nombre(t_list *recursos, char* recurso_a_buscar){

	bool es_recurso(void* recurso_sin_parsear){
		t_recurso *recurso = (t_recurso *) recurso_sin_parsear;
		return string_equals_ignore_case(recurso->nombre_recurso, recurso_a_buscar);
	}


	return list_find(recursos, es_recurso);
}

bool es_proceso_finalizado(void* recurso_sin_parsear){
	t_recurso *recurso = (t_recurso *) recurso_sin_parsear;
	return recurso->instancias_en_posesion == 0;
}

char *obtener_proceso_que_puede_finalizar(t_list *recusos_disponible, t_dictionary *matriz_necesidad, t_dictionary *matriz_asignados){
	t_list *procesos_posibles = list_create();

	void buscar_procesos(char *pid, void *recursos_pendientes_sin_parsear){
		t_list *recursos_pendientes = (t_list *) recursos_pendientes_sin_parsear;
		t_list *recursos_asignados_del_proceso = dictionary_get(matriz_asignados, pid);

		bool puede_sasitfacer_peticion(void* recurso_sin_parsear){
			t_recurso *recurso = (t_recurso *)recurso_sin_parsear;
			t_recurso* recurso_disponible = obtener_recurso_con_nombre(recusos_disponible, recurso->nombre_recurso);
			return recurso->instancias_en_posesion <= recurso_disponible->instancias_en_posesion;
		}

		if(list_all_satisfy(recursos_pendientes, puede_sasitfacer_peticion) && (
				!list_all_satisfy(recursos_asignados_del_proceso, es_proceso_finalizado) ||
				!list_all_satisfy(recursos_pendientes, es_proceso_finalizado)
		)){
			list_add(procesos_posibles, pid);
		}
	}

	dictionary_iterator(matriz_necesidad, buscar_procesos);

	if(list_size(procesos_posibles) == 0){
		return NULL;
	}

	return list_get(procesos_posibles, 0);
}

bool puede_finalizar_algun_proceso(t_list *recusos_disponible, t_dictionary *matriz_necesidad, t_dictionary *matriz_asignados){
	return obtener_proceso_que_puede_finalizar(recusos_disponible, matriz_necesidad, matriz_asignados) != NULL;
}


bool finalizaron_todos_los_procesos(t_dictionary *matriz_necesidad, t_dictionary *matriz_asignados){
	int procesos_finalizados = 0;

	void verificar_finalizo_proceso(char *pid, void *recursos_pendientes_del_proceso_sin_parsear){
		t_list *recursos_pendientes_del_proceso = (t_list *) recursos_pendientes_del_proceso_sin_parsear;
		t_list *recursos_asignados_del_proceso = dictionary_get(matriz_asignados, pid);

		if(list_all_satisfy(recursos_asignados_del_proceso, es_proceso_finalizado) && list_all_satisfy(recursos_pendientes_del_proceso, es_proceso_finalizado)){
			procesos_finalizados ++;
		}
	}


	dictionary_iterator(matriz_necesidad, verificar_finalizo_proceso);

	return procesos_finalizados == dictionary_size(matriz_necesidad);
}

t_recurso *recurso_new(char *nombre_recurso){
	t_recurso *recurso = malloc(sizeof(t_recurso));
	recurso->nombre_recurso = strdup(nombre_recurso);
	recurso->instancias_en_posesion = 0;
	return recurso;
}

void duplicar_diccionario(t_dictionary** duplicado, t_dictionary *a_duplicar){

	t_list* elementos = dictionary_elements(a_duplicar);
	t_list *keys = dictionary_keys(a_duplicar);

	t_list_iterator *iterador_elementos = list_iterator_create(elementos);

	while(list_iterator_has_next(iterador_elementos)){
		t_list *elemento_n = list_iterator_next(iterador_elementos);
		char *key_n = list_get(keys, list_iterator_index(iterador_elementos));

		t_list *elemento_n_dup = list_create();

		void  duplicar_recursos(void* args){
			t_recurso *recurso_n = (t_recurso *) args;
			t_recurso *recurso_n_dup = recurso_new(recurso_n->nombre_recurso);
			recurso_n_dup->instancias_en_posesion = recurso_n->instancias_en_posesion;

			list_add(elemento_n_dup, recurso_n_dup);
		}

		list_iterate(elemento_n, duplicar_recursos);


		dictionary_put(*duplicado, strdup(key_n), elemento_n_dup);
	}

	list_iterator_destroy(iterador_elementos);
}

void loggear_deadlock(void *pid_sin_parsear){
	char *pid = (char *) pid_sin_parsear;
	t_list *recursos_asignados = dictionary_get(matriz_recursos_asignados, pid);
	t_list *recursos_pendientes = dictionary_get(matriz_recursos_pendientes, pid);


	char **lista_recursos = string_array_new();

	int cantidad_recursos = list_size(recursos_asignados);

	for(int i = 0; i<cantidad_recursos ; i++){
		t_recurso *recurso_asignado = (t_recurso *) list_get(recursos_asignados,i);

		if(recurso_asignado->instancias_en_posesion > 0){
			char *coma = malloc(3);
			strcpy(coma, ", ");

			string_array_push(&lista_recursos,  strdup(recurso_asignado->nombre_recurso));
			string_array_push(&lista_recursos, coma);
		}
	}
	//saco la ultima comma
	string_array_pop(lista_recursos);

	char* lista_recursos_asignados = pasar_a_string(lista_recursos);

	string_array_destroy(lista_recursos);

	bool es_recurso_solicitado(void *recurso_sin_parsear){
		t_recurso *recurso = (t_recurso *) recurso_sin_parsear;
		return recurso->instancias_en_posesion > 0;
	}

	t_recurso *recurso_requerido = list_find(recursos_pendientes, es_recurso_solicitado);

	log_info(logger, "Deadlock detectado: %s - Recursos en posesión %s - Recurso requerido: %s", pid, lista_recursos_asignados, recurso_requerido->nombre_recurso);

	free(lista_recursos_asignados);
}

t_list *obtener_procesos_en_deadlock(t_dictionary *matriz_necesidad, t_dictionary *matriz_asignados){
	// lista de pids de procesos en deadlock
	t_list * procesos_en_deadlock = list_create();

	void buscar_procesos_no_finalizados(char *pid, void *recursos_pendientes_del_proceso_sin_parsear){
		t_list *recursos_pendientes_del_proceso = (t_list *) recursos_pendientes_del_proceso_sin_parsear;
		t_list *recursos_asignados_del_proceso = dictionary_get(matriz_asignados, pid);

		bool es_proceso_finalizado(void* recurso_sin_parsear){
			t_recurso *recurso = (t_recurso *) recurso_sin_parsear;
			return recurso->instancias_en_posesion == 0;
		}

		if(!list_all_satisfy(recursos_asignados_del_proceso, es_proceso_finalizado) || !list_all_satisfy(recursos_pendientes_del_proceso, es_proceso_finalizado)){
			list_add(procesos_en_deadlock, pid);
		}
	}


	dictionary_iterator(matriz_necesidad, buscar_procesos_no_finalizados);

	return procesos_en_deadlock;
}


void calcular_recursos_asignados(t_list **recursos_asignados){
	void contar_recursos_asignados(char *pid, void *recursos_sin_parsear){
		t_list *recursos = (t_list *) recursos_sin_parsear;
		void contar_recursos(void *recurso_sin_parsear){
			t_recurso *recurso = (t_recurso *) recurso_sin_parsear;

			t_recurso *recurso_asignado_buscado = obtener_recurso_con_nombre(*recursos_asignados, recurso->nombre_recurso);

			if(recurso_asignado_buscado == NULL){
				t_recurso *recurso_asignado = recurso_new(recurso->nombre_recurso);

				recurso_asignado->instancias_en_posesion += recurso->instancias_en_posesion;

				list_add(*recursos_asignados, recurso_asignado);
			} else {
				recurso_asignado_buscado->instancias_en_posesion += recurso->instancias_en_posesion;
			}
		}

		list_iterate(recursos, contar_recursos);
	}

	dictionary_iterator(matriz_recursos_asignados, contar_recursos_asignados);
}

void calcular_recursos_disponible(t_list **recursos_disponible, t_list *recursos_asignados){
	t_list_iterator *iterador_recursos_asignados = list_iterator_create(recursos_asignados);

	while(list_iterator_has_next(iterador_recursos_asignados)){
		t_recurso* recurso_asignado = list_iterator_next(iterador_recursos_asignados);

		t_recurso *recurso_disponible = recurso_new(recurso_asignado->nombre_recurso);

		recurso_disponible->instancias_en_posesion = recursos_totales[list_iterator_index(iterador_recursos_asignados)] - recurso_asignado->instancias_en_posesion;

		list_add(*recursos_disponible, recurso_disponible);
	}
}


void deteccion_de_deadlock(){
	// matriz de asignados
	// vector de recursos totales

	log_info(logger, "ANÁLISIS DE DETECCIÓN DE DEADLOCK");

	t_dictionary *matriz_asignados_cop = dictionary_create();
	t_dictionary *matriz_necesidad_cop = dictionary_create();
	t_list *recursos_disponible_cop = list_create();
	t_list *recursos_asignados = list_create();

	duplicar_diccionario(&matriz_asignados_cop, matriz_recursos_asignados);
	duplicar_diccionario(&matriz_necesidad_cop, matriz_recursos_pendientes);
	calcular_recursos_asignados(&recursos_asignados);
	calcular_recursos_disponible(&recursos_disponible_cop, recursos_asignados);

	//inicio simulacion
	while(puede_finalizar_algun_proceso(recursos_disponible_cop, matriz_necesidad_cop, matriz_asignados_cop)){
		char *pid_proceso_puede_finalizar = obtener_proceso_que_puede_finalizar(recursos_disponible_cop, matriz_necesidad_cop, matriz_asignados_cop);

		t_list* recursos_asignados_finalizar = dictionary_get(matriz_asignados_cop, pid_proceso_puede_finalizar);
		t_list* recursos_pendientes_finalizar = dictionary_get(matriz_necesidad_cop, pid_proceso_puede_finalizar);

		t_list_iterator *iterador_recursos_asignados_finalizar = list_iterator_create(recursos_asignados_finalizar);

		while(list_iterator_has_next(iterador_recursos_asignados_finalizar)){
			t_recurso* recurso_asignado_finalizar = list_iterator_next(iterador_recursos_asignados_finalizar);
			t_recurso* recurso_disponible_a_actualizar = list_get(recursos_disponible_cop, list_iterator_index(iterador_recursos_asignados_finalizar));
			t_recurso* recurso_pendiente_finalizar = list_get(recursos_pendientes_finalizar, list_iterator_index(iterador_recursos_asignados_finalizar));

			recurso_disponible_a_actualizar->instancias_en_posesion += recurso_asignado_finalizar->instancias_en_posesion;
			recurso_asignado_finalizar->instancias_en_posesion = 0;
			recurso_pendiente_finalizar->instancias_en_posesion = 0;
		}
	}


	if(!finalizaron_todos_los_procesos(matriz_necesidad_cop,matriz_asignados_cop)){
		//hay deadlock

		t_list* procesos_en_deadlock = obtener_procesos_en_deadlock(matriz_necesidad_cop,matriz_asignados_cop);


		list_iterate(procesos_en_deadlock, loggear_deadlock);

		list_destroy(procesos_en_deadlock);
	}

	destroy_matriz(matriz_asignados_cop);
	destroy_matriz(matriz_necesidad_cop);
	destroy_lista_de_recursos(recursos_disponible_cop);
	destroy_lista_de_recursos(recursos_asignados);
}

t_list *obtener_recursos_en_base_a_pid_en_matriz(t_dictionary **matriz, char *pid, int cantidad_de_recursos){
	t_list *recursos_a_devolver =  dictionary_get(*matriz, pid);

	if(recursos_a_devolver == NULL){
		recursos_a_devolver = list_create();

		for(int i = 0; i<cantidad_de_recursos; i++){
			t_recurso * recurso_n = recurso_new(recursos[i]);

			list_add(recursos_a_devolver, recurso_n);
		}

		dictionary_put(*matriz,pid, recursos_a_devolver);
	}

	return recursos_a_devolver;
}

void incrementar_recurso_en_matriz(t_dictionary **matriz, char *nombre_recurso, char *pid, int cantidad_de_recursos){
	t_list *recursos = obtener_recursos_en_base_a_pid_en_matriz(matriz, pid, cantidad_de_recursos);

	t_recurso *recurso_a_incrementar = obtener_recurso_con_nombre(recursos, nombre_recurso);
	recurso_a_incrementar->instancias_en_posesion ++;
}

void decrementar_recurso_en_matriz(t_dictionary **matriz, char *nombre_recurso, char *pid, int cantidad_de_recursos){
	t_list *recursos = obtener_recursos_en_base_a_pid_en_matriz(matriz, pid, cantidad_de_recursos);

	t_recurso *recurso_a_decrementar = obtener_recurso_con_nombre(recursos, nombre_recurso);
	recurso_a_decrementar->instancias_en_posesion --;
}

void bloquear_proceso_por_recurso(t_pcb* proceso_a_bloquear, char* nombre_recurso, int cantidad_de_recursos){

	char *pid = string_itoa(proceso_a_bloquear->PID);

	incrementar_recurso_en_matriz(&matriz_recursos_pendientes, nombre_recurso, pid, cantidad_de_recursos);

	deteccion_de_deadlock();

	log_info(logger, "PID: %s - Estado Anterior: %s - Estado Actual: %s", pid, "EXEC","BLOC");
	log_info(logger, "PID: %s - Bloqueado por: %s", pid, nombre_recurso);

	actualizar_estado_a_pcb(proceso_a_bloquear, "BLOC");

	t_queue* cola_bloqueados = dictionary_get(recurso_bloqueado,nombre_recurso);

	queue_push(cola_bloqueados,proceso_a_bloquear);

}

void finalizar_por_invalid_resource_proceso_ejecutando(t_contexto_ejec** contexto){
	sem_wait(&m_proceso_ejecutando);
	actualizar_estado_a_pcb(proceso_ejecutando, "EXIT");

	t_paquete* paquete = crear_paquete(FINALIZAR_PROCESO_MEMORIA);
	agregar_a_paquete_sin_agregar_tamanio(paquete, &(proceso_ejecutando->PID), sizeof(int));
	enviar_paquete(paquete, socket_memoria);

	eliminar_paquete(paquete);

	log_info(logger, "Finaliza el proceso %d - Motivo: INVALID_RESOURCE", proceso_ejecutando->PID);
	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_ejecutando->PID, "EXEC","EXIT");
	sem_post(&m_proceso_ejecutando);

	contexto_ejecucion_destroy(*contexto);

	sem_wait(&m_proceso_ejecutando);
	pcb_args_destroy(proceso_ejecutando);
	sem_post(&m_proceso_ejecutando);
	poner_a_ejecutar_otro_proceso();
}

void logear_instancias(char* pid, char* nombre_recurso, int *recurso_disponible, int cantidad_de_recursos){
	char* recursos_disponibles_string = listar_recursos_disponibles(recurso_disponible, cantidad_de_recursos);
	log_info(logger, "PID: %s - Wait: %s - Instancias: [%s]", pid, nombre_recurso, recursos_disponibles_string);

	free(recursos_disponibles_string);
}

void apropiar_recursos(int socket_cliente, char** recursos, int* recurso_disponible, int cantidad_de_recursos){
	t_contexto_ejec* contexto = (t_contexto_ejec*) recibir_contexto_de_ejecucion(socket_cliente);

	t_instruccion* instruccion = contexto->instruccion;
	char *nombre_recurso = instruccion->parametros[0];
	char *pid = string_itoa(contexto->pid);

	log_info(logger, "Inicio Wait al recurso %s",nombre_recurso);

	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando->program_counter = contexto->program_counter;
	sem_post(&m_proceso_ejecutando);

	int indice_recurso = obtener_indice_recurso(recursos, nombre_recurso);


	// si no existe el recurso finaliza
	if(indice_recurso == -1){
		finalizar_por_invalid_resource_proceso_ejecutando(&contexto);
		return;
	}

	recurso_disponible[indice_recurso] --;

	if(recurso_disponible[indice_recurso] < 0){

		//lamo a esta funcion para inicializar la matriz de recursos asignados para la deteccion de deadlock
		obtener_recursos_en_base_a_pid_en_matriz(&matriz_recursos_asignados, pid, cantidad_de_recursos);

		sem_wait(&m_proceso_ejecutando);
		bloquear_proceso_por_recurso(proceso_ejecutando, recursos[indice_recurso], cantidad_de_recursos);
		sem_post(&m_proceso_ejecutando);

		logear_instancias(pid, recursos[indice_recurso], recurso_disponible, cantidad_de_recursos);

		poner_a_ejecutar_otro_proceso();

		contexto_ejecucion_destroy(contexto);

		return;
	}

	incrementar_recurso_en_matriz(&matriz_recursos_asignados, nombre_recurso, pid, cantidad_de_recursos);

	logear_instancias(pid, recursos[indice_recurso], recurso_disponible, cantidad_de_recursos);

	// continua ejecutandose el mismo proceso
	enviar_contexto_de_ejecucion_a(contexto, PETICION_CPU, socket_cliente);

	//destruyo el contexto de ejecucion
	contexto_ejecucion_destroy(contexto);
}

void desalojar_recursos(int socket_cliente,char** recursos, int* recurso_disponible, int cantidad_de_recursos){
	t_contexto_ejec* contexto = (t_contexto_ejec*) recibir_contexto_de_ejecucion(socket_cliente);

	t_instruccion* instruccion = contexto->instruccion;
	char *nombre_recurso = instruccion->parametros[0];
	char *pid = string_itoa(contexto->pid);

	log_info(logger, "Inicio Signal al recurso %s",nombre_recurso);

	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando->program_counter = contexto->program_counter;
	sem_post(&m_proceso_ejecutando);

	int indice_recurso = obtener_indice_recurso(recursos, nombre_recurso);

	// si no existe el recurso finaliza
	if(indice_recurso == -1){
		finalizar_por_invalid_resource_proceso_ejecutando(&contexto);

		return;
	}

	recurso_disponible[indice_recurso] ++;

	t_list *recursos_del_proceso = obtener_recursos_en_base_a_pid_en_matriz(&matriz_recursos_asignados, pid, cantidad_de_recursos);

	t_recurso *recurso_buscado = obtener_recurso_con_nombre(recursos_del_proceso, nombre_recurso);

	if(recurso_buscado->instancias_en_posesion > 0){ // si este proceso solicito el recurso
		decrementar_recurso_en_matriz(&matriz_recursos_asignados, nombre_recurso, pid, cantidad_de_recursos);
	} else { //este proceso no solicito una instancia del recurso
		finalizar_por_invalid_resource_proceso_ejecutando(&contexto);
		return;
	}

	t_queue* cola_bloqueados= (t_queue*) dictionary_get(recurso_bloqueado,recursos[indice_recurso]);

	int cantidad_procesos_bloqueados = queue_size(cola_bloqueados);

	if(cantidad_procesos_bloqueados > 0){
		t_pcb* proceso_desbloqueado = queue_pop(cola_bloqueados);
		char * pid_desbloqueado = string_itoa(proceso_desbloqueado->PID);

		log_info(logger, "PID: %s - Estado Anterior: %s - Estado Actual: %s", pid_desbloqueado, "BLOC","READY");

		//actualizo los recursos disponibles para que no se le actualize a otro proceso
		recurso_disponible[indice_recurso] --;

		decrementar_recurso_en_matriz(&matriz_recursos_pendientes, nombre_recurso, pid_desbloqueado, cantidad_de_recursos);
		incrementar_recurso_en_matriz(&matriz_recursos_asignados, nombre_recurso, pid_desbloqueado, cantidad_de_recursos);

		pasar_a_ready(proceso_desbloqueado);
	}

	logear_instancias(pid, recursos[indice_recurso], recurso_disponible, cantidad_de_recursos);

	//continua ejecutandose el mismo proceso
	enviar_contexto_de_ejecucion_a(contexto, PETICION_CPU, socket_cliente);

	//destruyo el contexto de ejecucion
	contexto_ejecucion_destroy(contexto);
}


void finalinzar_proceso(int socket_cliente){
	t_contexto_ejec* contexto = recibir_contexto_de_ejecucion(socket_cliente);

	sem_wait(&m_proceso_ejecutando);
	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_ejecutando->PID, "EXEC","EXIT");

	actualizar_estado_a_pcb(proceso_ejecutando, "EXIT");


	t_paquete *paquete = crear_paquete(FINALIZAR_PROCESO_MEMORIA);
	agregar_a_paquete_sin_agregar_tamanio(paquete,&(proceso_ejecutando->PID),sizeof(int));
	enviar_paquete(paquete,socket_memoria);
	eliminar_paquete(paquete);


	log_info(logger, "Finaliza el proceso %d - Motivo: SUCCESS", proceso_ejecutando->PID);

	sem_post(&m_proceso_ejecutando);

	contexto_ejecucion_destroy(contexto);

	sem_wait(&m_proceso_ejecutando);
	pcb_args_destroy(proceso_ejecutando);
	free(proceso_ejecutando);
	sem_post(&m_proceso_ejecutando);

	poner_a_ejecutar_otro_proceso();
}




void destroy_lista_de_recursos(t_list* lista_recursos){
	void eliminar_recurso(void *arg){
		t_recurso *recurso_n = (t_recurso *)arg;

		free(recurso_n->nombre_recurso);
		free(recurso_n);
	}

	list_destroy_and_destroy_elements(lista_recursos, eliminar_recurso);
}

void destroy_proceso_en_matriz(t_dictionary *matriz, char *pid){
	void eliminar_recursos(void *args){
		t_list *recusos_a_borrar = (t_list *) args;

		destroy_lista_de_recursos(recusos_a_borrar);
	}

	dictionary_remove_and_destroy(matriz, pid, eliminar_recursos);
}
void destroy_matriz(t_dictionary *matriz){

	t_list *pids_procesos = dictionary_keys(matriz);

	void destruir_proceso(void *args){
		char *pid = (char *)args;

		destroy_proceso_en_matriz(matriz, pid);
	}

	list_iterate(pids_procesos, destruir_proceso);

	list_destroy(pids_procesos);
	dictionary_destroy(matriz);
}

t_list *duplicar_lista_recursos(t_list *a_duplicar){
	t_list *recursos_del_proceso_dup = list_create();

	void duplicar_recursos(void *arg){
		t_recurso *recurso_n = (t_recurso *)arg;

		t_recurso *recuso_n_dup = recurso_new(recurso_n->nombre_recurso);
		recuso_n_dup->instancias_en_posesion = recurso_n->instancias_en_posesion;

		list_add(recursos_del_proceso_dup, recuso_n_dup);
	}

	list_iterate(a_duplicar, duplicar_recursos);

	return recursos_del_proceso_dup;
}
