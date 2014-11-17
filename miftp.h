// miftp
# if !defined (_MIFTP_H)
# define _MIFTP_H

// constantes para el programa
#define COM_MAX 256 	/* numero maximo de caracteres por comando */
#define MAX_ARG 15 	/* numero maximo de argumentos en un comando */
#define CAD_MAX 256 	/* numero bytes reservados para almacanar el login y el password */
#define BACKLOG 1	/* definimos un backlog */
#define TAMBUFFER 512	/* definimos un tamano para las lecturas y escrituras con el socket 
			 cuando estamos en modo binario */

// Funciones colaboradoras
char* LeerReply(int sockfd);
void EnviarComando(int sockfd, char *comando);
int ControlReplicas(char *digitos);

// Funciones colaboradoras para las funciones que ejecutan los comandos
int  AbrirConexionDatos(int socket_control);
int  AbrirConexionDatosPasiva(int socket_control);
void CierraConexionDatos(int socket);
int  EnviarArchivo(int socket, char *nombre, int modo, int fd);
int  RecibirArchivo(int socket, char *nombre, int modo);
int  MostrarArchivo(int socket);
void SetEstadoBinario (int estado, int socket_control);

// Funciones que forman parte de  main
int AbreConexionControl(char *nomserv);
void Autenticacion(int socket_control);
void Inicializacion (int socket_control);
void Interfaz(int socket_control);
void CierraConexionControl(int socket_control);

// Funciones que ejecutan comandos
int ejecutar_get(int socket_control,char **args);
int ejecutar_put(int socket_control,char **args);
void ejecutar_comando_local(char *comando);
void ejecutar_quit(int socket_control);
int ejecutar_list(int socket_control,char **args);
void borrar_archivo(int socket_control,  char **args);
void borrar_directorio(int socket_control,  char **args);
void crear_directorio(int socket_control,  char **args);
void mostrar_directorio(int socket_control);
void ejecutar_pasivo();
void ejecutar_binario(int socket_control);
void ejecutar_ascii(int socket_control);
void cambiar_directorio(int socket_control,  char **args);
void ejecutar_syst(int socket_control);
void ejecutar_shlocal();
void ejecutar_cdl(char **args);
void ejecutar_ayuda(char **args);
#endif
