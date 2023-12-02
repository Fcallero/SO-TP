#include "peticiones_fs.h"

bool existe_lock_escritura_para(char* nombre_archivo){
	t_tabla_global_de_archivos_abiertos* tabla = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);
	return  tabla->lock_de_archivo != NULL &&  tabla->lock_de_archivo->write_lock_count > 0;
}

bool existe_lock_lectura_para(char* nombre_archivo){
	t_tabla_global_de_archivos_abiertos* tabla = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);
	return  tabla->lock_de_archivo != NULL &&  tabla->lock_de_archivo->read_lock_count > 0;
}

void agregar_a_lock_escritura_para_archivo(char* nombre_archivo, t_pcb* proceso){
	t_tabla_global_de_archivos_abiertos* tabla = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);
	if(tabla->lock_de_archivo != NULL){
		tabla->lock_de_archivo->write_lock_count ++;
		queue_push(tabla->lock_de_archivo->cola_locks, proceso);
	} else {
		log_error(logger, "el lock del archivo en tabla global de archivos abiertos no existe, esto es imposible aca");
	}
}

void agregar_a_lock_lectura_para_archivo(char* nombre_archivo, t_pcb* proceso){
	t_tabla_global_de_archivos_abiertos* tabla = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);
	if(tabla->lock_de_archivo != NULL){
		tabla->lock_de_archivo->read_lock_count ++;
		queue_push(tabla->lock_de_archivo->cola_locks, proceso);
	} else {
		log_error(logger, "el lock del archivo en tabla global de archivos abiertos no existe, esto es imposible aca");
	}
}

void agregar_como_participante_a_lock_lectura_para_archivo(char*nombre_archivo, t_pcb* proceso){
	t_tabla_global_de_archivos_abiertos* tabla = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);
	if(tabla->lock_de_archivo != NULL){
		tabla->lock_de_archivo->read_lock_count ++;
		list_add(tabla->lock_de_archivo->lista_locks_read, proceso);
	} else {
		log_error(logger, "el lock del archivo en tabla global de archivos abiertos no existe, esto es imposible aca");
	}
}

void crear_lock_escritura_para(char* nombre_archivo, t_pcb* proceso){
	t_tabla_global_de_archivos_abiertos* tabla = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);
	if(tabla->lock_de_archivo != NULL){
		tabla->lock_de_archivo->write_lock_count ++;
		tabla->lock_de_archivo->proceso_write_lock = proceso;
	} else {
		log_error(logger, "el lock del archivo en la tabla global de archivos abiertos no existe, esto es imposible aca");
	}

}
//NO SE USA!??
void actualizar_lock_escritura_para_archivo(char *nombre_archivo, t_pcb* proceso){
	t_tabla_global_de_archivos_abiertos* tabla = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);
	if(tabla->lock_de_archivo == NULL ){
		t_file_lock *lock = malloc(sizeof(t_file_lock));
		lock->cola_locks = queue_create();
		lock->lista_locks_read = list_create();
		lock->read_lock_count = 0;
		lock->write_lock_count = 1;
		lock->proceso_write_lock = proceso;

		tabla->lock_de_archivo = lock;
	}else {
		log_error(logger, "el tabla global de archivos abiertos no existe, esto es imposible aca");
	}
}


void agregar_a_array_instancias_recursos(t_list* array_instancias, int instancias_en_posesion, char* nombre_archivo){

	 bool bucar_archivo(void* args){
		 t_recurso* recurso_n = (t_recurso*)args;

		 return strcmp(recurso_n->nombre_recurso, nombre_archivo) == 0;
	 }

	t_recurso* recurso_buscado = list_find(array_instancias, bucar_archivo);

	if(recurso_buscado == NULL){
		t_recurso *archivo_nuevo = recurso_new(nombre_archivo);
		archivo_nuevo->instancias_en_posesion=instancias_en_posesion;
		list_add(array_instancias, archivo_nuevo);
	}
}



void agregar_recurso_a_matriz(char *nombre_archivo, int pid, t_dictionary** matriz){
	t_list *recursos_a_devolver =  dictionary_get(*matriz, string_itoa(pid));

	int cant_instancias_total_para_cada_archivo = 1;//los archivos se manejan como recursos de una unica instancia

	if(recursos_a_devolver == NULL){
		recursos_a_devolver = list_create();

		for(int i = 0; i<cant_recursos; i++){
			t_recurso * recurso_n = recurso_new(recursos[i]);

			list_add(recursos_a_devolver, recurso_n);
		}

		t_recurso * recurso_n = recurso_new(nombre_archivo);

		list_add(recursos_a_devolver, recurso_n);


		agregar_a_array_instancias_recursos(recursos_totales, cant_instancias_total_para_cada_archivo, nombre_archivo);


		dictionary_put(*matriz,string_itoa(pid), recursos_a_devolver);
	} else {

		bool es_el_archivo(void*args){
			t_recurso* recurso_n = (t_recurso*)args;

			return strcmp(nombre_archivo, recurso_n->nombre_recurso) == 0;
		}

		if(!list_any_satisfy(recursos_a_devolver, es_el_archivo)){
			t_recurso * recurso_n = recurso_new(nombre_archivo);

			list_add(recursos_a_devolver, recurso_n);

			agregar_a_array_instancias_recursos(recursos_totales, cant_instancias_total_para_cada_archivo, nombre_archivo);
		}
	}
}

void agregar_recurso_a_matrices(char *nombre_archivo, int pid){
	agregar_recurso_a_matriz(nombre_archivo, pid, &matriz_recursos_pendientes);
	agregar_recurso_a_matriz(nombre_archivo, pid, &matriz_recursos_asignados);
}

void enviar_instruccion(t_instruccion* instruccion, int socket_a_enviar, int opcode){
	t_paquete* paquete = crear_paquete(opcode);
	agregar_a_paquete(paquete, instruccion->opcode,instruccion->opcode_lenght);
	agregar_a_paquete(paquete, instruccion->parametros[0],instruccion->parametro1_lenght);
	agregar_a_paquete(paquete, instruccion->parametros[1],instruccion->parametro2_lenght);
	agregar_a_paquete(paquete, instruccion->parametros[2],instruccion->parametro3_lenght);
	enviar_paquete(paquete, socket_a_enviar);
	eliminar_paquete(paquete);
}

void crear_entrada_tabla_global_archivos_abiertos(char *nombre_archivo){
	t_tabla_global_de_archivos_abiertos *tabla_archivo_abierto_global = malloc(sizeof(t_tabla_global_de_archivos_abiertos));

	tabla_archivo_abierto_global->file = strdup(nombre_archivo);
	tabla_archivo_abierto_global->open = 1;

	t_file_lock* lock_archivo = malloc(sizeof(t_file_lock));
	lock_archivo->lista_locks_read = list_create();
	lock_archivo->cola_locks = queue_create();
	lock_archivo->read_lock_count = 0;
	lock_archivo->write_lock_count = 0;
	lock_archivo->proceso_write_lock = NULL;

	tabla_archivo_abierto_global->lock_de_archivo = lock_archivo ;

	dictionary_put(tabla_global_de_archivos_abiertos, nombre_archivo, tabla_archivo_abierto_global);
}

void bloquear_por_espera_a_fs(t_pcb* proceso_a_bloquear, char*nombre_archivo){
	//Si no existe el archivo en el diccionario, lo creo.
	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_a_bloquear->PID, "EXEC","BLOC");
	log_info(logger, "PID: %d - Bloqueado por: %s", proceso_a_bloquear->PID, nombre_archivo);

	if(!dictionary_has_key(colas_de_procesos_bloqueados_para_cada_archivo, nombre_archivo)){
		t_queue* cola_bloqueados = queue_create();
		queue_push(cola_bloqueados, proceso_a_bloquear);

		dictionary_put(colas_de_procesos_bloqueados_para_cada_archivo, nombre_archivo, cola_bloqueados);

		// Si existe agrego el elemento a la cola
	}else{
		//Creo una cola, le asigno la cola del diccionario y la remuevo
		t_queue* cola_bloqueados = dictionary_get(colas_de_procesos_bloqueados_para_cada_archivo, nombre_archivo);
		//cargo el proceso a bloquear en la cola

		queue_push(cola_bloqueados, proceso_a_bloquear);
	}

}

void desbloquear_por_espera_a_fs(int pid, char*nombre_archivo){
	if(dictionary_has_key(colas_de_procesos_bloqueados_para_cada_archivo, nombre_archivo)){
		t_queue* cola_bloqueados = dictionary_get(colas_de_procesos_bloqueados_para_cada_archivo, nombre_archivo);

		bool pid_encontrado = false;
		t_pcb* pcb_a_desbloquear;
		//esto lo hago asi para no tener que cambiar en donde se usa esta cola,
		// pero lo mas conveniente seria usar otro diccionario dentro del diccionario de procesos_bloqueados
		// donde se mapea el pcb con el pid, para encontrarlo mas rápido
		while(!pid_encontrado){
			pcb_a_desbloquear = queue_pop(cola_bloqueados);

			if(pcb_a_desbloquear->PID == pid){
				pid_encontrado = true;
			}else {
				queue_push(cola_bloqueados, pcb_a_desbloquear);
			}
		}
		log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pid, "BLOC","READY");

		pasar_a_ready(pcb_a_desbloquear);
	}
}

void manejar_lock_escritura(char *nombre_archivo, t_contexto_ejec* contexto, int socket_cliente){

	if(existe_lock_escritura_para(nombre_archivo)){

		deteccion_de_deadlock();

		sem_wait(&m_proceso_ejecutando);
		agregar_a_lock_escritura_para_archivo(nombre_archivo, proceso_ejecutando);
		bloquear_por_espera_a_fs(proceso_ejecutando, nombre_archivo);//o mantenerlo bloqueado
		sem_post(&m_proceso_ejecutando);

		poner_a_ejecutar_otro_proceso();
	} else {
		sem_wait(&m_proceso_ejecutando);
		crear_lock_escritura_para(nombre_archivo, proceso_ejecutando);
		sem_post(&m_proceso_ejecutando);

		decrementar_recurso_en_matriz(&matriz_recursos_pendientes, nombre_archivo, string_itoa(contexto->pid), 0);
		incrementar_recurso_en_matriz(&matriz_recursos_asignados, nombre_archivo, string_itoa(contexto->pid), 0);

		enviar_contexto_de_ejecucion_a(contexto, PETICION_CPU, socket_cliente);
	}
}

void manejar_lock_lectura(char *nombre_archivo, t_contexto_ejec* contexto, int socket_cliente){

	if(existe_lock_escritura_para(nombre_archivo)){

		deteccion_de_deadlock();

		sem_wait(&m_proceso_ejecutando);
		agregar_a_lock_lectura_para_archivo(nombre_archivo, proceso_ejecutando);
		bloquear_por_espera_a_fs(proceso_ejecutando, nombre_archivo);//o mantenerlo bloqueado
		sem_post(&m_proceso_ejecutando);

		poner_a_ejecutar_otro_proceso();
	}else {//si existe un lock lectura o no
		sem_wait(&m_proceso_ejecutando);
		agregar_como_participante_a_lock_lectura_para_archivo(nombre_archivo, proceso_ejecutando);
		sem_post(&m_proceso_ejecutando);

		decrementar_recurso_en_matriz(&matriz_recursos_pendientes, nombre_archivo, string_itoa(contexto->pid), 0);
		//no es necesario incrementar como recurso asignado a este proceso porque estan en modo lectura

		enviar_contexto_de_ejecucion_a(contexto, PETICION_CPU, socket_cliente);
	}

}


void actualizar_archivos_abiertos_por_proceso(t_pcb* proceso, char* nombre_archivo, char* modo_apertura){
	if(proceso->tabla_archivos_abiertos_del_proceso == NULL){
		proceso->tabla_archivos_abiertos_del_proceso = list_create();
	}

	t_tabla_de_archivos_por_proceso* entrada_archivo_abierto_proceso = malloc(sizeof(t_tabla_de_archivos_por_proceso));
	entrada_archivo_abierto_proceso->nombre_archivo = strdup(nombre_archivo);
	entrada_archivo_abierto_proceso->modo_apertura = strdup(modo_apertura);
	entrada_archivo_abierto_proceso->puntero_posicion=0;

	list_add(proceso->tabla_archivos_abiertos_del_proceso, entrada_archivo_abierto_proceso);
}

void actualizar_pcb(t_contexto_ejec* contexto){

	sem_wait(&m_proceso_ejecutando);
	proceso_ejecutando->program_counter = contexto->program_counter;

	if(proceso_ejecutando->registros_CPU == NULL){
		proceso_ejecutando->registros_CPU = contexto->registros_CPU;
	} else {
		proceso_ejecutando->registros_CPU->AX = contexto->registros_CPU->AX;
		proceso_ejecutando->registros_CPU->BX = contexto->registros_CPU->BX;
		proceso_ejecutando->registros_CPU->CX = contexto->registros_CPU->CX;
		proceso_ejecutando->registros_CPU->DX = contexto->registros_CPU->DX;
	}
	sem_post(&m_proceso_ejecutando);

}

void enviar_a_fs_crear_o_abrir_archivo (int socket_cpu, int socket_filesystem){
	t_contexto_ejec* contexto = recibir_contexto_de_ejecucion(socket_cpu);

	actualizar_pcb(contexto);

	char* nombre_archivo = strdup(contexto->instruccion->parametros[0]);
	agregar_recurso_a_matrices(nombre_archivo, contexto->pid);

	log_info(logger,"PID %d - ABRIR ARCHIVO: %s",contexto->pid, nombre_archivo);

	char *modo_apertura =  strdup(contexto->instruccion->parametros[1]);
	sem_wait(&m_proceso_ejecutando);
	actualizar_archivos_abiertos_por_proceso(proceso_ejecutando, nombre_archivo, modo_apertura);
	sem_post(&m_proceso_ejecutando);

	enviar_instruccion(contexto->instruccion, socket_filesystem, ABRIR_ARCHIVO);

	//esperamos una respuesata del fs:
	//	SI EXISTE, MANDARA EL TAM DEL ARCHIVO;
	//	SI NO EXISTE, MANDARA UN -1 Y MANDAREMOS ORDEN DE CREAR EL ARCHIVO

	int opcode = recibir_operacion(socket_filesystem);

	if(opcode == MENSAJE){
		//si el archivo no existe
		if(atoi(recibir_mensaje(socket_filesystem))==-1){
			//mandamos la orden
			enviar_instruccion(contexto->instruccion, socket_filesystem, CREAR_ARCHIVO);

			//epero la respuesta de la creacion del archivo
			int opcode = recibir_operacion(socket_filesystem);

			if(opcode != CREAR_ARCHIVO){
				log_error(logger, "No se pudo crear el archivo, hubo un error");
				return;
			}

			char *mensaje = recibir_mensaje(socket_filesystem);
			log_info(logger, "Se recibio %s de Filesystem, arhivo creado exitosamente", mensaje);
			free(mensaje);

			crear_entrada_tabla_global_archivos_abiertos(nombre_archivo);
		} else {
			log_error(logger, "Hubo un error al recibir de FS si el archivo %s existe o no", nombre_archivo);
		}
	// si existe el archivo
	} else if(opcode == ABRIR_ARCHIVO){
		char* mensaje = recibir_mensaje(socket_filesystem);
		log_info(logger, "se reicibo un %s de FS, archivo abierto correctamente", mensaje);
		free(mensaje);

		t_tabla_global_de_archivos_abiertos *tabla_global = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);

		if(tabla_global != NULL){
			tabla_global->open ++;
		} else {
			crear_entrada_tabla_global_archivos_abiertos(nombre_archivo);
		}
	}

	//para el algoritmo de deteccion, segun corresponda
	incrementar_recurso_en_matriz(&matriz_recursos_pendientes, nombre_archivo, string_itoa(contexto->pid), 0);

	//maneja los locks

	//MODO ESCRITURA
	if (string_equals_ignore_case(modo_apertura, "W")) {
		manejar_lock_escritura(nombre_archivo, contexto, socket_cpu);
	//MODO LECTURA
	}else if(string_equals_ignore_case(modo_apertura, "R")){
		manejar_lock_lectura(nombre_archivo, contexto, socket_cpu);
	}

	//dependiendo del lock, va a seguir ejecutando el proceso o se va a bloquear y llama a otro proceso de ready

	contexto_ejecucion_destroy(contexto);
	return;
}

void enviar_a_fs_truncar_archivo(int socket_cpu, int socket_filesystem)
{
	t_contexto_ejec* contexto = recibir_contexto_de_ejecucion(socket_cpu);

	char* nombre_archivo = strdup(contexto->instruccion->parametros[0]);
	char* tamanio_archivo = strdup(contexto->instruccion->parametros[1]);

	actualizar_pcb(contexto);

	log_info(logger,"PID: %d - Archivo: %s - Tamaño: %s",contexto->pid, nombre_archivo, tamanio_archivo);

	//ya recibimos la instruccion con los parametros, ahora hay que mandarle a filesystem la orden
	enviar_instruccion(contexto->instruccion, socket_filesystem, TRUNCAR_ARCHIVO);

	//bloquear mientras espera a FS y desbloquear el proceso cuando termina FS.


	deteccion_de_deadlock();

	sem_wait(&m_proceso_ejecutando);
	bloquear_por_espera_a_fs(proceso_ejecutando, nombre_archivo);
	sem_post(&m_proceso_ejecutando);
	poner_a_ejecutar_otro_proceso();

	int opcode = recibir_operacion(socket_filesystem);

	if(opcode != TRUNCAR_ARCHIVO){
		log_error(logger, "No se pudo truncar el archivo, hubo un error");
		return ;
	}

	char *mensaje = recibir_mensaje(socket_filesystem);

	log_info(logger, "se reicibio un %s de FS, archivo truncado correctamente ", mensaje);

	desbloquear_por_espera_a_fs(contexto->pid, nombre_archivo);

	contexto_ejecucion_destroy(contexto);
	return;
}

t_tabla_de_archivos_por_proceso *obtener_entrada_archivos_abiertos_proceso(t_list*tabla_arch_abiertos_proceso, char *nombre_proceso){

	bool buscar_entrada(void* args){
		t_tabla_de_archivos_por_proceso* entrada_n = (t_tabla_de_archivos_por_proceso*) args;

		return strcmp(entrada_n->nombre_archivo, nombre_proceso) == 0;
	}

	return list_find(tabla_arch_abiertos_proceso, buscar_entrada);
}

void reposicionar_puntero(int cliente_fd){
	t_contexto_ejec* contexto = (t_contexto_ejec*) recibir_contexto_de_ejecucion(cliente_fd);
	t_instruccion* instruccion_peticion = contexto->instruccion;

	actualizar_pcb(contexto);

	char* nombre_archivo = strdup(instruccion_peticion->parametros[0]);
	int posicion = atoi(instruccion_peticion->parametros[1]);

	sem_wait(&m_proceso_ejecutando);
	t_tabla_de_archivos_por_proceso* entrada_tabla_arch_por_proceso = obtener_entrada_archivos_abiertos_proceso(proceso_ejecutando->tabla_archivos_abiertos_del_proceso, nombre_archivo);
	sem_post(&m_proceso_ejecutando);
	entrada_tabla_arch_por_proceso->puntero_posicion= posicion;

	log_info(logger, "PID: %d - Actualizar puntero Archivo: %s - Puntero %d", contexto->pid, nombre_archivo, posicion);

	// continua con el mismo proceso
	enviar_contexto_de_ejecucion_a(contexto, PETICION_CPU, cliente_fd);

	free(nombre_archivo);
	contexto_ejecucion_destroy(contexto);
}

void enviar_peticion_puntero_fs(op_code opcode, t_instruccion *instruccion, int puntero, int pid){
	t_paquete* paquete = crear_paquete(opcode);
	agregar_a_paquete_sin_agregar_tamanio(paquete, &pid, sizeof(int));
	agregar_a_paquete(paquete, instruccion->opcode,instruccion->opcode_lenght);
	agregar_a_paquete(paquete, instruccion->parametros[0],instruccion->parametro1_lenght);
	agregar_a_paquete(paquete, instruccion->parametros[1],instruccion->parametro2_lenght);
	agregar_a_paquete(paquete, instruccion->parametros[2],instruccion->parametro3_lenght);
	agregar_a_paquete_sin_agregar_tamanio(paquete, &puntero, sizeof(int));
	enviar_paquete(paquete, socket_fs);
	eliminar_paquete(paquete);
}

void leer_archivo(int socket_cpu){
	t_contexto_ejec* contexto = (t_contexto_ejec*) recibir_contexto_de_ejecucion(socket_cpu);
	t_instruccion* instruccion_peticion = contexto->instruccion;

	actualizar_pcb(contexto);

	char* nombre_archivo = strdup(instruccion_peticion->parametros[0]);
	sem_wait(&m_proceso_ejecutando);
	t_tabla_de_archivos_por_proceso *entrada_tabla_arch_abierto_proceso = obtener_entrada_archivos_abiertos_proceso(proceso_ejecutando->tabla_archivos_abiertos_del_proceso, nombre_archivo);
	sem_post(&m_proceso_ejecutando);
	int puntero = entrada_tabla_arch_abierto_proceso->puntero_posicion;

	char* direccion_fisica = strdup(instruccion_peticion->parametros[1]);
	char* bytes_a_leer_string = strdup(instruccion_peticion->parametros[2]);

	log_info(logger, "PID: %d - Leer Archivo: %s - Puntero %d - Dirección Memoria %s - Tamaño %s", contexto->pid, nombre_archivo, puntero, direccion_fisica, bytes_a_leer_string);

	free(direccion_fisica);
	free(bytes_a_leer_string);

	enviar_peticion_puntero_fs(LEER_ARCHIVO,instruccion_peticion, puntero, contexto->pid);

	deteccion_de_deadlock();

	sem_wait(&m_proceso_ejecutando);
	bloquear_por_espera_a_fs(proceso_ejecutando, nombre_archivo);
	sem_post(&m_proceso_ejecutando);

	poner_a_ejecutar_otro_proceso();

	op_code cod_op = recibir_operacion(socket_fs);

	if(cod_op == LEER_ARCHIVO){
		char* mensaje = recibir_mensaje(socket_fs);

		if(strcmp(mensaje,"OK") == 0){
			//desbloquear tras recibir "OK"
			desbloquear_por_espera_a_fs(contexto->pid, nombre_archivo);

			log_info(logger, "Se leyo correctamente el contenido del archivo");

		} else {
			log_error(logger, "No se pudo leer el archivo");
		}
	}
	free(nombre_archivo);
	contexto_ejecucion_destroy(contexto);
}

void finalizar_por_invalid_write_proceso_ejecutando(t_contexto_ejec** contexto){
	sem_wait(&m_proceso_ejecutando);
	actualizar_estado_a_pcb(proceso_ejecutando, "EXIT");

	t_paquete* paquete = crear_paquete(FINALIZAR_PROCESO_MEMORIA);
	agregar_a_paquete_sin_agregar_tamanio(paquete, &(proceso_ejecutando->PID), sizeof(int));
	enviar_paquete(paquete, socket_memoria);

	eliminar_paquete(paquete);

	log_info(logger, "Finaliza el proceso %d - Motivo: INVALID_WRITE", proceso_ejecutando->PID);
	log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", proceso_ejecutando->PID, "EXEC","EXIT");

	sem_wait(&m_cola_exit);
	queue_push(cola_exit, string_itoa(proceso_ejecutando->PID));
	sem_post(&m_cola_exit);
	sem_post(&m_proceso_ejecutando);


	contexto_ejecucion_destroy(*contexto);

	sem_wait(&m_proceso_ejecutando);
	pcb_args_destroy(proceso_ejecutando);
	sem_post(&m_proceso_ejecutando);
	poner_a_ejecutar_otro_proceso();
}

void escribir_archivo(int socket_cpu){
	t_contexto_ejec* contexto = (t_contexto_ejec*) recibir_contexto_de_ejecucion(socket_cpu);
	t_instruccion* instruccion_peticion = contexto->instruccion;

	actualizar_pcb(contexto);

	char* nombre_archivo = strdup(instruccion_peticion->parametros[0]);

	sem_wait(&m_proceso_ejecutando);
	t_tabla_de_archivos_por_proceso *entrada_tabla_arch_abierto_proceso = obtener_entrada_archivos_abiertos_proceso(proceso_ejecutando->tabla_archivos_abiertos_del_proceso, nombre_archivo);
	sem_post(&m_proceso_ejecutando);
	int puntero = entrada_tabla_arch_abierto_proceso->puntero_posicion;

	char* direccion_fisica = strdup(instruccion_peticion->parametros[1]);
	char* bytes_a_escribir_string = strdup(instruccion_peticion->parametros[2]);

	log_info(logger, "PID: %d - Escribir Archivo: %s - Puntero %d - Dirección Memoria %s - Tamaño %s", contexto->pid, nombre_archivo, puntero, direccion_fisica, bytes_a_escribir_string);

	free(bytes_a_escribir_string);
	free(direccion_fisica);

	if(strcmp(entrada_tabla_arch_abierto_proceso->modo_apertura, "R") == 0){
		finalizar_por_invalid_write_proceso_ejecutando(&contexto);
		return;
	}

	enviar_peticion_puntero_fs(ESCRIBIR_ARCHIVO,instruccion_peticion, puntero, contexto->pid);

	deteccion_de_deadlock();

	sem_wait(&m_proceso_ejecutando);
	bloquear_por_espera_a_fs(proceso_ejecutando, nombre_archivo);
	sem_post(&m_proceso_ejecutando);

	poner_a_ejecutar_otro_proceso();

	op_code cod_op = recibir_operacion(socket_fs);

	if(cod_op == ESCRIBIR_ARCHIVO){
		char* mensaje = recibir_mensaje(socket_fs);
		if(strcmp(mensaje,"OK") == 0){
			//desbloquear tras recibir "OK"
			desbloquear_por_espera_a_fs(contexto->pid, nombre_archivo);

			log_info(logger, "Se leyo correctamente el contenido del archivo");
			//continua con el mismo proceso
		} else {
			log_error(logger, "No se pudo escirbir el archivo");
		}
	}

	free(nombre_archivo);
	contexto_ejecucion_destroy(contexto);
}

void cerrar_archivo(int cliente_fd){
	t_contexto_ejec* contexto = (t_contexto_ejec*) recibir_contexto_de_ejecucion(cliente_fd);
	t_instruccion* instruccion_peticion = contexto->instruccion;

	actualizar_pcb(contexto);

	char* nombre_archivo = strdup(instruccion_peticion->parametros[0]);

	log_info(logger, "Cerrar Archivo: “PID: %d - Cerrar Archivo: %s”", contexto->pid, nombre_archivo);

	sem_wait(&m_proceso_ejecutando);
	t_tabla_de_archivos_por_proceso *entrada_tabla_arch_abierto_proceso = obtener_entrada_archivos_abiertos_proceso(proceso_ejecutando->tabla_archivos_abiertos_del_proceso, nombre_archivo);
	sem_post(&m_proceso_ejecutando);

	t_tabla_global_de_archivos_abiertos* tabla_global_archivo = dictionary_get(tabla_global_de_archivos_abiertos, nombre_archivo);

	if(strcmp(entrada_tabla_arch_abierto_proceso->modo_apertura, "R") == 0){
		tabla_global_archivo->lock_de_archivo->read_lock_count --;

		bool esElProceso(void* arg){
			t_pcb* proceso_n = (t_pcb*) arg;

			return proceso_n->PID == contexto->pid;
		}
		list_remove_by_condition(tabla_global_archivo->lock_de_archivo->lista_locks_read, esElProceso);

	} else {
		tabla_global_archivo->lock_de_archivo->write_lock_count --;

		if(queue_size(tabla_global_archivo->lock_de_archivo->cola_locks) > 0){
			t_pcb* proceso_a_desbloquear = queue_pop(tabla_global_archivo->lock_de_archivo->cola_locks);

			bool esElArchivo(void* args){
				t_tabla_de_archivos_por_proceso* entrada_n = (t_tabla_de_archivos_por_proceso*) args;

				return strcmp(entrada_n->nombre_archivo, nombre_archivo) == 0;
			}

			t_tabla_de_archivos_por_proceso* entrada_arch_abierto_proceso = list_find(proceso_a_desbloquear->tabla_archivos_abiertos_del_proceso, esElArchivo);

			if(entrada_arch_abierto_proceso != NULL && strcmp(entrada_arch_abierto_proceso->modo_apertura, "R") == 0){
				tabla_global_archivo->lock_de_archivo->proceso_write_lock = NULL;
				desbloquear_por_espera_a_fs(proceso_a_desbloquear->PID, nombre_archivo);

				t_pcb* proceso_posible_a_desbloquear;
				t_tabla_de_archivos_por_proceso* entrada_arch_abierto_proceso_n = entrada_arch_abierto_proceso;
				
				while(strcmp(entrada_arch_abierto_proceso_n->modo_apertura, "R") == 0 && queue_size(tabla_global_archivo->lock_de_archivo->cola_locks) > 0){
					//no lo saco para evaluar si es de lectura, si lo es hace el pop para sacarlo
					proceso_posible_a_desbloquear = queue_peek(tabla_global_archivo->lock_de_archivo->cola_locks);
					entrada_arch_abierto_proceso_n = list_find(proceso_posible_a_desbloquear->tabla_archivos_abiertos_del_proceso, esElArchivo);

					if(strcmp(entrada_arch_abierto_proceso_n->modo_apertura, "R") == 0){
						proceso_posible_a_desbloquear= queue_pop(tabla_global_archivo->lock_de_archivo->cola_locks);
						desbloquear_por_espera_a_fs(proceso_posible_a_desbloquear->PID, nombre_archivo);
					}
				}


			} else if(entrada_arch_abierto_proceso != NULL && strcmp(entrada_arch_abierto_proceso->modo_apertura, "W") == 0){
				tabla_global_archivo->lock_de_archivo->proceso_write_lock = proceso_a_desbloquear;
				desbloquear_por_espera_a_fs(proceso_a_desbloquear->PID, nombre_archivo);
			}

			
		}

	}
	tabla_global_archivo->open --;

	if(tabla_global_archivo->open == 0){
		list_destroy(tabla_global_archivo->lock_de_archivo->lista_locks_read);
		queue_destroy(tabla_global_archivo->lock_de_archivo->cola_locks);
		tabla_global_archivo->lock_de_archivo->proceso_write_lock = NULL;
		free(tabla_global_archivo->lock_de_archivo);

		free(tabla_global_archivo->file);
		free(tabla_global_archivo);
		
		dictionary_remove(tabla_global_de_archivos_abiertos, nombre_archivo);
	}

	enviar_contexto_de_ejecucion_a(contexto, PETICION_CPU, cliente_fd);

	contexto_ejecucion_destroy(contexto);
	free(nombre_archivo);
}

