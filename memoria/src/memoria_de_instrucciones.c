#include "memoria_de_instrucciones.h"


t_list* instruccion;

int leer_pseudo(int cliente_fd){

	//recibe el path de la instruccion enviado por kernel por "enviar_mensaje"
	char archivo_path[]= recibir_mensaje(cliente_fd);

	FILE* archivo = fopen(archivo_path,"r");

	//comprobar si el archivo existe
	if(archivo == NULL){
			logger_error(logger, "Error en la apertura del archivo.");
			return 1;
		}

	//declaro variables

	char* cadena;
	t_list* lista_instrucciones = list_create();

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
