#include "peticiones_kernel.h"

t_fcb* iniciar_fcb(t_config* config){

	// si no existe la config no hace nada
	if(config == NULL){
		log_error(logger, "El archivo ligado a la FCB que intentas abrir es erroneo o no existe");
		return NULL;
	}

	t_fcb* fcb = malloc(sizeof(t_fcb));

	fcb->nombre_archivo = config_get_string_value(config, "NOMBRE_ARCHIVO");
	fcb->tamanio_archivo = config_get_int_value(config, "TAMANIO_ARCHIVO");
	fcb->bloque_inicial = config_get_int_value(config, "BLOQUE_INICIAL");


	if(fcb->nombre_archivo==NULL ){
		log_error(logger, "El archivo ligado a la FCB que intentas abrir es erroneo o no existe");
		return NULL;
	}


	return fcb;
}


void abrir_archivo(uint64_t cliente_fd){

	//VARIABLES Y DATOS PARA LA FUNCION
	t_instruccion* instruccion = recibir_instruccion(cliente_fd);

	char* nombre_archivo = strdup(instruccion->parametros[0]);

	log_info(logger, "Abrir Archivo: %s", nombre_archivo);

	char* direccion_fcb = string_new();
	string_append(&direccion_fcb, path_fcb);
	string_append(&direccion_fcb, "/");
	string_append(&direccion_fcb, nombre_archivo);

	//CASO 1: La FCB esta en el diccionario (en memoria) => el archivo existe
	if(dictionary_has_key(fcb_por_archivo, nombre_archivo)){

		enviar_mensaje("OK", cliente_fd, ABRIR_ARCHIVO);

	//CASO 2: La FCB no esta en memoria pero si esta en el sistema
	}else {
		t_config* config_FCB = config_create(direccion_fcb);

		//CASO A: La FCB existe => el archivo tambien
		if(config_FCB != NULL){
			//Levanto la fcb
			t_fcb* fcb = malloc(sizeof(t_fcb));
			fcb = iniciar_fcb(config_FCB);


			config_destroy(config_FCB);

			dictionary_put(fcb_por_archivo, nombre_archivo, fcb);

			enviar_mensaje("OK", cliente_fd, ABRIR_ARCHIVO);


		//CASO B: La FCB no existe => el archivo tampoco
		}else{

			log_info(logger, "El archivo %s no se encontro ", nombre_archivo);
			//Doy aviso a Kernel
			enviar_mensaje("-1", cliente_fd, MENSAJE);

		}

	}

}


t_fcb* crear_fcb( t_instruccion* instruccion, char* path){
	t_config* config = malloc(sizeof(t_config));

	config->path = strdup(path);
	config->properties = dictionary_create();

	config_set_value(config, "NOMBRE_ARCHIVO", string_duplicate(instruccion->parametros[0]));
	config_set_value(config, "TAMANIO_ARCHIVO", string_itoa(0));
	config_set_value(config, "BLOQUE_INICIAL", "");

	config_save_in_file(config, path);

	t_fcb* fcb = malloc(sizeof(t_fcb));

	fcb->nombre_archivo = string_duplicate(config_get_string_value(config, "NOMBRE_ARCHIVO"));
	fcb->tamanio_archivo = config_get_int_value(config, "TAMANIO_ARCHIVO");
	fcb->bloque_inicial = config_get_int_value(config, "BLOQUE_INICIAL");

	config_destroy(config);
	return fcb;
}


void crear_archivo(uint64_t cliente_fd){
	//Cargamos las estructuras necesarias
	t_instruccion* instruccion = recibir_instruccion(cliente_fd);

	char* nombre_archivo = strdup(instruccion->parametros[0]);

	log_info(logger, "Crear Archivo: %s", nombre_archivo);

	char* direccion_fcb = string_new();
	string_append(&direccion_fcb, path_fcb);
	string_append(&direccion_fcb, "/");
	string_append(&direccion_fcb, nombre_archivo);

	log_info(logger, "Guardando el fcb del archivo %s en %s", nombre_archivo, direccion_fcb);

	//creamos la fcb
	t_fcb* fcb = malloc(sizeof(t_fcb));
	fcb = crear_fcb(instruccion, direccion_fcb);

	//Cargo estructuras resantes
	dictionary_put(fcb_por_archivo, nombre_archivo, fcb);
	enviar_mensaje("OK", cliente_fd, CREAR_ARCHIVO);
}



//dado un tamanio en bytes, calcula cuantos bloques son
int calcular_cantidad_de_bloques(int tamanio_en_bytes){
	float numero_bloques_nuevo_aux = tamanio_en_bytes/(float)tam_bloque;

	double parte_fraccional, numero_bloques_nuevo;
	parte_fraccional = modf(numero_bloques_nuevo_aux, &numero_bloques_nuevo);


	if( parte_fraccional != 0)
		numero_bloques_nuevo=numero_bloques_nuevo+1;

	return (int)numero_bloques_nuevo;
}

void agregar_bloques(t_fcb* fcb_a_actualizar, int bloques_a_agregar){

	//Asigno a posicion_fat el bloque incial y recorro la fat hasta encontrar el final (UINT32_MAX)
	uint32_t posicion_fat = fcb_a_actualizar->bloque_inicial;
	while(bits_fat[posicion_fat] != UINT32_MAX){
		log_info(logger, "Acceso a FAT - Entrada: %d - Valor: %d", posicion_fat, bits_fat[posicion_fat]);
			posicion_fat = bits_fat[posicion_fat];
		}

	//Creo un auxiliar para contar los bloques que me quedan por agregar y los voy agregando
	int bloques_restantes_por_agregar = bloques_a_agregar;
	uint32_t aux_busqueda = 1; //Voy a utilizar este auxiliar para recorrer la fat y encontrar los bloques libres

	//Inicio el bucle para agregar bloques
	while(bloques_restantes_por_agregar != 0){

		//Chequeo que no sea el ultimo bloque, si es el ultimo cargo el UINT32_MAX, sino asigno un bloque libre
		if(bloques_restantes_por_agregar == 0){
			log_info(logger, "Acceso a FAT - Entrada: %d - Valor: %d", posicion_fat, bits_fat[posicion_fat]);
			bits_fat[posicion_fat] = UINT32_MAX;
		}else{

		while(bits_fat[aux_busqueda] != 0){ //Bucle auxiliar para encontrar un bloque libre
			log_info(logger, "Acceso a FAT buscando bloque libre: - Entrada: %d - Valor: %d", posicion_fat, bits_fat[posicion_fat]);
			aux_busqueda ++;
		}
	//Encontrado el bloque libre lo asigno en la fat (donde antes estaba el uint32_max )
		log_info(logger, "Acceso a FAT - Entrada: %d - Valor: %d", posicion_fat, bits_fat[posicion_fat]);
		bits_fat[posicion_fat] = aux_busqueda;
		bloques_restantes_por_agregar --;
		}


	}

}

void sacar_bloques(t_fcb* fcb_a_actualizar, int bloques_a_sacar, int bloques_actual){

	//Creo un bucle que se repite la cantidad de veces que tenga que eliminar bloques
	while(bloques_a_sacar != 0){

		//Al ingresar al bucle inicializo una variable auxiliar para buscar el final del archivo y una con la posicion inicial de la fat;
		int aux_busca_final = 1;
		uint32_t posicion_fat = fcb_a_actualizar->bloque_inicial;

		//Creo un bucle auxiliar que se repetira avanzando la posicion de la fat hasta llegar al ultimo bloque
		while(aux_busca_final != bloques_actual){
			log_info(logger, "Acceso a FAT - Entrada: %d - Valor: %d", posicion_fat, bits_fat[posicion_fat]);
			posicion_fat = bits_fat[posicion_fat];
		}

		//Si el bloque a quitar es el ultimo (Luego de esto el truncado esta completo) le cargo el EOF
		if(bloques_a_sacar == 1){
			log_info(logger, "Acceso a FAT - Entrada: %d - Valor: %d", posicion_fat, bits_fat[posicion_fat]);
			bits_fat[posicion_fat] = UINT32_MAX;
		}else{
		//Si aun debo seguir truncando libero la posicion de la fat y el bloque de datos
			log_info(logger, "Acceso a FAT - Entrada: %d - Valor: %d", posicion_fat, bits_fat[posicion_fat]);
			bits_fat[posicion_fat] = 0;
			// Creo que esto | no es necesario, por las dudas lo comento
			//				 v
			//uint32_t cero =0;
			//memcpy(array_bloques[posicion_fat], &cero, tam_bloque)
		}

		//Decremento las variables de bloques a sacar  bloques actuales para ir actualizando los bucles
		bloques_a_sacar --;
		bloques_actual --;
		}


}

void truncar_archivo(uint64_t cliente_fd){

		t_instruccion* instruccion_peticion = (t_instruccion*) recibir_instruccion(cliente_fd);

		char* nombre_archivo = strdup(instruccion_peticion->parametros[0]);
		int nuevo_tamano_archivo = atoi(instruccion_peticion->parametros[1]);


		log_info(logger, "Truncar Archivo: %s - Tamaño: %d", nombre_archivo, nuevo_tamano_archivo);
		t_fcb* fcb_a_truncar = dictionary_get(fcb_por_archivo, nombre_archivo);



		  //calculo de cantidad de bloques a truncar
	    int numero_de_bloques_nuevo = calcular_cantidad_de_bloques(nuevo_tamano_archivo);

	    //calculo de cantidad de bloques actuales (antes del truncado :D)
	    int numero_de_bloques_actual = calcular_cantidad_de_bloques(fcb_a_truncar->tamanio_archivo);




		log_info(logger, "Bloques actual %d vs Bloque nuevo %d", numero_de_bloques_actual, numero_de_bloques_nuevo);

	    if(numero_de_bloques_nuevo > numero_de_bloques_actual){
		   //ampliar tamanio
	    	int bloques_a_agregar = numero_de_bloques_nuevo - numero_de_bloques_actual;

	    	agregar_bloques(fcb_a_truncar, bloques_a_agregar);


	    } else if(numero_de_bloques_nuevo < numero_de_bloques_actual) {
		   //reducir tamanio

		  int bloques_a_sacar = numero_de_bloques_actual - numero_de_bloques_nuevo;
		  sacar_bloques(fcb_a_truncar, bloques_a_sacar, numero_de_bloques_actual);

	   }

	    // actualizo tamanio_fcb de memoria y de el archivo config
	   fcb_a_truncar->tamanio_archivo = nuevo_tamano_archivo;



	 	char* direccion_fcb = string_new();
		string_append(&direccion_fcb, path_fcb);
		string_append(&direccion_fcb, "/");
		string_append(&direccion_fcb, fcb_a_truncar->nombre_archivo);

	   	t_config* archivo_fcb = config_create(direccion_fcb);

		if(archivo_fcb == NULL){
			log_error(logger, "No se encontro el fcb del archivo %s", fcb_a_truncar->nombre_archivo);
			enviar_mensaje("ERROR", cliente_fd, TRUNCAR_ARCHIVO);
			return;

		}


		config_set_value(archivo_fcb, "TAMANIO_ARCHIVO", string_itoa(fcb_a_truncar->tamanio_archivo));
		config_set_value(archivo_fcb, "BLOQUE_INICIAL", string_itoa(fcb_a_truncar->bloque_inicial));

		config_save(archivo_fcb );


	   // respondo a kernel un OK
		enviar_mensaje("OK", cliente_fd, TRUNCAR_ARCHIVO);

		free(nombre_archivo);
}

void deserializar_instruccion_con_dos_parametros_y_puntero(void* buffer, t_instruccion* instruccion, int *puntero, int *desplazamiento){
	memcpy(&(instruccion->opcode_lenght), buffer + *desplazamiento, sizeof(int));
	*desplazamiento+=sizeof(int);
	instruccion->opcode = malloc(instruccion->opcode_lenght);
	memcpy(instruccion->opcode, buffer+*desplazamiento, instruccion->opcode_lenght);
	*desplazamiento+=instruccion->opcode_lenght;

	memcpy(&(instruccion->parametro1_lenght), buffer+*desplazamiento, sizeof(int));
	*desplazamiento+=sizeof(int);
	instruccion->parametros[0] = malloc(instruccion->parametro1_lenght);
	memcpy(instruccion->parametros[0], buffer + *desplazamiento, instruccion->parametro1_lenght);
	*desplazamiento += instruccion->parametro1_lenght;

	memcpy(&(instruccion->parametro2_lenght), buffer+*desplazamiento, sizeof(int));
	*desplazamiento+=sizeof(int);
	instruccion->parametros[1] = malloc(instruccion->parametro2_lenght);
	memcpy(instruccion->parametros[1], buffer + *desplazamiento, instruccion->parametro2_lenght);
	*desplazamiento += instruccion->parametro2_lenght;

	memcpy(&(puntero), buffer+*desplazamiento, sizeof(int));
	*desplazamiento+=sizeof(int);


	 //esta linea es para el destroy de la instruccion
	 instruccion->parametro3_lenght = 0;
}



void escribir_archivo_fs(int cliente_fd){

	int size;
	void * buffer = recibir_buffer(&size ,cliente_fd);
	int *desplazamiento  = malloc(sizeof(int));
	int *puntero = malloc(sizeof(int));
	t_instruccion* instruccion = malloc(sizeof(t_instruccion));

	deserializar_instruccion_con_dos_parametros_y_puntero(buffer, instruccion, puntero, desplazamiento);
	t_fcb *fcb = dictionary_get(fcb_por_archivo, instruccion->parametros[0]);

	//TODO solicitarle el contenido a escribir a memoria y asignarlo abajo

	char* contenido_a_escribir = malloc(tam_bloque);

	if(dictionary_has_key(fcb_por_archivo, instruccion->parametros[0])){

		int bloque_a_escribir = *puntero/tam_bloque;
		int i = 0;
		int aux_busqueda_fat = fcb->bloque_inicial;

		while(i < bloque_a_escribir){
			log_info(logger, "Acceso a FAT - Entrada: %d - Valor: %d", aux_busqueda_fat, bits_fat[aux_busqueda_fat]);
			aux_busqueda_fat = bits_fat[aux_busqueda_fat];
			i++;
		}
		log_info(logger, "Acceso a bloque - Archivo: %s - Bloque archivo: %d - Bloque FS: %d", instruccion->parametros[0], bloque_a_escribir, aux_busqueda_fat);
		array_bloques[aux_busqueda_fat] = contenido_a_escribir;

		log_info(logger, "Archivo escrito: %s - Puntero: %d - Memoria: %s", instruccion->parametros[0], *puntero, instruccion->parametros[1]);

		enviar_mensaje("OK", cliente_fd, ESCRIBIR_ARCHIVO);


	}else{
		log_error(logger, "El archivo no se encuentra en el FS");
	}


	free(buffer);
	free(puntero);
	instruccion_destroy(instruccion);
}


void leer_archivo_fs(int cliente_fd){
	/*Esta operación leerá la información correspondiente al bloque a partir del puntero.
La información se deberá enviar al módulo Memoria para ser escrita a partir de la dirección física
recibida por parámetro, una vez recibida la confirmación por parte del módulo Memoria, se informará al
 módulo Kernel del éxito de la operación.*/
	int size;
	void * buffer = recibir_buffer(&size ,cliente_fd);
	int *desplazamiento  = malloc(sizeof(int));
	int *puntero = malloc(sizeof(int));
	t_instruccion* instruccion = malloc(sizeof(t_instruccion));

	deserializar_instruccion_con_dos_parametros_y_puntero(buffer, instruccion, puntero, desplazamiento);
	t_fcb *fcb = dictionary_get(fcb_por_archivo, instruccion->parametros[0]);

	if(dictionary_has_key(fcb_por_archivo, instruccion->parametros[0])){

		int bloque_a_leer = *puntero/tam_bloque;
		int i = 0;
		int aux_busqueda_fat = fcb->bloque_inicial;

		while(i < bloque_a_leer){
			log_info(logger, "Acceso a FAT - Entrada: %d - Valor: %d", aux_busqueda_fat, bits_fat[aux_busqueda_fat]);
			aux_busqueda_fat = bits_fat[aux_busqueda_fat];
			i++;
		}

		log_info(logger, "Acceso a bloque - Archivo: %s - Bloque archivo: %d - Bloque FS: %d ", instruccion->parametros[0], bloque_a_leer, aux_busqueda_fat);
		void *contenido_leido = array_bloques[aux_busqueda_fat];

		t_paquete *paquete = crear_paquete(LEER_ARCHIVO);
		agregar_a_paquete_sin_agregar_tamanio(paquete, contenido_leido, tam_bloque);
		enviar_paquete(paquete, cliente_fd); //TODO buscar la forma de enviarle esto a memoria

		eliminar_paquete(paquete);

		log_info(logger, "Archivo leido: %s - Puntero: %d - Memoria: %s", instruccion->parametros[0], *puntero, instruccion->parametros[1]);


	}else{
		log_error(logger, "El archivo no se encuentra en el FS");
	}


	free(buffer);
	free(puntero);
	instruccion_destroy(instruccion);
}


