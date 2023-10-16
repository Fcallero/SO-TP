#include "consola.h"
sem_t despertar_planificacion_largo_plazo;

t_instruccion* armar_comando(char* cadena){

	t_instruccion* comando = malloc(sizeof(t_instruccion));
	string_to_upper(cadena); //paso la cadena a mayuscula

	char* token = strtok(cadena, " "); // obtiene el primer elemento en token

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


void parametros_lenght(t_instruccion* ptr_inst){

		ptr_inst->opcode_lenght = strlen(ptr_inst->opcode)+1;

		if(ptr_inst->parametros[0] != NULL){
			ptr_inst->parametro1_lenght = strlen(ptr_inst->parametros[0])+1;
		} else {
			ptr_inst->parametro1_lenght = 0;
		}
		if(ptr_inst->parametros[1] != NULL){
			ptr_inst->parametro2_lenght = strlen(ptr_inst->parametros[1])+1;
		} else {
			ptr_inst->parametro2_lenght = 0;
		}
		if(ptr_inst->parametros[2] != NULL){
			ptr_inst->parametro3_lenght = strlen(ptr_inst->parametros[2])+1;
		} else {
			ptr_inst->parametro3_lenght = 0;
		}
}

void iniciar_proceso(t_instruccion* comando){
	//Armar PCB, colocarla en estado new y enviar el path a memoria
	t_pcb* pcb_proceso = malloc(sizeof(t_pcb));

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

void finalizar_proceso(){
	//Liberar recursos, archivos ,memoria y finalizar el proceso por EXIT


}

void detener_planificacion(){
	//Se detiene la planificacion de largo y corto plazo (el proceso en EXEC continua hasta salir) (si se encuentrand detenidos ignorar)
}

void iniciar_planificacion(){
	//Reanudar los planificadores (si no se encuentran detenidos ignorar)
}

void multiprogramacion(){
	//Modificar el grado de multiprogramacion (no desalojar procesos)
}
void proceso_estado(){
	//Listara por consola todos los estados y los procesos que se encuentran dentro de ellos
}

void verificar_entrada_comando(){

}

void levantar_consola(){
	while(1){
		char* linea = readline(">");
	    t_instruccion* comando = malloc(sizeof(t_instruccion));
		comando = armar_comando(linea);
		parametros_lenght(comando);

		if(strcmp(comando->opcode,"INICIAR_PROCESO")==0){
			iniciar_proceso(comando);

		}else if(strcmp(comando->opcode,"FINALIZAR_PROCESO")==0){
			finalizar_proceso();

		}else if(strcmp(comando->opcode,"DETENER_PLANIFICACION")==0){
			detener_planificacion();

		}else if(strcmp(comando->opcode,"INICIAR_PLANIFICACION")==0){
			iniciar_planificacion();

		}else if(strcmp(comando->opcode,"MULTIPROGRAMACION")==0){
			multiprogramacion();

		}else if(strcmp(comando->opcode,"PROCESO_ESTADO")==0){
			proceso_estado();

		}else{
			log_error(logger, "Comando desconocido campeon, leete la documentacion de nuevo :p");
		}

	 free(linea);
	}
}
