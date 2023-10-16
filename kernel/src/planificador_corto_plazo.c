#include "planificador_corto_plazo.h"

void reordenar_cola_ready_prioridades(){
	// reodena de mayor a menor para que al hacer pop, saque al de mayor proridad
	// cuando hace pop saca al primer elemento de la lista
	bool __proceso_mayor_prioridad(t_pcb* pcb_proceso1, t_pcb* pcb_proceso2){
		return pcb_proceso1->prioridad > pcb_proceso2->prioridad;
	}

	sem_wait(&m_cola_ready);
	list_sort(cola_ready->elements, (void *) __proceso_mayor_prioridad);
	sem_post(&m_cola_ready);
}

void notificar_desalojo_cpu_interrupt(){

	//el mensaje es el motivo del desalojo (usado solo para un log en cpu)
	enviar_mensaje("Desalojo por fin de quantum", socket_cpu_interrupt, INTERRUPCION);

	op_code operacion = recibir_operacion(socket_cpu_dispatch);

	if(operacion != INTERRUPCION){
		log_error(logger, "no se pudo recibir el contexto de ejecucion por desalojo de fin de quantum");
		return;
	}

	t_contexto_ejec *contexto = recibir_contexto_de_ejecucion(socket_cpu_dispatch);

	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando->program_counter = contexto->program_counter;
	proceso_ejecutando->registros_CPU->AX = contexto->registros_CPU->AX;
	proceso_ejecutando->registros_CPU->BX = contexto->registros_CPU->BX;
	proceso_ejecutando->registros_CPU->CX = contexto->registros_CPU->CX;
	proceso_ejecutando->registros_CPU->DX = contexto->registros_CPU->DX;
	sem_post(&m_proceso_ejecutando);


	contexto_ejecucion_destroy(contexto);
}

void crear_contexto_y_enviar_a_CPU(t_pcb* proceso_a_ejecutar){
	t_contexto_ejec* contexto_ejecucion = malloc(sizeof(t_contexto_ejec));
	contexto_ejecucion->instruccion = malloc(sizeof(t_instruccion));
	contexto_ejecucion->program_counter = proceso_a_ejecutar->program_counter;
	contexto_ejecucion->pid = proceso_a_ejecutar->PID;
	contexto_ejecucion->registros_CPU = proceso_a_ejecutar->registros_CPU;

	enviar_contexto_de_ejecucion_a(contexto_ejecucion, PETICION_CPU, socket_cpu_dispatch);


	contexto_ejecucion_destroy(contexto_ejecucion);
}

void planificar_corto_plazo_fifo() {
	sem_wait(&despertar_corto_plazo);
	sem_wait(&m_cola_ready);

	if (queue_size(cola_ready) == 0) {
		sem_post(&m_cola_ready);
		return;
	}

	t_pcb *proceso_a_ejecutar = queue_pop(cola_ready);
	sem_post(&m_cola_ready);

	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando = proceso_a_ejecutar;

	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s",
			proceso_a_ejecutar->PID, "READY", "EXEC");
	sem_post(&m_proceso_ejecutando);

	crear_contexto_y_enviar_a_CPU(proceso_a_ejecutar);
}

void planificar_corto_plazo_prioridades() {
	sem_wait(&despertar_corto_plazo);
	sem_wait(&m_cola_ready);

	if (queue_size(cola_ready) == 0) {
		sem_post(&m_cola_ready);
		return;
	}

	reordenar_cola_ready_prioridades();

	//saca el de mayor prioridad
	t_pcb *proceso_a_ejecutar = queue_pop(cola_ready);
	sem_post(&m_cola_ready);

	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando = proceso_a_ejecutar;
	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s",
			proceso_a_ejecutar->PID, "READY", "EXEC");
	sem_post(&m_proceso_ejecutando);

	crear_contexto_y_enviar_a_CPU(proceso_a_ejecutar);

}

void planificar_corto_plazo_round_robbin() {

	sem_wait(&despertar_corto_plazo);
	sem_wait(&m_cola_ready);

	if (queue_size(cola_ready) == 0) {
		sem_post(&m_cola_ready);
		return;
	}

	t_pcb *proceso_a_ejecutar = queue_pop(cola_ready);
	sem_post(&m_cola_ready);

	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando = proceso_a_ejecutar;

	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s",
			proceso_a_ejecutar->PID, "READY", "EXEC");
	sem_post(&m_proceso_ejecutando);

	crear_contexto_y_enviar_a_CPU(proceso_a_ejecutar);

	esperar_por(quantum);

	sem_wait(&m_proceso_ejecutando);

	log_info(logger, "PID: %d - Desalojado por fin de Quantum",
			proceso_a_ejecutar->PID);
	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s",
			proceso_a_ejecutar->PID, "EXEC", "READY");

	notificar_desalojo_cpu_interrupt();

	proceso_ejecutando = NULL;

	sem_post(&m_proceso_ejecutando);

	pasar_a_ready(proceso_a_ejecutar);

}

void enviar_contexto_de_ejecucion_a(t_contexto_ejec* contexto_a_ejecutar, op_code opcode, int socket_cliente){

	t_paquete *paquete = crear_paquete(opcode);

	agregar_a_paquete_sin_agregar_tamanio(paquete, &(contexto_a_ejecutar->pid),
			sizeof(int));

	agregar_a_paquete_sin_agregar_tamanio(paquete,
			&(contexto_a_ejecutar->program_counter), sizeof(int));

	agregar_a_paquete_sin_agregar_tamanio(paquete,
			&(contexto_a_ejecutar->registros_CPU->AX), sizeof(uint32_t));
	agregar_a_paquete_sin_agregar_tamanio(paquete,
			&(contexto_a_ejecutar->registros_CPU->BX), sizeof(uint32_t));
	agregar_a_paquete_sin_agregar_tamanio(paquete,
			&(contexto_a_ejecutar->registros_CPU->CX), sizeof(uint32_t));
	agregar_a_paquete_sin_agregar_tamanio(paquete,
			&(contexto_a_ejecutar->registros_CPU->DX), sizeof(uint32_t));

	enviar_paquete(paquete, socket_cliente);

	eliminar_paquete(paquete);
}

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

