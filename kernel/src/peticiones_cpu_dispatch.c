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

	bool tiene_instancias(t_recurso *recurso_n){
		return recurso_n->instancias_en_posesion > 0 ;
	}

	char *lista_recursos_asignados =listar_recursos_lista_recursos_por_condicion(recursos_asignados, tiene_instancias);

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

	sem_wait(&m_cola_exit);
	queue_push(cola_exit, string_itoa(proceso_ejecutando->PID));
	sem_post(&m_cola_exit);
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
		log_info(logger, "Finaliza el proceso %d por recurso no tomado", contexto->pid);
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

	sem_wait(&m_cola_exit);
	queue_push(cola_exit, string_itoa(proceso_ejecutando->PID));
	sem_post(&m_cola_exit);
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

void bloquear_por_espera_a_fs(t_pcb* proceso_a_bloquear, char*nombre_archivo){
	//Si no existe el archivo en el diccionario, lo creo.
	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_a_bloquear->PID, "EXEC","BLOC");
	log_info(logger, "PID: %d - Bloqueado por: %s", proceso_a_bloquear->PID, nombre_archivo);

	if(!dictionary_has_key(colas_de_procesos_bloqueados_para_cada_archivo, nombre_archivo)){
		t_queue* cola_bloqueados = queue_create();
		queue_push(cola_bloqueados, proceso_a_bloquear);

		dictionary_put(colas_de_procesos_bloqueados_para_cada_archivo, nombre_archivo, cola_bloqueados);

		// Si existe agrego el elemento a la cola
	}else{
		//Creo una cola, le asigno la cola del diccionario y la remuevo
		t_queue* cola_bloqueados = dictionary_get(colas_de_procesos_bloqueados_para_cada_archivo, nombre_archivo);
		//cargo el proceso a bloquear en la cola

		queue_push(cola_bloqueados, proceso_a_bloquear);
	}

}

void desbloquear_por_espera_a_fs(int pid, char*nombre_archivo){
	if(dictionary_has_key(colas_de_procesos_bloqueados_para_cada_archivo, nombre_archivo)){
		t_queue* cola_bloqueados = dictionary_get(colas_de_procesos_bloqueados_para_cada_archivo, nombre_archivo);

		bool pid_encontrado = false;
		t_pcb* pcb_a_desbloquear;
		//esto lo hago asi para no tener que cambiar en donde se usa esta cola,
		// pero lo mas conveniente seria usar otro diccionario dentro del diccionario de procesos_bloqueados
		// donde se mapea el pcb con el pid, para encontrarlo mas rápido
		while(!pid_encontrado){
			pcb_a_desbloquear = queue_pop(cola_bloqueados);

			if(pcb_a_desbloquear->PID == pid){
				pid_encontrado = true;
			}else {
				queue_push(cola_bloqueados, pcb_a_desbloquear);
			}
		}
		log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pid, "BLOC","READY");

		pasar_a_ready(pcb_a_desbloquear);
	}
}

void enviar_instruccion(t_instruccion* instruccion, int socket_a_enviar, int opcode){
	t_paquete* paquete = crear_paquete(opcode);
	agregar_a_paquete(paquete, instruccion->opcode,sizeof(instruccion->opcode_lenght));
	agregar_a_paquete(paquete, instruccion->parametros[0],sizeof(instruccion->parametro1_lenght));
	agregar_a_paquete(paquete, instruccion->parametros[1],sizeof(instruccion->parametro2_lenght));
	agregar_a_paquete(paquete, instruccion->parametros[2],sizeof(instruccion->parametro3_lenght));
	enviar_paquete(paquete, socket_a_enviar);
	eliminar_paquete(paquete);
}

void enviar_a_fs_truncar_archivo(int socket_cpu, int socket_filesystem)
{
	t_contexto_ejec* contexto = recibir_contexto_de_ejecucion(socket_cpu);

	char* nombre_archivo = strdup(contexto->instruccion->parametros[0]);
	char* tamanio_archivo = strdup(contexto->instruccion->parametros[1]);

	log_info(logger,"PID: %d - Archivo: %s - Tamaño: %s",contexto->pid, nombre_archivo, tamanio_archivo);

	//ya recibimos la instruccion con los parametros, ahora hay que mandarle a filesystem la orden
	enviar_instruccion(contexto->instruccion, socket_filesystem, TRUNCAR_ARCHIVO);

	//bloquear mientras espera a FS y desbloquear el proceso cuando termina FS.

	sem_wait(&m_proceso_ejecutando);
	bloquear_por_espera_a_fs(proceso_ejecutando, nombre_archivo);
	sem_post(&m_proceso_ejecutando);
	poner_a_ejecutar_otro_proceso();

	int opcode = recibir_operacion(socket_filesystem);

	if(opcode != TRUNCAR_ARCHIVO){
		log_error(logger, "No se pudo truncar el archivo, hubo un error");
		return ;
	}

	char *mensaje = recibir_mensaje(socket_filesystem);

	log_info(logger, "se reicibio un %s de FS, archivo truncado correctamente ", mensaje);

	desbloquear_por_espera_a_fs(contexto->pid, nombre_archivo);

	contexto_ejecucion_destroy(contexto);
	return;
}


bool existe_lock_escritura_para(char* nombre_archivo){
	t_tabla_global_de_archivos_abiertos* tabla = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);
	return  tabla->lock_de_archivo != NULL &&  tabla->lock_de_archivo->write_lock_count > 0;
}

bool existe_lock_lectura_para(char* nombre_archivo){
	t_tabla_global_de_archivos_abiertos* tabla = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);
	return  tabla->lock_de_archivo != NULL &&  tabla->lock_de_archivo->read_lock_count > 0;
}

void agregar_a_lock_escritura_para_archivo(char* nombre_archivo, t_pcb* proceso){
	t_tabla_global_de_archivos_abiertos* tabla = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);
	if(tabla->lock_de_archivo != NULL){
		tabla->lock_de_archivo->write_lock_count ++;
		queue_push(tabla->lock_de_archivo->cola_locks, proceso);
	} else {
		log_error(logger, "el lock del archivo en tabla global de archivos abiertos no existe, esto es imposible aca");
	}
}

void agregar_a_lock_lectura_para_archivo(char* nombre_archivo, t_pcb* proceso){
	t_tabla_global_de_archivos_abiertos* tabla = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);
	if(tabla->lock_de_archivo != NULL){
		tabla->lock_de_archivo->read_lock_count ++;
		queue_push(tabla->lock_de_archivo->cola_locks, proceso);
	} else {
		log_error(logger, "el lock del archivo en tabla global de archivos abiertos no existe, esto es imposible aca");
	}
}

void agregar_como_participante_a_lock_lectura_para_archivo(char*nombre_archivo, t_pcb* proceso){
	t_tabla_global_de_archivos_abiertos* tabla = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);
	if(tabla->lock_de_archivo != NULL){
		tabla->lock_de_archivo->read_lock_count ++;
		list_add(tabla->lock_de_archivo->lista_locks_read, proceso);
	} else {
		log_error(logger, "el lock del archivo en tabla global de archivos abiertos no existe, esto es imposible aca");
	}
}

void crear_lock_escritura_para(char* nombre_archivo, t_pcb* proceso){
	t_tabla_global_de_archivos_abiertos* tabla = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);
	if(tabla->lock_de_archivo != NULL){
		tabla->lock_de_archivo->write_lock_count ++;
		tabla->lock_de_archivo->proceso_write_lock = proceso;
	}else {
		log_error(logger, "el lock del archivo en la tabla global de archivos abiertos no existe, esto es imposible aca");
	}

}

void actualizar_lock_escritura_para_archivo(char *nombre_archivo, t_pcb* proceso){
	t_tabla_global_de_archivos_abiertos* tabla = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);
	if(tabla->lock_de_archivo != NULL ){
		t_file_lock *lock = malloc(sizeof(t_file_lock));
		lock->cola_locks = queue_create();
		lock->lista_locks_read = list_create();
		lock->read_lock_count = 0;
		lock->write_lock_count = 1;
		lock->proceso_write_lock = proceso;

		tabla->lock_de_archivo = lock;
	}else {
		log_error(logger, "el tabla global de archivos abiertos no existe, esto es imposible aca");
	}
}

void manejar_lock_escritura(char *nombre_archivo, t_contexto_ejec* contexto, int socket_cliente){

	if(existe_lock_escritura_para(nombre_archivo)){

		deteccion_de_deadlock();

		sem_wait(&m_proceso_ejecutando);
		agregar_a_lock_escritura_para_archivo(nombre_archivo, proceso_ejecutando);
		bloquear_por_espera_a_fs(proceso_ejecutando, nombre_archivo);//o mantenerlo bloqueado
		sem_post(&m_proceso_ejecutando);

		//la cantidad de recusos no se usa en este caso, asi que no importa que este en 0
		incrementar_recurso_en_matriz(&matriz_recursos_pendientes, nombre_archivo, string_itoa(contexto->pid), 0);

		poner_a_ejecutar_otro_proceso();
	} else {
		sem_wait(&m_proceso_ejecutando);
		crear_lock_escritura_para(nombre_archivo, proceso_ejecutando);
		sem_post(&m_proceso_ejecutando);

		decrementar_recurso_en_matriz(&matriz_recursos_pendientes, nombre_archivo, string_itoa(contexto->pid), 0);
		incrementar_recurso_en_matriz(&matriz_recursos_asignados, nombre_archivo, string_itoa(contexto->pid), 0);

		enviar_contexto_de_ejecucion_a(contexto, PETICION_CPU, socket_cliente);
	}
}

void manejar_lock_lectura(char *nombre_archivo, t_contexto_ejec* contexto, int socket_cliente){

	if(existe_lock_escritura_para(nombre_archivo)){

		deteccion_de_deadlock();

		sem_wait(&m_proceso_ejecutando);
		agregar_a_lock_lectura_para_archivo(nombre_archivo, proceso_ejecutando);
		bloquear_por_espera_a_fs(proceso_ejecutando, nombre_archivo);//o mantenerlo bloqueado
		sem_post(&m_proceso_ejecutando);

		//la cantidad de recusos no se usa en este caso, asi que no importa que este en 0
		incrementar_recurso_en_matriz(&matriz_recursos_pendientes, nombre_archivo, string_itoa(contexto->pid), 0);

		poner_a_ejecutar_otro_proceso();
	}else {
		sem_wait(&m_proceso_ejecutando);
		agregar_como_participante_a_lock_lectura_para_archivo(nombre_archivo, proceso_ejecutando);
		sem_post(&m_proceso_ejecutando);

		decrementar_recurso_en_matriz(&matriz_recursos_pendientes, nombre_archivo, string_itoa(contexto->pid), 0);
		incrementar_recurso_en_matriz(&matriz_recursos_asignados, nombre_archivo, string_itoa(contexto->pid), 0);

		enviar_contexto_de_ejecucion_a(contexto, PETICION_CPU, socket_cliente);
	}

}

void crear_entrada_tabla_global_archivos_abiertos(char *nombre_archivo){
	t_tabla_global_de_archivos_abiertos *tabla_archivo_abierto_global = malloc(sizeof(t_tabla_global_de_archivos_abiertos));

	tabla_archivo_abierto_global->file = strdup(nombre_archivo);
	tabla_archivo_abierto_global->open = 1;

	t_file_lock* lock_archivo = malloc(sizeof(t_file_lock));
	lock_archivo->lista_locks_read = list_create();
	lock_archivo->cola_locks = queue_create();
	lock_archivo->read_lock_count = 0;
	lock_archivo->write_lock_count = 0;
	lock_archivo->proceso_write_lock = NULL;

	tabla_archivo_abierto_global->lock_de_archivo = lock_archivo ;

	dictionary_put(tabla_global_de_archivos_abiertos, nombre_archivo, tabla_archivo_abierto_global);
}

void crear_entrada_lista_archivo_abierto_por_proceso(char* nombre_archivo, t_pcb* proceso, char* modo_apertura){

	t_list* lista_de_archivos_abiertos_proceso = proceso->tabla_archivos_abiertos_del_proceso;

	t_tabla_de_archivos_por_proceso* tabla_archivo_abierto = malloc(sizeof(t_tabla_de_archivos_por_proceso));
	tabla_archivo_abierto->nombre_archivo = nombre_archivo;
	tabla_archivo_abierto->puntero_posicion =0;
	tabla_archivo_abierto->modo_apertura = modo_apertura;

	list_add(lista_de_archivos_abiertos_proceso, tabla_archivo_abierto);
}

void agregar_recurso_a_matriz(char *nombre_archivo, int pid, t_dictionary** matriz){
	t_list *recursos_a_devolver =  dictionary_get(*matriz, string_itoa(pid));

	if(recursos_a_devolver == NULL){
		recursos_a_devolver = list_create();

		for(int i = 0; i<cant_recursos; i++){
			t_recurso * recurso_n = recurso_new(recursos[i]);

			list_add(recursos_a_devolver, recurso_n);
		}

		t_recurso * recurso_n = recurso_new(nombre_archivo);
		list_add(recursos_a_devolver, recurso_n);

		dictionary_put(*matriz,string_itoa(pid), recursos_a_devolver);
	} else {

		bool es_el_archivo(void*args){
			t_recurso* recurso_n = (t_recurso*)args;

			return strcmp(nombre_archivo, recurso_n->nombre_recurso) == 0;
		}

		if(!list_any_satisfy(recursos_a_devolver, es_el_archivo)){
			t_recurso * recurso_n = recurso_new(nombre_archivo);

			list_add(recursos_a_devolver, recurso_n);
		}
	}
}
void agregar_recurso_a_matrices(char *nombre_archivo, int pid){
	agregar_recurso_a_matriz(nombre_archivo, pid, &matriz_recursos_pendientes);
	agregar_recurso_a_matriz(nombre_archivo, pid, &matriz_recursos_asignados);
}

void enviar_a_fs_crear_o_abrir_archivo (int socket_cpu, int socket_filesystem)
{
	t_contexto_ejec* contexto = recibir_contexto_de_ejecucion(socket_cpu);

	char* nombre_archivo = strdup(contexto->instruccion->parametros[0]);
	agregar_recurso_a_matrices(nombre_archivo, contexto->pid);

	log_info(logger,"PID %d - ABRIR ARCHIVO: %s",contexto->pid, nombre_archivo);

	enviar_instruccion(contexto->instruccion, socket_filesystem, ABRIR_ARCHIVO);

	//esperamos una respuesata del fs:
	//	SI EXISTE, MANDARA EL TAM DEL ARCHIVO;
	//	SI NO EXISTE, MANDARA UN -1 Y MANDAREMOS ORDEN DE CREAR EL ARCHIVO

	int opcode = recibir_operacion(socket_filesystem);
	if(opcode == MENSAJE){
		//si el archivo no existe
		if(atoi(recibir_mensaje(socket_filesystem))==-1){
			//mandamos la orden
			enviar_instruccion(contexto->instruccion, socket_filesystem, CREAR_ARCHIVO);

			//epero la respuesta de la creacion del archivo
			int opcode = recibir_operacion(socket_filesystem);

			if(opcode != CREAR_ARCHIVO){
				log_error(logger, "No se pudo crear el archivo, hubo un error");
				return;
			}

			char *mensaje = recibir_mensaje(socket_filesystem);
			log_info(logger, "Se recibio %s de Filesystem, arhivo creado exitosamente", mensaje);
			free(mensaje);

			crear_entrada_tabla_global_archivos_abiertos(nombre_archivo);
		} else {
			log_error(logger, "Hubo un error al recibir de FS si el archivo %s existe o no", nombre_archivo);
		}
	// si existe el archivo
	} else if(opcode == ABRIR_ARCHIVO){
		char* mensaje = recibir_mensaje(socket_filesystem);
		log_info(logger, "se reicibo un %s de FS, archivo abierto correctamente", mensaje);
		free(mensaje);

		t_tabla_global_de_archivos_abiertos *tabla_global = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);
		tabla_global->open ++;
	}

	char *modo_apertura =  strdup(contexto->instruccion->parametros[1]);
	sem_wait(&m_proceso_ejecutando);
	crear_entrada_lista_archivo_abierto_por_proceso(nombre_archivo, proceso_ejecutando, modo_apertura);
	sem_post(&m_proceso_ejecutando);
	//maneja los locks

	//MODO ESCRITURA
	if (string_equals_ignore_case(modo_apertura, "W")) {
		manejar_lock_escritura(nombre_archivo, contexto, socket_cpu);
	//MODO LECTURA
	}else if(string_equals_ignore_case(modo_apertura, "R")){
		manejar_lock_lectura(nombre_archivo, contexto, socket_cpu);
	}

	//dependiendo del lock, va a seguir ejecutando el proceso o se va a bloquear y llama a otro proceso de ready, etc

	contexto_ejecucion_destroy(contexto);
	return;
}


void* hilo_que_maneja_pf(void* args){

	struct t_arg_page_fault {
		int numero_pagina;
		int pid;
	}*args_page_fault = args;


	int numero_pagina = args_page_fault->numero_pagina;
	int pid = args_page_fault->pid;

	sem_wait(&m_proceso_ejecutando);
	t_pcb* proceso_a_bloquear = proceso_ejecutando;
	sem_post(&m_proceso_ejecutando);

	char* pid_del_bloqueado=string_itoa(proceso_a_bloquear->PID);
	dictionary_put(colas_de_procesos_bloqueados_por_pf,pid_del_bloqueado,proceso_a_bloquear);

	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pid, "EXEC","BLOC");
	log_info(logger, "PID: %d - Bloqueado por: PAGE_FAULT", proceso_a_bloquear->PID);
	log_info(logger, "Page Fault PID: %d - Pagina: %d  ", pid,numero_pagina);

	poner_a_ejecutar_otro_proceso();
	t_paquete* paquete= crear_paquete(PAGE_FAULT);

	agregar_a_paquete_sin_agregar_tamanio(paquete,&numero_pagina,sizeof(int));
	agregar_a_paquete_sin_agregar_tamanio(paquete,&pid,sizeof(int));
	enviar_paquete(paquete,socket_memoria);

	eliminar_paquete(paquete);

	 int cod_op = recibir_operacion(socket_memoria);


	 if(cod_op!=PAGE_FAULT){
		 log_error(logger, "No se pudo recibir bloques asignados. Terminando servidor");
		 return NULL;
	 }
	 //aca deberia llegar un ok
	 char* mensaje = recibir_mensaje(socket_memoria);


	 if(strcmp(mensaje,"OK")==0){
		 t_pcb* proceso_a_ready = dictionary_get(colas_de_procesos_bloqueados_por_pf,pid_del_bloqueado);
		 actualizar_estado_a_pcb(proceso_a_ready, "READY");
		 pasar_a_ready(proceso_a_ready);
	 }

	 free(args_page_fault);
   return NULL;
}

void manejar_page_fault(int socket_cliente){

	int size;
	void *buffer = recibir_buffer(&size, socket_cliente);
	int numero_pagina;
	int desplazamiento = 0;

	memcpy(&numero_pagina, buffer+desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);

	t_contexto_ejec* contexto = deserializar_contexto_de_ejecucion(buffer, size, &desplazamiento);

	pthread_t hilo_pf;

	struct t_arg_page_fault {
		int numero_pagina;
		int pid;
	}*args_page_fault = malloc(sizeof(struct t_arg_page_fault));
	args_page_fault->numero_pagina=numero_pagina;
	args_page_fault->pid=contexto->pid;

	pthread_create(&hilo_pf,NULL,hilo_que_maneja_pf,(void*)args_page_fault);
	pthread_detach(hilo_pf);

	free(buffer);
}

