/************************************************************************ 
Fichero: comandos.c 
Programa: miftp
Otros ficheros del programa:  miftp.c, funciones.c, comandos.c, 
			      colaboradoras.c, miftp.h
Version: 1.5 Fecha: 29-03-2009 
Autor/es: Luis Fernando Nicolas Alonso, lnicalo@ribera.tel.uva.es
	  Javier Cortejoso de Andres, jcorand@ribera.tel.uva.es
	  Grupo 2 de Fundamentos de Ordenadores, Ing. Telecomunicaciones
Descripcion: Cliente ftp sencillo basado en el uso de sockets.
Objetivo: Diseno de un programa-cliente capaz de conectarse con un ser-
	  vidor que siga el estadar ftp descrito en la rfc 793, ser au-
	  tentificado de forma correcta, y ser capaz de enviar y reci-
	  bir archivos, asi como otras funciones menos relevantes.
*************************************************************************/

#include "librerias.h"
#include "miftp.h"

/*--------------------------------------------------------------------------
Funcion: AbreConexionControl()
Sintaxis: int AbreConexionControl(char *nomserv)
Prototipo en: miftp.h
Argumentos: char *nomserv: puntero al inicio de la cadena que contiene el nombre del servidor o
		direccion IP al que queremos conectarnos
Descripcion: Realiza la apertura de al conexion de control
Llama a: LeerReply(), ControlReplicas()
Fecha ultima modificacion: 24-03-09 
Ultima modificacion: Ninguna. No se modifico desde su creacion
---------------------------------------------------------------------------*/
int AbreConexionControl(char *nomserv) {
	int socket_control;		// Descriptor de socket de la conexion de control. Es el valor devuelto por la funcion
	struct sockaddr_in servidor;	// Estructura donde se almacenan los datos relativos al servidor y al tipo de conexion que 
					// que se quiere hacer
	struct hostent *host;		// Puntero donde se almecena la direccion a la estructura que devuelve gethostbyname().
					// Se usa para hacer la conversion entre el nombre dns y la direccion IP
	char *mensaje; 			// Puntero al comienzo de la respuesta recibida del servidor

	/*Creacion de nuevo socket*/
	if ( (socket_control = socket(AF_INET, SOCK_STREAM, 0)) < 0)	{
		fprintf(stderr,"Error de Socket\n");
		perror("miftp");
		exit(1);
	}
	
	/*Inicializacion de estructura de direccion*/
	bzero(&servidor, sizeof(servidor));
	servidor.sin_family = AF_INET;
	servidor.sin_port   = htons(21);	
	/*Conversion dotted-decimal a 32 bits*/
	if ( (servidor.sin_addr.s_addr = inet_addr(nomserv)) == -1)	{
		if ((host = gethostbyname(nomserv)) == NULL ) {
			fprintf(stderr,"Error al introducir el servidor %s\n",nomserv);
			exit(1);
		}
		bcopy(*host->h_addr_list, (char *) & servidor.sin_addr, sizeof( servidor.sin_addr ));
	}
	
	/* Establecimiento de conexion */
	if (connect(socket_control, (struct sockaddr *) &servidor, sizeof(servidor)) < 0)	{
		fprintf(stderr,"Error de connect()\n");
		perror("miftp");
		exit(1);
	}
	
	// Leer replica e imprimirla por pantalla
	mensaje = LeerReply(socket_control);
	printf("%s",mensaje);

	// control de replicas
	 if (ControlReplicas(mensaje) != 2 ) {
                // La replica no es la esperada. devolvemos -1
                free(mensaje);
                exit(1);
        }
	free(mensaje);
	// si todo va bien devolvemos el socket conectado
	return socket_control;
}

/*--------------------------------------------------------------------------
Funcion: Autenticacion()
Sintaxis: void Autenticacion(int socket_control)
Prototipo en: miftp.h
Argumentos: int socket_control: Descriptor de socket de la conexion de control
Descripcion: Realiza la autenticacion cuando el usuario quiere acceder al sistema.
	En la autenticacion se permiten como maximo tres intentos. Se ha comprobado que un servidor
	FTP habitual no admite mayor numero se intentos
Llama a: EnviarComando(), LeerReply(), ControlReplicas()
Fecha ultima modificacion: 24-03-09 
Ultima modificacion: En su primera version si se pulsaba enter directamente cuando se solicitaba el login
	se enviaba el comando sin argumentos recibiendo un mensaje de error por parte del servidor. Se soluciono 
	el problema haciendo que cuando se pulse enter dierctamente sin introducir ningun caracter antes se considera
	quiere usar el login de la sesion actual en la maquina local
---------------------------------------------------------------------------*/
void Autenticacion(int socket_control)
{
	char *user;			// puntero al inicio de la cadena del nombre del usuario que ejecuto el programa
	char cadena[CAD_MAX];		// Cadena donde se guarda el login recogido por gets()
	char *password;			// Puntero al inicio del password. Se inicializa con el valor devuelto de getpass()
	char comando[CAD_MAX + 8];	// Cadena que se utiliza para almacenar los comando que vamos a enviar al servidor.
					// Necesitamos 8 bytes mas: comando (USER, PASS), el espacio y el final de cadenay el \r\n
	char *mensaje; 			// Puntero al comienzo de la respuesta recibida del servidor
	int veces=0;			// Entero que almecena el numero de intentos realizados para la autenticacion
	int exito=0;			// Indica si se ha conseguido acceso (toma el valor 1) o no (valor 0)
	do	{
			if ((user = getenv("LOGNAME")) == NULL) {
				fprintf(stderr,"Error al intentar averiguar el nombre del usuario\n");
				perror("miftp");
			}
			printf("Nombre(%s):",user);
			// Leemos el login de la entrada estandar LGN_MAX -1 porque de los LGN_MAX bytes unos 
			// es para el final de cadena
			if(fgets(cadena, CAD_MAX, stdin) == NULL) {
				fprintf(stderr,"Error en la lectura del login\n");
				exit(1);
			}
	
			// Enviamos comando USER + login + '\r\n'
			// pero antes quitamos el \n
			cadena[strlen(cadena)-1] = 0;
			if (*cadena == '\0' ) {
				sprintf(comando, "USER %s\r\n",user);

			}
			else {
				sprintf(comando, "USER %s\r\n",cadena);			
			}
				
			EnviarComando(socket_control, comando);
				
			// Leer replica e imprimirla por pantalla
			mensaje = LeerReply(socket_control);
			printf("%s",mensaje);
			// Control de replicas
			if (ControlReplicas(mensaje) != 3) {
				// La replica no es la esperada.
                		free(mensaje);
                		exit(1);
        		}
			free(mensaje);

			// Leer password. La llamada devuelve un 
			password = getpass("Password:");
			if (password == NULL ) {
				fprintf(stderr,"Error al leer el password\n");
				perror("miftp");
				exit(1);
			}
			// Enviamos comando PASS + password + '\r\n'
			sprintf(comando, "PASS %s\r\n",password);
			EnviarComando(socket_control, comando);
			// Leer replica e imprimirla por pantalla
			mensaje = LeerReply(socket_control);
			printf("%s",mensaje);
			// Control de replicas
			if (ControlReplicas(mensaje) != 2)	{
					fprintf(stdout,"Usuario/Password incorrectos\n");
					++veces;
			}
			else	{
					exito=1;
			}
			free(mensaje);
	// Dejamos tres intentos para introducir el login y el password correctamente
	}while (veces<3 && exito == 0);

	// Si se ha fallado tres veces salimos
	if (veces == 3) {
		exit(0);
	}
}

/*--------------------------------------------------------------------------
Funcion: Inicializacion ()
Sintaxis: void Inicializacion (int socket_control)
Prototipo en: miftp.h
Argumentos: int socket_control: Descriptor de socket de la conexion de control
Descripcion: Establece un estado inicial en el tipo de transmision
Llama a: SetEstadoBinario()
	 ejecutar_syst()
Fecha ultima modificacion: 31-03-09 
Ultima modificacion: Ninguna
---------------------------------------------------------------------------*/
void Inicializacion (int socket_control)
{
	SetEstadoBinario(0, socket_control);
	ejecutar_syst(socket_control);
	return;
}

/*--------------------------------------------------------------------------
Funcion: Interfaz()
Sintaxis: void Interfaz(int socket_control)
Prototipo en: miftp.h
Argumentos: int socket_control: Descriptor de socket de la conexion de control
Descripcion: Realiza de interfaz con el usuario. Se muestra un prompt y se solicita un comando segun el cual
	se va llamando a los distintos manejadores
Llama a: int ejecutar_put(int socket_control,char **args);
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

Fecha ultima modificacion: 24-03-09 
Ultima modificacion: Se soluciono el problema que se generaba cuando el usuario pulsaba enter
	derectamente sin introducir ninguna caracter antes. Si no se tiene en cuenta esto hay 
	problemas ya un vez que se quita el retorno de linea almacenado por gets() se quiere 
	trabajar con una cadena vacia
---------------------------------------------------------------------------*/

void Interfaz(int socket_control)
{
	char comando[COM_MAX];	// Cadena donde se almacena el comando introducido por el usuario
	char *saveptr;		// 
	char *token[MAX_ARG];	// Vector de punteros a los distintos argumentos de los comandos
	int j;			// Variable de control para el bucle que se hace en la separacion de los argumentos del comando
	int bytes;		// Bytes transmitidos en los comandos que requieren conexion de datos

	while (1)		{
			printf ("miftp> ");
			// Recogemos la cadena introducida, y hacemos una limpieza de buffer de stdin por si tuviera 
			// residuos
			fflush (stdin);	
			if(fgets(comando, COM_MAX, stdin) == NULL) {
				fprintf(stderr,"Error en la lectura del comando\n");
				exit(1);
			}
			// quitamos el retorno de linea recogido por fgets
			comando[strlen(comando)-1] = 0;
			// Hay que asegurarse de que no se ha dado el intro sin haber introducido un comando
			// en ese caso *comando ser�a nulo
			if (*comando != 0)	{
				// solo separamos los  cuando no es comando local, es decir en el primer caacter no tenemos un !
				if (*comando != '!') {
					bzero(token,MAX_ARG*sizeof(char *));
					// separamos la cadena. Utilizando como separador el espacio en blanco
					j=0;
					if ((token[j] = (char *)strtok_r(comando, " ", &saveptr)) != NULL) { 
						j++;			
						while (((token[j] = (char *)strtok_r(NULL, " ", &saveptr)) != NULL) && (j < MAX_ARG))  {
							j++; 
							if (j == MAX_ARG) {
								fprintf(stdout,"Demasiados argumentos\n");
								continue;
							}
		       				}
					}
				} 

				//Sabemos que tiene que el usuario tiene que introducir lo primero es el comando
				// No comprobamos los argumentos (token) aqui ya lo hacemos en las funciones que ejecutan los 
				// comando porque son las que saben si los argumentos son correctos 
				if (!strcmp(token[0], "get")) {
					bytes = ejecutar_get(socket_control,token);
					// mostrar bytes recibidos si se ha recibido al menos 1
					if (bytes > 0 ) {
						fprintf(stdout,"%d bytes recibidos\n",bytes);
					}
				}
				else if (!strcmp(token[0],"put")) {
					bytes = ejecutar_put(socket_control,token);
					// mostrar bytes enviados si se ha enviado al menos 1
					if (bytes > 0 ) {
						fprintf(stdout,"%d bytes enviados\n",bytes);
					}
				}
				// si el primer caracter es ! ejecuto local
				else if (*comando  == '!') {
					// quitamos el '!' del principio
					strcpy(comando,comando+1);	
					ejecutar_comando_local(comando);
				}
				else if (!strcmp(token[0],"quit")){
		                	ejecutar_quit(socket_control);
					// despues de ejecutar quit el programa se sale de este bucle 
					// infinito para proceder a la terminacion del programa
					break;
				}
			
				else if (!strcmp(token[0],"list")){
		                	bytes = ejecutar_list(socket_control, token);
					// mostrar bytes recibidos si se ha recibido al menos 1
					if (bytes > 0 ) {
						fprintf(stdout,"%d bytes recibidos\n",bytes);
					}
				}

                                else if (!strcmp(token[0],"lista")){
                                        bytes = ejecutar_lista(socket_control, token);
                                        // mostrar bytes recibidos si se ha recibido al menos 1
                                        if (bytes > 0 ) {
                                                fprintf(stdout,"%d bytes recibidos\n",bytes);
                                        }
                                }
			
				else if (!strcmp(token[0],"borrar")){
		                	borrar_archivo (socket_control, token);
				}

				else if (!strcmp(token[0],"borrardir")){
		                	borrar_directorio (socket_control, token);
				}
			
				else if (!strcmp(token[0],"creardir")){
		                	crear_directorio (socket_control, token);
				}

				else if (!strcmp(token[0],"cd")){
		                	cambiar_directorio (socket_control, token);
				}

				else if (!strcmp(token[0],"pwd")){
		                	mostrar_directorio (socket_control);
				}

				else if (!strcmp(token[0],"pasivo")){
		                	ejecutar_pasivo();
				}

				else if (!strcmp(token[0],"binario")){
		                	ejecutar_binario(socket_control);
				}

				else if (!strcmp(token[0],"ascii")){
		                	ejecutar_ascii(socket_control);
				}

				else if (!strcmp(token[0],"syst")){
		                	ejecutar_syst(socket_control);
				}
				else if (!strcmp(token[0],"shell")){
					ejecutar_shlocal();
				}
				else if (!strcmp(token[0],"cdl")){
					ejecutar_cdl(token);
				}
				else if (!strcmp(token[0],"?") || !strcmp(token[0],"help")){
					ejecutar_ayuda(token);
				}
				else	{
					printf("Comando invalido\n");
				}
			}
	}
}

/*--------------------------------------------------------------------------
Funcion:CierraConexionControl()
Sintaxis: void CierraConexionControl(int socket_control)
Prototipo en: miftp.h
Argumentos: int socket_control: Descriptor de socket de la conexion de control
Descripcion: Cierra la conxion asociada al descriptor de socket pasada como 
	argumento. Es un funcion envoltorio de close()
Llama a: 
Fecha ultima modificacion: 24-03-09 
Ultima modificacion: Ninguna
---------------------------------------------------------------------------*/
void CierraConexionControl(int socket_control)
{
	if (close(socket_control) < 0 ) {
		fprintf(stderr,"Error en cierre del socket\n");
		perror("miftp");
		exit(1);
	}
}


