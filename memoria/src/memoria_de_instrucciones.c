#include "memoria_de_instrucciones.h"
#include "memoria.h"

t_list* lista_instrucciones;

int leer_pseudo(int cliente_fd){

	//recibe el path de la instruccion enviado por kernel por "enviar_mensaje"
	char* archivo_path= recibir_mensaje(cliente_fd);

	FILE* archivo = fopen(archivo_path,"r");

	//comprobar si el archivo existe
	if(archivo == NULL){
			log_error(logger, "Error en la apertura del archivo.");
			return 1;
		}

	//declaro variables

	char* cadena;
	lista_instrucciones = list_create();

	//leo el pseudocodigo, pongo en la lista
	while(feof(archivo) == 0)
	{
		cadena = malloc(30);
		char *resultado_cadena = fgets(cadena, 30, archivo);

		//si el char esta vacio, hace break.
		if(resultado_cadena == NULL)
		{
			break;
		}

		// se interpreta que no tiene "/n"


		/*ya tengo la linea del codigo, ahora tengo que separarlos en opcode
		y los parametros. Esta dividido por un espacio*/

		//creo t_instruccion
		t_instruccion *ptr_inst = malloc(sizeof(t_instruccion));

		//leo el opcode
		char* token = strtok(resultado_cadena," ");
		ptr_inst->opcode = token;
		ptr_inst->opcode_lenght = string_length(ptr_inst->opcode);

		//leo parametros
		token = strtok(NULL, " ");
		int n = 0;
		while(token != NULL)
		{
			ptr_inst->parametros[n] = token;
			token = strtok(NULL, " ");
			n++;
		}
		ptr_inst->parametro1_lenght = string_length(ptr_inst->parametros[0]);
		ptr_inst->parametro2_lenght = string_length(ptr_inst->parametros[1]);
		ptr_inst->parametro3_lenght = string_length(ptr_inst->parametros[2]);

		//ya el t_instruccion esta completo, hay que agregarlo en la cola

		list_add(lista_instrucciones,ptr_inst);
	}
	return 0;
}

void enviar_instruccion_a_cpu(int cliente_cpu)
{
	//recibe el opcode del paquete de cpu
	int ip;
	op_code *opcode = recibir_operacion(cliente_cpu);
	if(opcode != INSTRUCCION)
	{
		log_error(logger, "Error de Operacion! codigo de operacion recibido: %d", opcode);
		return;
	}
	t_paquete *paquete = recibir_paquete(cliente_cpu);
	memcpy(ip,paquete->buffer,sizeof(int));

	//consigo la instruccion
	t_instruccion *instruccion = list_get(lista_instrucciones,ip);

	//Una vez obtenida la instruccion, creo el paquete y serializo
	t_paquete *paquete_instruccion = crear_paquete(INSTRUCCION);

	agregar_a_paquete(paquete_instruccion,instruccion->opcode,instruccion->opcode_lenght);
	agregar_a_paquete(paquete_instruccion,instruccion->parametros[0],instruccion->parametro1_lenght);
	agregar_a_paquete(paquete_instruccion,instruccion->parametros[1],instruccion->parametro2_lenght);
	agregar_a_paquete(paquete_instruccion,instruccion->parametros[2],instruccion->parametro3_lenght);

	//esperar_por(/*variable de retardo*/);
	//envio paquete
	enviar_paquete(paquete_instruccion,cliente_cpu);
	// libero memoria
	eliminar_paquete(paquete_instruccion);
}
