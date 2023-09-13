#include "peticiones_cpu_dispatch.h"


//este es un hilo que se levantaria al bloquear por io como en el tp anterior
//no estan creadas las estructuras necesarias ni el como lo haremos
//asique dejo esta colgada para cuando llegue el momento
//carinios con enie UwU

void* simulacion_io(void* arg){

	t_argumentos_simular_sleep* argumentos = (t_argumentos_simular_sleep* ) arg;
	int tiempo_io = argumentos->tiempo_io;

	sem_wait(&m_proceso_ejecutando);
	t_pcb* proceso_en_IO = proceso_ejecutando;
	sem_post(&m_proceso_ejecutando);

	// despierta el semáforo para poner a ejecutar otro proceso

	//ya veremos este de abajo
	//sem_post(&esperar_proceso_ejecutando);
	//el de arriba


	esperar_por(tiempo_io *1000);

	sem_wait(&m_proceso_ejecutando);
	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_en_IO->PID, "BLOC","READY");




	pasar_a_ready(proceso_en_IO);
	sem_post(&m_proceso_ejecutando);

	return NULL;
}


