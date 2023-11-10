#include "consola.h"
sem_t despertar_planificacion_largo_plazo;
t_pcb *proceso_ejecutando;
t_dictionary *colas_de_procesos_bloqueados_para_cada_archivo;
bool planificacion_detenida = false;
int grado_max_multiprogramacion;

void destroy_commando(t_instruccion *comando){

	free(comando->opcode);
	free(comando->parametros[0]);
	free(comando->parametros[1]);
	free(comando->parametros[2]);
	free(comando);
}

t_instruccion* armar_comando(char *cadena) {

	t_instruccion *comando = malloc(sizeof(t_instruccion));


	char *token = strtok(strdup(cadena), " "); // obtiene el primer elemento en token
	string_to_upper(token); //paso la cadena a mayuscula
	comando->opcode = token;


	token = strtok(NULL, " "); // avanza al segundo elemento
	int i = 0; // Variable local utilizada para cargar el array de parametros

	comando->parametros[0] = NULL;
	comando->parametros[1] = NULL;
	comando->parametros[2] = NULL;

	while (token != NULL) { //Ingresa si el parametro no es NULL

		comando->parametros[i] = strdup(token); //Carga el parametro en el array de la struct
		token = strtok(NULL, " "); // obtiene el siguiente elemento
		i++; //Avanza en el array
	}
	return comando;
}

void parametros_lenght(t_instruccion *ptr_inst) {

	ptr_inst->opcode_lenght = strlen(ptr_inst->opcode) + 1;

	if (ptr_inst->parametros[0] != NULL) {
		ptr_inst->parametro1_lenght = strlen(ptr_inst->parametros[0]) + 1;
	} else {
		ptr_inst->parametro1_lenght = 0;
	}
	if (ptr_inst->parametros[1] != NULL) {
		ptr_inst->parametro2_lenght = strlen(ptr_inst->parametros[1]) + 1;
	} else {
		ptr_inst->parametro2_lenght = 0;
	}
	if (ptr_inst->parametros[2] != NULL) {
		ptr_inst->parametro3_lenght = strlen(ptr_inst->parametros[2]) + 1;
	} else {
		ptr_inst->parametro3_lenght = 0;
	}
}

void iniciar_proceso(t_instruccion *comando) {
	//Armar PCB, colocarla en estado new y enviar el path a memoria
	t_pcb *pcb_proceso = malloc(sizeof(t_pcb));

	pcb_proceso->PID = rand() % 10000;
	pcb_proceso->program_counter = 1;
	pcb_proceso->proceso_estado = malloc(8);
	strcpy(pcb_proceso->proceso_estado, "NEW");
	pcb_proceso->tiempo_llegada_ready = 0;
	pcb_proceso->prioridad = atoi(comando->parametros[2]);
	pcb_proceso->tabla_archivos_abiertos_del_proceso = NULL;

	pcb_proceso->comando = malloc(sizeof(t_instruccion));
	pcb_proceso->comando->parametros[0] = strdup(comando->parametros[0]);
	pcb_proceso->comando->parametro1_lenght = comando->parametro1_lenght;
	pcb_proceso->comando->parametro2_lenght = 0;
	pcb_proceso->comando->parametro3_lenght = 0;

	//este malloc para evitar el segmentation fault en el envio del contexto de ejecución a cpu
	pcb_proceso->registros_CPU = malloc(sizeof(registros_CPU));


	agregar_cola_new(&pcb_proceso);

	sem_post(&despertar_planificacion_largo_plazo);

	destroy_commando(comando);
}

t_list *obtener_procesos_bloqueados_sin_repetir_de(t_list* lista_de_colas){
	t_list *lista_de_procesos_sin_repetir = list_create();

	void agregar_pcb_de_la_cola(void* arg){
		t_queue *cola_n = (t_queue *)arg;

		void buscar_proceso(void *args){
			t_pcb *proceso_n = (t_pcb *) args;

			if(list_size(lista_de_procesos_sin_repetir) == 0){
				list_add(lista_de_procesos_sin_repetir, proceso_n);
			} else {
				//agregarlo si no esta repetido
				bool es_proceso_n(void*args_proceso){
					t_pcb *proceso_guardado_n = (t_pcb *) args_proceso;

					return proceso_guardado_n->PID == proceso_n->PID;
				}

				t_pcb *proceso_guardado = list_find(lista_de_procesos_sin_repetir, es_proceso_n);

				if(proceso_guardado == NULL){
					list_add(lista_de_procesos_sin_repetir, proceso_n);
				}
			}
		}

		list_iterate(cola_n->elements, buscar_proceso);
	}

	list_iterate(lista_de_colas, agregar_pcb_de_la_cola);

	return lista_de_procesos_sin_repetir;
}

void eliminar_por_pcb(t_pcb* pcb_a_eliminar, t_list* lista){

	pcb_args_destroy(pcb_a_eliminar);

	bool fue_eliminado = list_remove_element(lista, pcb_a_eliminar);

	if(!fue_eliminado){
		 log_error(logger, "no se encontro el pcb a eliminar en la lista dada");
	}
}

void avisar_memoria_finalizar_proceso(t_pcb* proceso_a_finalizar, char* estado_anterior){
	// Eliminar y solicitar la liberacion de memoira
	t_paquete *paquete = crear_paquete(FINALIZAR_PROCESO_MEMORIA);
	agregar_a_paquete_sin_agregar_tamanio(paquete,&(proceso_a_finalizar->PID),sizeof(int));

	enviar_paquete(paquete,socket_memoria);
	eliminar_paquete(paquete);
	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_a_finalizar->PID, estado_anterior,"EXIT");
}

bool finalizar_proceso_si_esta_en_list(t_list* procesos_bloqueado, bool(*closure)(void*), char *estado_anterior){
	//buscar en cola de bloqueados por recurso
	t_pcb* pcb_a_eliminar = list_find(procesos_bloqueado, closure);

	if(pcb_a_eliminar == NULL){
		return false;
	}

	avisar_memoria_finalizar_proceso(pcb_a_eliminar, estado_anterior);

	eliminar_por_pcb(pcb_a_eliminar, procesos_bloqueado);

	return true;
}

bool finalizar_proceso_si_esta_en_alguna_queue_del_list(t_list* lista_de_colas, bool(*condicion)(void*), char *estado_anterior){
	bool fue_eliminada = false;

	void bucar_si_esta_en_alugna_cola(void *args){
		t_queue *cola_bloqueado = (t_queue *) args;

		if(finalizar_proceso_si_esta_en_list(cola_bloqueado->elements, condicion, estado_anterior)){
			fue_eliminada = true;
		}
	}

	list_iterate(lista_de_colas, bucar_si_esta_en_alugna_cola);

	return fue_eliminada;
}


void liberar_recursos_de(int pid_proceso_a_liberar){
	char *pid = string_itoa(pid_proceso_a_liberar);

	t_list *recursos_del_proceso = dictionary_get(matriz_recursos_asignados, pid);
	t_list *recursos_del_proceso_dup = duplicar_lista_recursos(recursos_del_proceso);


	void actualizar_recurso_disponible(void *arg){
		t_recurso *recurso_n = (t_recurso *) arg;

		int indice_recurso = obtener_indice_recurso(recursos, recurso_n->nombre_recurso);

		recursos_disponible[indice_recurso] ++;

	}

	list_iterate(recursos_del_proceso, actualizar_recurso_disponible);

	destroy_proceso_en_matriz(matriz_recursos_pendientes, pid);
	destroy_proceso_en_matriz(matriz_recursos_asignados, pid);

	deteccion_de_deadlock();

	//desbloqueo a los procesos en caso de que esten bloqueados por algun recurso liberado

	void desbloquear_por_recurso_liberado(void *args){
		t_recurso *recurso_n = (t_recurso *)args;

		if(recurso_n->instancias_en_posesion > 0){
			char *nombre_recurso = recurso_n->nombre_recurso;
			t_queue* cola_bloqueados= (t_queue*) dictionary_get(recurso_bloqueado, nombre_recurso);

			int cantidad_procesos_bloqueados = queue_size(cola_bloqueados);

			if(cantidad_procesos_bloqueados > 0){
				t_pcb *proceso_desbloqueado = queue_pop(cola_bloqueados);
				char *pid_desbloqueado = string_itoa(proceso_desbloqueado->PID);

				log_info(logger, "PID: %s - Estado Anterior: %s - Estado Actual: %s", pid_desbloqueado, "BLOC","READY");

				int indice_recurso = obtener_indice_recurso(recursos, recurso_n->nombre_recurso);

				//actualizo los recursos disponibles para que no se le actualize a otro proceso
				recursos_disponible[indice_recurso] --;

				decrementar_recurso_en_matriz(&matriz_recursos_pendientes, nombre_recurso, pid_desbloqueado, cant_recursos);
				incrementar_recurso_en_matriz(&matriz_recursos_asignados, nombre_recurso, pid_desbloqueado, cant_recursos);

				pasar_a_ready(proceso_desbloqueado);
			}
		}
	}


	list_iterate(recursos_del_proceso_dup, desbloquear_por_recurso_liberado);

	destroy_lista_de_recursos(recursos_del_proceso_dup);
}

void liberar_archivos_de(int pid_proceso_a_liberar){
	char *pid = string_itoa(pid_proceso_a_liberar);

	//TODO liberar archivos

	deteccion_de_deadlock();

	//desbloqueo a un proceso en caso de que este bloqueado por el archivo

}


void finalizar_proceso(t_instruccion *comando) {
	//Liberar recursos, archivos ,memoria y finalizar el proceso por EXIT

	//Tomo el PID de el comando por consola
	int pid_buscado = atoi(comando->parametros[0]);

	//Declaro funcion de busqueda
	bool _encontrar_por_pid(void *pcb) {
		t_pcb *pcb_n = (t_pcb*) pcb;

		if (pcb_n == NULL) {
			return false;
		}
		return pcb_n->PID == pid_buscado;
	}

	//Liberar recursos, archivos
	liberar_recursos_de(pid_buscado);
	liberar_archivos_de(pid_buscado);


	//mutex de la variable compartida
	sem_wait(&m_proceso_ejecutando);
	if (proceso_ejecutando != NULL && proceso_ejecutando->PID == pid_buscado) {
		//Creo mensaje de INT
		sem_post(&m_proceso_ejecutando);
		char* mensaje = string_new();
		char* pid_str = string_itoa(pid_buscado);
		string_append(&mensaje, "Finalizacion del proceso PID: ");
		string_append(&mensaje, pid_str);
		enviar_mensaje(mensaje, socket_cpu_interrupt, INTERRUPCION);

		sem_wait(&recibir_interrupcion);//espero recibir la interrupcion de cpu

		sem_wait(&m_proceso_ejecutando);
		avisar_memoria_finalizar_proceso(proceso_ejecutando, "EXEC");
		//libero
		pcb_args_destroy(proceso_ejecutando);
		sem_post(&m_proceso_ejecutando);

		poner_a_ejecutar_otro_proceso();
	} else {
		//mutex de la variable compartida
		sem_post(&m_proceso_ejecutando);

		//Buscar en cola de ready
		sem_wait(&m_cola_ready);
		bool estaba_en_el_list = finalizar_proceso_si_esta_en_list(cola_ready->elements, _encontrar_por_pid, "READY");
		sem_post(&m_cola_ready);

		if (!estaba_en_el_list) {
			//Buscar en cola de new
			sem_wait(&m_cola_new);
			bool estaba_en_el_list = finalizar_proceso_si_esta_en_list(cola_new->elements, _encontrar_por_pid, "NEW");
			sem_post(&m_cola_new);

			if (!estaba_en_el_list) {
				//buscar en cola de bloqueados por archivos
				sem_wait(&m_cola_de_procesos_bloqueados_para_cada_archivo);
				t_list *procesos_bloqueados = dictionary_elements(colas_de_procesos_bloqueados_para_cada_archivo);
				sem_post(&m_cola_de_procesos_bloqueados_para_cada_archivo);

				bool estaba_en_el_list = finalizar_proceso_si_esta_en_alguna_queue_del_list(procesos_bloqueados, _encontrar_por_pid, "BLOC");

				if (!estaba_en_el_list) {
					//buscar en cola de bloqueados por recurso
					sem_wait(&m_recurso_bloqueado);
					t_list *procesos_bloqueados_por_recurso = dictionary_elements(recurso_bloqueado);
					sem_post(&m_recurso_bloqueado);

					bool estaba_en_el_list = finalizar_proceso_si_esta_en_alguna_queue_del_list(procesos_bloqueados_por_recurso, _encontrar_por_pid, "BLOC");

					if(!estaba_en_el_list){
						log_error(logger,"No se encontro ningun proceso con el PID indicado");
					}
				}
			}

		}
	}

	log_info(logger, "Finaliza el proceso %d - Motivo: SUCCESS", pid_buscado);

	sem_wait(&m_proceso_ejecutando);
	if(proceso_ejecutando == NULL){
		sem_post(&m_proceso_ejecutando);
		poner_a_ejecutar_otro_proceso();
	}else {
		sem_post(&m_proceso_ejecutando);
	}

	destroy_commando(comando);
}

void detener_planificacion() {
	//Se detiene la planificacion de largo y corto plazo (el proceso en EXEC continua hasta salir) (si se encuentrand detenidos ignorar)
	if(planificacion_detenida == true){
		log_info(logger, "La planificacion ya se encuentra detenida");
	}else{
		pthread_mutex_lock(&m_planificador_largo_plazo);
		pthread_mutex_lock(&m_planificador_corto_plazo);
		planificacion_detenida = true;
	}
}

void iniciar_planificacion() {
	//Reanudar los planificadores (si no se encuentran detenidos ignorar)
	if(planificacion_detenida == false){
		log_info(logger, "La planificacion ya se encuentra activa");
	}else{
		pthread_mutex_unlock(&m_planificador_largo_plazo);
		pthread_mutex_unlock(&m_planificador_corto_plazo);
		planificacion_detenida = false;
	}
}


void multiprogramacion(t_instruccion* comando) {
	//Modificar el grado de multiprogramacion (no desalojar procesos)
	int nuevo_grado_multiprogramacion = atoi(comando->parametros[0]);
	if (grado_max_multiprogramacion == nuevo_grado_multiprogramacion){
		log_info(logger, "El grado de multiprogramacion actual ya es %d", grado_max_multiprogramacion);
	}else if (grado_max_multiprogramacion != nuevo_grado_multiprogramacion){
		log_info(logger, "Cambiando grado de multiprogramacion de: %d a %d", grado_max_multiprogramacion, nuevo_grado_multiprogramacion);
		grado_max_multiprogramacion = nuevo_grado_multiprogramacion;

	}else{
		log_error(logger, "Se produjo un error al recibir el comando, ya que paso algo imposible, por favor verificar");
	}

	destroy_commando(comando);
}
void proceso_estado() {
	//Listara por consola todos los estados y los procesos que se encuentran dentro de ellos
	log_info(logger, "Lista de todos los procesos del sistema y su respectivo estado:");

	sem_wait(&m_proceso_ejecutando);
	if(proceso_ejecutando!= NULL){
	log_info(logger, "PID: %d ESTADO: %s \n", proceso_ejecutando->PID, proceso_ejecutando->proceso_estado);
	}
	sem_post(&m_proceso_ejecutando);


	sem_wait(&m_cola_ready);
	int tamanio_cola_ready = queue_size(cola_ready);
	if(tamanio_cola_ready > 0){
		for(int i = 0; i< tamanio_cola_ready; i++){
			t_pcb* pcb = list_get(cola_ready->elements,i);
			log_info(logger, "PID: %d ESTADO: %s \n", pcb->PID, pcb->proceso_estado);
		}
	}
	sem_post(&m_cola_ready);

	sem_wait(&m_cola_new);
	int tamanio_cola_new = queue_size(cola_new);
	if(tamanio_cola_new > 0){
		for(int i = 0; i< tamanio_cola_new; i++){
			t_pcb* pcb = list_get(cola_new->elements,i);
			log_info(logger, "PID: %d ESTADO: %s \n", pcb->PID, pcb->proceso_estado);
		}
	}
	sem_post(&m_cola_new);

	sem_wait(&m_cola_de_procesos_bloqueados_para_cada_archivo);
	t_list *procesos_bloqueados_colas = dictionary_elements(colas_de_procesos_bloqueados_para_cada_archivo);
	t_list* procesos_bloqueados_sin_repetir = obtener_procesos_bloqueados_sin_repetir_de(procesos_bloqueados_colas);
	int tamanio_lista_procesos_bloqueados = list_size(procesos_bloqueados_sin_repetir);
	for(int i = 0; i< tamanio_lista_procesos_bloqueados; i++){
		t_pcb* pcb = list_get(procesos_bloqueados_sin_repetir,i);

		log_info(logger, "PID: %d ESTADO: %s \n", pcb->PID, pcb->proceso_estado);
	}
	sem_post(&m_cola_de_procesos_bloqueados_para_cada_archivo);


	sem_wait(&m_recurso_bloqueado);
	t_list *procesos_bloqueados_por_recurso_colas = dictionary_elements(recurso_bloqueado);
	t_list *procesos_bloqueados_por_recurso = obtener_procesos_bloqueados_sin_repetir_de(procesos_bloqueados_por_recurso_colas);
	int tamanio_cola_bloqueados_por_recurso = list_size(procesos_bloqueados_por_recurso);
	for(int i = 0; i< tamanio_cola_bloqueados_por_recurso; i++){
		t_pcb* pcb = list_get(procesos_bloqueados_por_recurso,i);
		log_info(logger, "PID: %d ESTADO: %s \n", pcb->PID, pcb->proceso_estado);
	}
	sem_post(&m_recurso_bloqueado);

	list_destroy(procesos_bloqueados_por_recurso);
	list_destroy(procesos_bloqueados_sin_repetir);
	list_destroy(procesos_bloqueados_por_recurso_colas);
	list_destroy(procesos_bloqueados_colas);
}



void levantar_consola() {
	while (1) {
		char *linea = readline(">");

		//es para que guarde un historial de los comandos y poder usar las flechas
		if(linea){
			add_history(linea);
		}

		t_instruccion *comando = armar_comando(linea);
		parametros_lenght(comando);

		if (strcmp(comando->opcode, "INICIAR_PROCESO") == 0) {
			iniciar_proceso(comando);

		} else if (strcmp(comando->opcode, "FINALIZAR_PROCESO") == 0) {
			finalizar_proceso(comando);

		} else if (strcmp(comando->opcode, "DETENER_PLANIFICACION") == 0) {
			destroy_commando(comando);
			detener_planificacion();

		} else if (strcmp(comando->opcode, "INICIAR_PLANIFICACION") == 0) {
			destroy_commando(comando);
			iniciar_planificacion();

		} else if (strcmp(comando->opcode, "MULTIPROGRAMACION") == 0) {
			multiprogramacion(comando);

		} else if (strcmp(comando->opcode, "PROCESO_ESTADO") == 0) {
			destroy_commando(comando);
			proceso_estado();

		} else {
			log_error(logger,"Comando desconocido campeon, leete la documentacion de nuevo :p");
			destroy_commando(comando);
		}

		free(linea);
	}
}
