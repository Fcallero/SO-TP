#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <commons/collections/list.h>
#include <commons/temporal.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/dictionary.h>


extern t_log* logger;


typedef enum
{
	MENSAJE,
	HANDSHAKE,
	HANDSHAKE_TAM_MEMORIA,
	PAQUETE,
	// CPU
	CREAR_PROCESO,
	TERMINAR_PROCESO, //Libera la pcb, avisa a memoria y a consola
	FINALIZAR_PROCESO,
	BLOQUEAR_PROCESO,
	PETICION_KERNEL,
	APROPIAR_RECURSOS,
	DESALOJAR_RECURSOS,
	DESALOJAR_PROCESO,
	SLEEP,
	PROCESAR_INSTRUCCION,
	INTERRUPCION,
	PETICION_CPU,
	// memoria
	ACCESO_A_PAGINA,
	PAGE_FAULT,
	NUEVO_PROCESO_MEMORIA,
	FINALIZAR_PROCESO_MEMORIA,
	READ_MEMORY,
	WRITE_MEMORY,
	INSTRUCCION,
	TAMANO_PAGINA, //se usa?
	// filesystem
	NUEVO_PROCESO_FS,
	FINALIZAR_PROCESO_FS,
	ABRIR_ARCHIVO,
	CERRAR_ARCHIVO,
	APUNTAR_ARCHIVO,
	TRUNCAR_ARCHIVO,
	LEER_ARCHIVO,
	ESCRIBIR_ARCHIVO,
	CREAR_ARCHIVO,
	//consola kernel (FINALIZAR PROCESO REUTILIZA EL DE ARRIBA)
	INICIAR_PROCESO,
	DETENER_PLANIFICACION,
	INICIAR_PLANIFICACION,
	MULTIPROGRAMACION,
	PROCESO_ESTADO,
}op_code;


typedef struct {
	int opcode_lenght;
	char* opcode;
	int parametro1_lenght;
	int parametro2_lenght;
	int parametro3_lenght;
	char* parametros[3];

}t_instruccion;


// Registros de 4 bytes
typedef struct {
    uint32_t AX;
    uint32_t BX;
    uint32_t CX;
    uint32_t DX;
} registros_CPU;



typedef struct
{
	int pid;
	t_instruccion* instruccion;
	int program_counter;

	registros_CPU* registros_CPU;

} t_contexto_ejec;

typedef struct {
	char* nombre_archivo;
	int tamanio_archivo;
	int bloque_inicial;
} t_fcb;


typedef struct{
	int fileDescriptor;
	char* file;
	int open;
}t_tabla_global_de_archivos_abiertos;

typedef struct{
	char* nombre_archivo;
	int puntero_posicion; // se usa para el fseek
}t_tabla_de_archivos_por_proceso;

typedef struct
{
	int PID;
	int program_counter;
	char* proceso_estado; // pueden ser "NEW", "READY", "EXEC", "BLOCKED" y "EXIT"
	int64_t tiempo_llegada_ready;
	registros_CPU* registros_CPU;

	int prioridad;

	t_instruccion* comando;

	// lista de t_tabla_de_archivos_por_proceso
	t_list* tabla_archivos_abiertos_del_proceso;

} t_pcb;

#endif /* GLOBAL_H_ */
