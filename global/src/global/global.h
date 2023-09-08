#ifndef GLOBAL_H_
#define GLOBAL_H_

#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include<string.h>
#include <netdb.h>
#include<sys/socket.h>
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
	PAQUETE,
	// CPU
	INSTRUCCION,
	TERMINAR_PROCESO, //Libera la pcb, avisa a memoria y a consola
	FINALIZAR_PROCESO,
	BLOQUEAR_PROCESO,
	PETICION_KERNEL,
	APROPIAR_RECURSOS,
	DESALOJAR_RECURSOS,
	DESALOJAR_PROCESO,
	PROCESAR_INSTRUCCION,
	// memoria
	ACCESO_A_PAGINA,
	PETICION_CPU,
	PAGE_FAULT,
	NUEVO_PROCESO_MEMORIA,
	FINALIZAR_PROCESO_MEMORIA,
	READ_MEMORY,
	WRITE_MEMORY,
	// filesystem
	ABRIR_ARCHIVO,
	CERRAR_ARCHIVO,
	APUNTAR_ARCHIVO,
	TRUNCAR_ARCHIVO,
	LEER_ARCHIVO,
	ESCRIBIR_ARCHIVO,
	CREAR_ARCHIVO,
}op_code;


typedef struct {
	int opcode_lenght;
	char* opcode;
	int parametro1_lenght;
	int parametro2_lenght;
	int parametro3_lenght;
	char* parametros[3];

}t_instruccion;


typedef struct {
    uint32_t AX;   // Registro de 4 bytes
    uint32_t BX;   // Registro de 4 bytes
    uint32_t CX;   // Registro de 4 bytes
    uint32_t DX;   // Registro de 4 bytes
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

	registros_CPU* registros_CPU;

	int prioridad;

	// lista de t_tabla_de_archivos_por_proceso
	t_list* tabla_archivos_abiertos_del_proceso;

	t_temporal* temporal_ready; // para cronometrar el tiempo para el Round Robbin

} t_pcb;

#endif /* GLOBAL_H_ */
