#include "planificador_largo_plazo.h"

t_queue* cola_new;
t_queue* cola_ready;
t_pcb* proceso_ejecutando;
char* algoritmo_planificacion;


sem_t despertar_corto_plazo;
sem_t m_cola_ready;
sem_t m_cola_new;
sem_t m_proceso_ejecutando;
sem_t m_recurso_bloqueado;
sem_t m_cola_de_procesos_bloqueados_para_cada_archivo;
sem_t despertar_planificacion_largo_plazo;
t_dictionary* colas_de_procesos_bloqueados_para_cada_archivo;
t_dictionary* recurso_bloqueado;


void inicializar_colas_y_semaforos(){
	cola_new = queue_create();
	cola_ready = queue_create();
	sem_init(&m_cola_ready,0,1);
	sem_init(&m_cola_new, 0, 1);
	sem_init(&m_proceso_ejecutando, 0, 1);
	sem_init(&despertar_corto_plazo,0,0);
	sem_init(&m_recurso_bloqueado, 0, 1);
	sem_init(&m_cola_de_procesos_bloqueados_para_cada_archivo, 0,1);
	sem_init(&despertar_planificacion_largo_plazo,0,0);
}



char* listar_pids_cola_ready(void){

	char** array_pids = string_array_new();


	t_list* lista_ready = cola_ready->elements;

	int tamano_cola_ready = queue_size(cola_ready);

	for(int i =0; i< tamano_cola_ready; i++){

		t_pcb* item = list_get(lista_ready, i);

		char *PID_string = string_itoa(item->PID);

		string_array_push(&array_pids, PID_string);
		string_array_push(&array_pids, ",");
	}

	//saco la ultima comma
	string_array_pop(array_pids);


	char* string_pids = pasar_a_string(array_pids);

	string_array_destroy(array_pids);

	return string_pids ;
}

// si el proceso no es new, no es necesario el socket de memoria
void agregar_proceso_a_ready(int conexion_memoria, char* algoritmo_planificacion){
	sem_wait(&m_cola_new);
	t_pcb* proceso_new_a_ready = queue_pop(cola_new);
	sem_post(&m_cola_new);

	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_new_a_ready->PID, "NEW", "READY");

	//envio a memoria de instrucciones
	t_paquete* paquete = crear_paquete(INICIAR_PROCESO);
	agregar_a_paquete(paquete,proceso_new_a_ready->comando->parametros[0],proceso_new_a_ready->comando->parametro1_lenght);
	agregar_a_paquete_sin_agregar_tamanio(paquete,proceso_new_a_ready->PID,sizeof(int));
	enviar_paquete(paquete,socket_memoria);

	eliminar_paquete(paquete);

	free(proceso_new_a_ready->comando);//el comando luego no se va a usar, lo libero


	sem_wait(&m_cola_ready);
	queue_push(cola_ready, proceso_new_a_ready);
	char *pids = listar_pids_cola_ready();
	sem_post(&m_cola_ready);

	log_info(logger, "Cola Ready %s: [%s]",algoritmo_planificacion, pids);

	free(pids);

	// si ya esta ejecutando un proceso, cuando termine se llama al planificador de largo plazo
	sem_wait(&m_proceso_ejecutando);
	if(proceso_ejecutando == NULL){
		sem_post(&m_proceso_ejecutando);
		sem_post(&despertar_corto_plazo);
	} else {
		sem_post(&m_proceso_ejecutando);
	}
}

int calcular_procesos_en_memoria(int procesos_en_ready){

	int procesos_bloqueados = 0;

	void _calcular_procesos_bloqueados(char* key, void* value){
		t_queue* cola_bloqueados_recurso_n = (t_queue*) value;

		if(queue_size(cola_bloqueados_recurso_n) != 0){
			procesos_bloqueados += queue_size(cola_bloqueados_recurso_n);
		}
	}

	dictionary_iterator(recurso_bloqueado, _calcular_procesos_bloqueados);


	sem_wait(&m_cola_de_procesos_bloqueados_para_cada_archivo);
	void _calcular_procesos_bloqueados_por_archivo(char* key, void* value){
		t_queue* cola_bloqueados_archivo_n = (t_queue*) value;

		if(queue_size(cola_bloqueados_archivo_n) != 0){
			procesos_bloqueados += queue_size(cola_bloqueados_archivo_n);
		}
	}

	dictionary_iterator(colas_de_procesos_bloqueados_para_cada_archivo, _calcular_procesos_bloqueados_por_archivo);
	sem_post(&m_cola_de_procesos_bloqueados_para_cada_archivo);



	sem_wait(&m_proceso_ejecutando);
	if(proceso_ejecutando != NULL){
		sem_post(&m_proceso_ejecutando);
		procesos_bloqueados ++;
	} else {
		sem_post(&m_proceso_ejecutando);
	}


	return procesos_bloqueados + procesos_en_ready;
}

void *planificar_nuevos_procesos_largo_plazo(void *arg){



	while(1){
		sem_wait(&despertar_planificacion_largo_plazo);
		sem_wait(&m_cola_ready);
		int tamanio_cola_ready = queue_size(cola_ready);
		sem_post(&m_cola_ready);
		sem_wait(&m_cola_new);
		int tamanio_cola_new = queue_size(cola_new);
		sem_post(&m_cola_new);

		int procesos_en_memoria_total = calcular_procesos_en_memoria(tamanio_cola_ready);

		// sumo uno para simular si agrego a ready el proceso qeu esta en new
		procesos_en_memoria_total ++;


		if(tamanio_cola_new != 0 && procesos_en_memoria_total < grado_max_multiprogramacion){

			//verificar si se lo puede admitir a la cola de ready
			agregar_proceso_a_ready(socket_memoria, algoritmo_planificacion);
		}
	}

	return NULL;
}


void pasar_a_ready(t_pcb* proceso_bloqueado){

	sem_wait(&m_cola_ready);

	queue_push(cola_ready, proceso_bloqueado);
	int procesos_en_ready = queue_size(cola_ready);

	char *pids = listar_pids_cola_ready();

	sem_post(&m_cola_ready);


	log_info(logger, "Cola Ready %s: [%s]",algoritmo_planificacion, pids);

	free(pids);

	sem_wait(&m_proceso_ejecutando);
	if(proceso_ejecutando == NULL && procesos_en_ready > 0 ){
		sem_post(&m_proceso_ejecutando);
		sem_post(&despertar_corto_plazo);
	} else {
		sem_post(&m_proceso_ejecutando);
	}

}


void agregar_cola_new(t_pcb* pcb_proceso){
	sem_wait(&m_cola_new);
	queue_push(cola_new, pcb_proceso);
	sem_post(&m_cola_new);

	log_info(logger, "Se crea el proceso %d en NEW", pcb_proceso->PID);
}
