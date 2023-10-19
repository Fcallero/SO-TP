#include "consola.h"
sem_t despertar_planificacion_largo_plazo;
t_pcb *proceso_ejecutando;
t_dictionary *colas_de_procesos_bloqueados_para_cada_archivo;
bool planificacion_detenida = false;
int grado_max_multiprogramacion;



t_instruccion* armar_comando(char *cadena) {

	t_instruccion *comando = malloc(sizeof(t_instruccion));
	string_to_upper(cadena); //paso la cadena a mayuscula

	char *token = strtok(cadena, " "); // obtiene el primer elemento en token

	comando->opcode = token;

	token = strtok(NULL, " "); // avanza al segundo elemento
	int i = 0; // Variable local utilizada para cargar el array de parametros

	while (token != NULL) { //Ingresa si el parametro no es NULL

		comando->parametros[i] = token; //Carga el parametro en el array de la struct
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
	strcpy(pcb_proceso->proceso_estado, "NEW");
	pcb_proceso->tiempo_llegada_ready = 0;
	pcb_proceso->prioridad = atoi(comando->parametros[2]);
	pcb_proceso->tabla_archivos_abiertos_del_proceso = NULL;
	pcb_proceso->comando = comando;

	//este malloc para evitar el segmentation fault en el envio del contexto de ejecución a cpu
	pcb_proceso->registros_CPU = malloc(sizeof(registros_CPU));


	agregar_cola_new(pcb_proceso);

	sem_post(&despertar_planificacion_largo_plazo);

}

void eliminar_por_pcb(t_pcb* pcb_a_eliminar, t_list* lista){
	list_remove_element(lista, pcb_a_eliminar);
	free(pcb_a_eliminar);
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

//TODO revisar si es necesario un mutex
	//mutex de la variable compartida
	sem_wait(&m_proceso_ejecutando);
	if (proceso_ejecutando->PID == pid_buscado) {
		//Creo mensaje de INT
		sem_post(&m_proceso_ejecutando);
		char* mensaje = string_new();
		char* pid_str = string_itoa(pid_buscado);
		string_append(&mensaje, "Finalizacion del proceso PID: ");
		string_append(&mensaje, pid_str);
		enviar_mensaje(mensaje, socket_cpu_interrupt, FINALIZAR_PROCESO);

		t_contexto_ejec* contexto = recibir_contexto_de_ejecucion(socket_cpu_interrupt);

		// Eliminar y solicitar la liberacion de memoira
		t_paquete *paquete = crear_paquete(FINALIZAR_PROCESO_MEMORIA);
		sem_wait(&m_proceso_ejecutando);
		agregar_a_paquete_sin_agregar_tamanio(paquete,&(proceso_ejecutando->PID),sizeof(int));
		sem_post(&m_proceso_ejecutando);

		enviar_paquete(paquete,socket_memoria);

		sem_wait(&m_proceso_ejecutando);
		log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_ejecutando->PID, "EXEC","EXIT");
		sem_post(&m_proceso_ejecutando);
		//libero
		eliminar_paquete(paquete);

		destroy_proceso_ejecutando();
		contexto_ejecucion_destroy(contexto);

		poner_a_ejecutar_otro_proceso();


	} else {
		//mutex de la variable compartida
		sem_post(&m_proceso_ejecutando);
		//Buscar en cola de ready
		sem_wait(&m_cola_ready);
		t_pcb *pcb_a_eliminar = list_find(cola_ready->elements, _encontrar_por_pid);
		sem_post(&m_cola_ready);

		if (pcb_a_eliminar != NULL) {
			sem_wait(&m_cola_ready);
			eliminar_por_pcb(pcb_a_eliminar, cola_ready->elements);
			sem_post(&m_cola_ready);
			sem_post(&despertar_corto_plazo);
		} else {
			//Buscar en cola de new
			sem_wait(&m_cola_new);
			pcb_a_eliminar = list_find(cola_new->elements, _encontrar_por_pid);
			sem_post(&m_cola_new);
			if (pcb_a_eliminar != NULL) {
				sem_wait(&m_cola_new);
				eliminar_por_pcb(pcb_a_eliminar, cola_new->elements);
				sem_post(&m_cola_new);
				sem_post(&despertar_planificacion_largo_plazo);
			} else {
				//buscar en cola de bloqueados por archivos
				sem_wait(&m_cola_de_procesos_bloqueados_para_cada_archivo);
				t_list *procesos_bloqueados = dictionary_elements(colas_de_procesos_bloqueados_para_cada_archivo);
				sem_post(&m_cola_de_procesos_bloqueados_para_cada_archivo);
				pcb_a_eliminar = list_find(procesos_bloqueados, _encontrar_por_pid);
				if (pcb_a_eliminar != NULL) {
					sem_wait(&m_cola_de_procesos_bloqueados_para_cada_archivo);
					eliminar_por_pcb(pcb_a_eliminar, procesos_bloqueados);
					sem_post(&m_cola_de_procesos_bloqueados_para_cada_archivo);
					sem_post(&despertar_corto_plazo);
				} else {
					printf(
							"No se encontro ningun proceso con el PID indicado\n");
				}
			}

		}
	}

}

void detener_planificacion() {
	//Se detiene la planificacion de largo y corto plazo (el proceso en EXEC continua hasta salir) (si se encuentrand detenidos ignorar)
	if(planificacion_detenida == true){
		printf("La planificacion ya se encuentra detenida");
	}else{
		sem_wait(&despertar_planificacion_largo_plazo);
		sem_wait(&despertar_corto_plazo);
		planificacion_detenida = true;
	}
}

void iniciar_planificacion() {
	//Reanudar los planificadores (si no se encuentran detenidos ignorar)
	if(planificacion_detenida == false){
		printf("La planificacion ya se encuentra activa");
	}else{
		sem_post(&despertar_planificacion_largo_plazo);
		sem_post(&despertar_corto_plazo);
		planificacion_detenida = false;
	}
}


void multiprogramacion(t_instruccion* comando) {
	//Modificar el grado de multiprogramacion (no desalojar procesos)
	int nuevo_grado_multiprogramacion = (long)comando->parametros[0];
	if (grado_max_multiprogramacion == nuevo_grado_multiprogramacion){
		printf("El grado de multiprogramacion actual ya es %d", grado_max_multiprogramacion);
	}else if (grado_max_multiprogramacion != nuevo_grado_multiprogramacion){
		printf("Cambiando grado de multiprogramacion de: %d a %d", grado_max_multiprogramacion, nuevo_grado_multiprogramacion);
		grado_max_multiprogramacion = nuevo_grado_multiprogramacion;

		sem_post(&despertar_planificacion_largo_plazo);
	}else{
		printf("Se produjo un error al recibir el comando, por favor verificar");
	}
}
void proceso_estado() {
	//Listara por consola todos los estados y los procesos que se encuentran dentro de ellos
	printf("Lista de todos los procesos del sistema y su respectivo estado:");

	sem_wait(&m_proceso_ejecutando);
	printf("PID: %d ESTADO: %s \n", proceso_ejecutando->PID, proceso_ejecutando->proceso_estado);
	sem_post(&m_proceso_ejecutando);

	sem_wait(&m_cola_ready);
	int tamanio_cola_ready = queue_size(cola_ready);
	for(int i = 0; i< tamanio_cola_ready; i++){
		t_pcb* pcb = list_get(cola_ready->elements,i);
		printf("PID: %d ESTADO: %s \n", pcb->PID, pcb->proceso_estado);
	}
	sem_post(&m_cola_ready);

	sem_wait(&m_cola_new);
	int tamanio_cola_new = queue_size(cola_new);
	for(int i = 0; i< tamanio_cola_new; i++){
		t_pcb* pcb = list_get(cola_new->elements,i);
		printf("PID: %d ESTADO: %s \n", pcb->PID, pcb->proceso_estado);
	}
	sem_post(&m_cola_ready);

	sem_wait(&m_cola_de_procesos_bloqueados_para_cada_archivo);
	t_list *procesos_bloqueados = dictionary_elements(colas_de_procesos_bloqueados_para_cada_archivo);
	int tamanio_cola_bloqueados = list_size(procesos_bloqueados);
	for(int i = 0; i< tamanio_cola_bloqueados; i++){
		t_pcb* pcb = list_get(procesos_bloqueados,i);
		printf("PID: %d ESTADO: %s \n", pcb->PID, pcb->proceso_estado);
	}
	sem_post(&m_cola_de_procesos_bloqueados_para_cada_archivo);
}



void levantar_consola() {
	while (1) {
		char *linea = readline(">");
		t_instruccion *comando = malloc(sizeof(t_instruccion));
		comando = armar_comando(linea);
		parametros_lenght(comando);

		if (strcmp(comando->opcode, "INICIAR_PROCESO") == 0) {
			iniciar_proceso(comando);

		} else if (strcmp(comando->opcode, "FINALIZAR_PROCESO") == 0) {
			finalizar_proceso(comando);

		} else if (strcmp(comando->opcode, "DETENER_PLANIFICACION") == 0) {
			detener_planificacion();

		} else if (strcmp(comando->opcode, "INICIAR_PLANIFICACION") == 0) {
			iniciar_planificacion();

		} else if (strcmp(comando->opcode, "MULTIPROGRAMACION") == 0) {
			multiprogramacion(comando);

		} else if (strcmp(comando->opcode, "PROCESO_ESTADO") == 0) {
			proceso_estado();

		} else {
			log_error(logger,"Comando desconocido campeon, leete la documentacion de nuevo :p");
		}

		free(linea);
	}
}
