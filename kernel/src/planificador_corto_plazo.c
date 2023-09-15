#include "planificador_corto_plazo.h"


void *planificar_nuevos_procesos_corto_plazo(void *arg){

	while(1){
		if(string_equals_ignore_case(algoritmo_planificacion, "FIFO")){
			planificar_corto_plazo_fifo();
		}else if(string_equals_ignore_case(algoritmo_planificacion, "PRIORIDADES")){
			planificar_corto_plazo_prioridades();
		}else {
			planificar_corto_plazo_round_robbin();
		}
	}

	return NULL;
}

void planificar_corto_plazo_fifo(){
	sem_wait(&despertar_corto_plazo);
	sem_wait(&m_cola_ready);

	if(queue_size(cola_ready) == 0){
		sem_post(&m_cola_ready);
		return;
	}

	t_pcb *proceso_a_ejecutar = queue_pop(cola_ready);
	sem_post(&m_cola_ready);

	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando = proceso_a_ejecutar;

	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_a_ejecutar->PID, "READY", "EXEC");
	sem_post(&m_proceso_ejecutando);


	// CREAR CONTEXTO DE EJCUCION

	//TODO CREAR FUNCION ENVIAR_CONTEXTO_A
	//enviar_contexto_ejecucion_a(contexto_ejecucion, PETICION_CPU, socket_cpu_dispatch);

	//TODO CREAR FUNCION DESTROY PARA CONTEXTO DE EJECUCION
	// destroy contexto de ejecucion
}

void planificar_corto_plazo_prioridades(){
	sem_wait(&despertar_corto_plazo);
	sem_wait(&m_cola_ready);

	if(queue_size(cola_ready) == 0){
		sem_post(&m_cola_ready);
		return;
	}

	// reordenar cola en base a las prioridades (ver la de hrrn del tp anterior)

	t_pcb *proceso_a_ejecutar = queue_pop(cola_ready);
	sem_post(&m_cola_ready);

	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando = proceso_a_ejecutar;
	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_a_ejecutar->PID, "READY", "EXEC");
	sem_post(&m_proceso_ejecutando);



	// CREAR CONTEXTO DE EJCUCION

	//enviar_contexto_ejecucion_a(contexto_ejecucion, PETICION_CPU, socket_cpu_dispatch);

	// destroy contexto de ejecucion

	//TODO algoritmo planificador corto plazo prioridades
}

void planificar_corto_plazo_round_robbin(){

	sem_wait(&despertar_corto_plazo);
	sem_wait(&m_cola_ready);

	if(queue_size(cola_ready) == 0){
		sem_post(&m_cola_ready);
		return;
	}


	t_pcb *proceso_a_ejecutar = queue_pop(cola_ready);
	sem_post(&m_cola_ready);

	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando = proceso_a_ejecutar;

	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_a_ejecutar->PID, "READY", "EXEC");
	sem_post(&m_proceso_ejecutando);



	// CREAR CONTEXTO DE EJCUCION

	//enviar_contexto_ejecucion_a(contexto_ejecucion, PETICION_CPU, socket_cpu_dispatch);


	// destroy contexto de ejecucion


	esperar_por(quantum);

	sem_wait(&m_proceso_ejecutando);

	log_info(logger, "PID: %d - Desalojado por fin de Quantum", proceso_a_ejecutar->PID);
	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_a_ejecutar->PID, "EXEC", "READY");

	// TODO notificar desalojo a CPU (tal vez una interrupccion?)

	proceso_ejecutando = NULL;

	sem_post(&m_proceso_ejecutando);

	pasar_a_ready(proceso_a_ejecutar);


	//TODO algoritmo planificador corto plazo RR
}


