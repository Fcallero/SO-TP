#include "consola.h"

void* levantar_consola(){
	//TODO crear consola interactiva
	while(1){
		char* linea = readline(">");
		 printf("comando recibido  \n");
	    t_comando* comando = malloc(sizeof(t_comando));
		comando = armar_comando(linea);
		int cod_op = comando->opcode;
		 switch(cod_op){
		 case INICIAR_PROCESO:
			 printf("Caso: iniciar proceso \n");
			// iniciar_proceso(comando);
			 break;
		 case FINALIZAR_PROCESO:
			 printf("Caso: finalizar proceso \n");
			 break;
		 case DETENER_PLANIFICACION:
			 printf("Caso: detener planificacion \n");
			 break;
		 case INICIAR_PLANIFICACION:
			 printf("Caso: iniciar planificacion \n");
			 break;
		 case MULTIPROGRAMACION:
			 printf("Caso: multiprogramacion \n");
			 break;
		 case PROCESO_ESTADO:
			 printf("Caso: proceso estado \n");
			 break;
		 }
	 free(linea);
	 free(comando);
}
}

t_comando* armar_comando(char* cadena){

	t_comando* comando = malloc(sizeof(t_comando));

	char* token = strtok(cadena, " "); // obtiene el primer elemento en token

	comando = cargar_opcode(token, comando);

	token = strtok(NULL, " "); // avanza al segundo elemento
	int i = 0; // Variable local utilizada para cargar el array de parametros

		while (token != NULL) { //Ingresa si el parametro no es NULL

			comando->parametros[i] = token; //Carga el parametro en el array de la struct
			token = strtok(NULL, " "); // obtiene el siguiente elemento
			i++; //Avanza en el array
		}
	return comando;
	}


t_comando* cargar_opcode(char* cadena, t_comando* comando){
	if(string_equals_ignore_case(cadena, "INICIAR_PROCESO")){
		comando->opcode = INICIAR_PROCESO;
	} else if(string_equals_ignore_case(cadena, "FINALIZAR_PROCESO")){
		comando->opcode = FINALIZAR_PROCESO;
	}else if(string_equals_ignore_case(cadena, "DETENER_PLANIFICACION")){
		comando->opcode = DETENER_PLANIFICACION;
	}else if(string_equals_ignore_case(cadena, "INICIAR_PLANIFICACION")){
		comando->opcode = INICIAR_PLANIFICACION;
	}else if(string_equals_ignore_case(cadena, "MULTIPROGRAMACION")){
		comando->opcode = MULTIPROGRAMACION;
	}else if(string_equals_ignore_case(cadena, "PROCESO_ESTADO")){
		comando->opcode = PROCESO_ESTADO;
	}

	return comando;
}

void iniciar_proceso(t_comando* comando){
	//Armar PCB, colocarla en estado new y enviar el path a memoria
	t_pcb* pcb_proceso = malloc(sizeof(t_pcb));

	pcb_proceso->PID = rand() % 10000;
	pcb_proceso->program_counter = 1;
	pcb_proceso->proceso_estado = "NEW";
	pcb_proceso->tiempo_llegada_ready = 0;
//	pcb_proceso->prioridad = (int)comando->parametros[3];  //TODO verificar que el parametro ingresado sea un numero
	pcb_proceso->tabla_archivos_abiertos_del_proceso = NULL;

	//este malloc para evitar el segmentation fault en el envio del contexto de ejecución a cpu
	pcb_proceso->registros_CPU = malloc(sizeof(registros_CPU));

/*	strcpy(pcb_proceso->registros_CPU->AX, "");
	strcpy(pcb_proceso->registros_CPU->BX, "");
	strcpy(pcb_proceso->registros_CPU->CX, "");
	strcpy(pcb_proceso->registros_CPU->DX, "");

	agregar_cola_new(pcb_proceso);*/

	//envio a memoria de instrucciones
	t_paquete* paquete = crear_paquete(INICIAR_PROCESO);
	agregar_a_paquete(paquete, comando, sizeof(t_comando));

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

