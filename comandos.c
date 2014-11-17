/************************************************************************ 
Fichero: comandos.c 
Programa: miftp
Otros ficheros del programa:  miftp.c, funciones.c, comandos.c, 
			      colaboradoras.c, miftp.h
Version: 1.6 Fecha: 21-04-2009 
Autor/es: Luis Fernando Nicolas Alonso, lnicalo@ribera.tel.uva.es
	  Javier Cortejoso de Andres, jcorand@ribera.tel.uva.es
	  Grupo 2 de Fundamentos de Ordenadores, Ing. Telecomunicaciones
Descripcion: Cliente ftp sencillo basado en el uso de sockets.
Objetivo: Archivo que contiene las funciones encargadas de la ejecucion
	  de los diferentes comandos del cliente miftp
*************************************************************************/

#include "librerias.h"
#include "miftp.h"
#include <libgen.h>
/* Estructura 	que mantiene el estado del programa. */
struct sEstado{
	int pasivo;	// Forma de abrir la conexion de datos:  modo no pasivo (0) o pasivo (1)
	int binario;	// Forma de transmitir el archivo: en modo binario (1) o ascci (0)
};
	
static struct sEstado estado = {0,0}; 


/*--------------------------------------------------------------------------
Funcion: ejecutar_get()
Sintaxis: int ejecutar_get(int socket_control, char **args)
Prototipo en: miftp.h
Argumentos: int scoket_control: Entero que contiene el descriptor del socket
	    establecido para la conexion de control.
	    char **args: "puntero" a un vector que contiene punteros a los 
	    argumentos (tipo cadena) que se han introducido al ejecutar el
	    la ordet "get".
Descripcion: Funcion que realiza la recepcion de archivos desde el servidor
             ftp hasta nuestro terminal.
Llama a: AbrirConexionDatos(), AbrirConexionDatosPasivo(), EnviarComando(), 
	 LeerReply(), ControlReplicas(), CierraConexionDatos(), 
	 RecibirArchivo().	
Fecha ultima modificacion: 24-03-09 
Ultima modificacion: Modificada para aï¿½adir modo pasivo.
---------------------------------------------------------------------------*/

int ejecutar_get(int socket_control, char **args) {
	struct sockaddr_in recoge_dir;	// para recoger datos en la funcion accept. No tiene ninguna utilidad en el resto de la funcion
	int socket_datos; 		// socket_datos: descriptor de socket que en el caso de una apertura pasiva se corresponde con el socket para transmitir archivos y en el caso de no pasivo es
					// en un primer momento socket de escucha sobre el que se hace el accept y que luego pasara a contener el socket de la conexion de datos 
	int socket_serv;		// socket_serv: descriptor de socket que almancena ek retorno del socket conectado devuelto por accept
	char comando [COM_MAX];		// cadena donde se almacena el comado FTP que vamos a enviar al servidor
	char *mensaje;			// Puntero donde se almacena la direccion del primer caracter de la cadena de respuesta obtenida por LeerReply()
	int bytes_enviados;		// Numero de bytes enviados en la transmision de.archivo EnviarArchivo()
	int len;			// variable auxiliar para usar la funcion accept()	
	

	// Comprobamos que hay un numero correcto de argumentos
	 if(args[1] == NULL) {
                printf ("Numero de argumentos invalido\n");
                return (-1); 
        }

	/*Comprobamos si estamos en modo pasivo o no, y actuamos en consecuencia
	abriendo la conexion de datos que corresponda*/
	if (estado.pasivo==0)
		socket_datos=AbrirConexionDatos(socket_control);
	else
		socket_datos=AbrirConexionDatosPasivo(socket_control);
	
	//Enviar comando de transferencia de archivo
	sprintf(comando, "RETR %s\r\n", args[1]);
	EnviarComando(socket_control, comando);
	
	// Leer replica e imprimirla por pantalla
	mensaje = LeerReply(socket_control);
	printf("%s",mensaje);
	
	// Control de replicas
	if (ControlReplicas(mensaje) != 1)	{
		fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
		free(mensaje);
		return (-1);
	}
	free (mensaje);
	
	// Aceptar la conexion del servidor si no es el modo pasivo
	if (estado.pasivo==0) {
		len = sizeof(recoge_dir);
		if( (socket_serv = accept(socket_datos, (struct sockaddr *) &recoge_dir,  &len)) < 0)    {
		        fprintf(stderr, "Error de accept()\n");
			perror("miftp");
		        exit(1);
		}
		// ya no necesitamos escuchar por mas tiempo en socket_datos
		CierraConexionDatos(socket_datos);
		// Hacemos esto para que en lo siguiente valga el mismo codigo tanto para el modo pasivo como
		// para el no pasivo
		socket_datos = socket_serv;
	}
	
	//Enviamos archivo. Si no se especifico un segundo argumento se considera que se quiere darle
	// el mismo nombre
	if (args[2] == NULL) {
		args[2] = args[1];
	}
	bytes_enviados=RecibirArchivo (socket_datos, args[2],estado.binario);
	
	//Cerrar conexion de datos
	CierraConexionDatos(socket_datos);
	
	// Leer replica e imprimirla por pantalla
	mensaje = LeerReply(socket_control);
	printf("%s",mensaje);
	
	// Control de replicas
	if (ControlReplicas(mensaje) != 2)	{
		fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
		free(mensaje);
		return (-1);
	}
	free (mensaje);
	
	// Mostramos el resultado solo si se transmitio algun byte
	if (bytes_enviados > 0) {
		fprintf(stdout,"local: %s, remoto: %s\n",args[2],args[1]);
	}
	//Devolver bytes enviados
	return (bytes_enviados);
}

/*--------------------------------------------------------------------------
Funcion: ejecutar_put()
Sintaxis: int ejecutar_put(int socket_control, char **args)
Prototipo en: miftp.h
Argumentos: int scoket_control: Entero que contiene el descriptor del socket
	    establecido para la conexion de control.
	    char **args: "puntero" a un vector que contiene punteros a los 
	    argumentos (tipo cadena) que se han introducido al ejecutar el
	    la ordet "put".
Descripcion: Funcion que realiza el envio de archivos desde el terminal que
	     se ejecuta hasta el sevidor ftp.
Llama a: AbrirConexionDatos(), AbrirConexionDatosPasivo(), EnviarComando(), 
         LeerReply(), ControlReplicas(), CierraConexionDatos(),
         EnviarArchivo(). 
Fecha ultima modificacion: 24-03-09 
Ultima modificacion: Modificada para anadir modo pasivo.
---------------------------------------------------------------------------*/
int ejecutar_put(int socket_control, char **args) {
	
	struct sockaddr_in recoge_dir;	// para recoger datos en la funcion accept. No tiene ninguna utilidad en el resto de la funcion
	int socket_datos; 		// socket_datos: descriptor de socket que en el caso de una apertura pasiva se corresponde con el socket para transmitir archivos y en el caso de no pasivo es
					// en un primer momento socket de escucha sobre el que se hace el accept y que luego pasara a contener el socket de la conexion de datos 
	int socket_serv;		// socket_serv: descriptor de socket que almancena ek retorno del socket conectado devuelto por accept
	char comando [COM_MAX];		// cadena donde se almacena el comado FTP que vamos a enviar al servidor
	char *mensaje;			// Puntero donde se almacena la direccion del primer caracter de la cadena de respuesta obtenida por LeerReply()
	int bytes_enviados;		// Numero de bytes enviados en la transmision de.archivo EnviarArchivo()
	int len;			// variable auxiliar para usar la funcion accept()
	int fd;				// Descriptor de archivo
	
	// Comprobamos que hay un numero correcto de argumentos
	if(args [1]==NULL)      {
                printf ("Numero de argumentos invalido\n");
                return (-1);
        }
	
	if (args[2] == NULL )
		args[2] = args[1];
 	sprintf(comando, "STOR %s\r\n", args[2]);
	
	/*Tenemos que comprobar que existe el archivo que queremos mandar, para que en caso de no existir no se cree ese archivo en el otro lado (con 0 bytes)*/
	
	if  ((fd = open(args[1], O_RDONLY)) < 0)
	{
		printf("Error al abrir el archivo\n");
		perror("miftp");
		return -1;
	}
	else	{
		/*Comprobamos si estamos en modo pasivo o no, y actuamos en consecuencia
		abriendo la conexion de datos que corresponda*/
		if (estado.pasivo==0)
			socket_datos=AbrirConexionDatos(socket_control);
		else
			socket_datos=AbrirConexionDatosPasivo(socket_control);

		EnviarComando(socket_control, comando);
	
		// Leer replica e imprimirla por pantalla
		mensaje = LeerReply(socket_control);
		printf("%s",mensaje);
	
		// Control de replicas
		if (ControlReplicas(mensaje) != 1) {
			free(mensaje);
			fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
			return (-1);
		}
		free (mensaje);
	
		// Aceptar la conexion del servidor si no es el modo pasivo
		if (estado.pasivo==0) {
			len = sizeof(recoge_dir);
			if( (socket_serv = accept(socket_datos, (struct sockaddr *) &recoge_dir,  &len)) < 0)    {
			        fprintf(stderr, "Error de accept()\n");
				perror("miftp");
		      	  exit(1);
			}
			// ya no necesitamos escuchar por mas tiempo en socket_datos
			CierraConexionDatos(socket_datos);
			// Hacemos esto para que en lo siguiente valga el mismo codigo tanto para el modo pasivo como
			// para el no pasivo
			socket_datos = socket_serv;
		}
	
	
		//Enviamos archivo
		bytes_enviados=EnviarArchivo (socket_datos, args[1],estado.binario,fd);
	
		//Cerrar conexion de datos
		CierraConexionDatos (socket_datos);
	
		// Leer replica e imprimirla por pantalla
		mensaje = LeerReply(socket_control);
		printf("%s",mensaje);
	
		// Control de replicas
		if (ControlReplicas(mensaje) != 2) {
			free(mensaje);
			fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
			return (-1);
		}
		free (mensaje);
	
		// Mostramos el resultado solo si se transmitio algun byte
		if (bytes_enviados > 0) {
      	          fprintf(stdout,"local: %s, remoto: %s\n",args[1],args[2]);
     	  	}
	
		//Devolver bytes enviados
		return (bytes_enviados);
	}
}


/*--------------------------------------------------------------------------
Funcion: ejecutar_list()
Sintaxis: int ejecutar_list(int socket_control, char **args)
Prototipo en: miftp.h
Argumentos: int scoket_control: Entero que contiene el descriptor del socket
	    establecido para la conexion de control.
	    char **args: "puntero" a un vector que contiene punteros a los 
	    argumentos (tipo cadena) que se han introducido al ejecutar el
	    la ordet "list".
Descripcion: Funcion que la realiza la recepcion del archivo que contiene
 	     la lista 'sencilla' de archivos que hay en el servidor y lo
 	     muestra por pantalla.
Llama a: AbrirConexionDatos(), AbrirConexionDatosPasivo(), EnviarComando(), 
         LeerReply(), ControlReplicas(), CierraConexionDatos(), 
         MostrarArchivo(), SetEstadoBinario(). 
Fecha ultima modificacion: 24-03-09 
Ultima modificacion: Modificada para anadir modo pasivo.
---------------------------------------------------------------------------*/
int ejecutar_list(int socket_control, char **args) {
	struct sockaddr_in recoge_dir;	// para recoger datos en la funcion accept. No tiene ninguna utilidad en el resto de la funcion
	int socket_datos; 		// socket_datos: descriptor de socket que en el caso de una apertura pasiva se corresponde con el socket para transmitir archivos y en el caso de no pasivo es
					// en un primer momento socket de escucha sobre el que se hace el accept y que luego pasara a contener el socket de la conexion de datos 
	int socket_serv;		// socket_serv: descriptor de socket que almancena ek retorno del socket conectado devuelto por accept
	char comando [COM_MAX];		// cadena donde se almacena el comado FTP que vamos a enviar al servidor
	char *mensaje;			// Puntero donde se almacena la direccion del primer caracter de la cadena de respuesta obtenida por LeerReply()
	int bytes_enviados;		// Numero de bytes enviados en la transmision de.archivo EnviarArchivo()
	int len;			// variable auxiliar para usar la funcion accept()
	
	/*Comprobamos si estamos en modo pasivo o no, y actuamos en consecuencia
	abriendo la conexion de datos que corresponda*/
	if (estado.pasivo==0)
		socket_datos=AbrirConexionDatos(socket_control);
	else
		socket_datos=AbrirConexionDatosPasivo(socket_control);
	
	/*Para transferir la lista de archivos, necesitamos que el formato de transferencia sea ascii,
	por lo que en caso de estar en binario enviamos el comando de ascii*/
	
	if (estado.binario==1)	{
		SetEstadoBinario(0,socket_control);
	}

	/*Comprobamos los argumentos. Debe ser 0 o 1. Si no hay enviamos el comando 
	solo, si hay alguno enviamos el argumento junto al comando*/
	if(args [1] == NULL)      {
		sprintf(comando, "NLST\r\n");
		EnviarComando(socket_control, comando);
        }
	else {
		sprintf(comando, "NLST %s\r\n", args[1]);
		EnviarComando(socket_control, comando);
	}

	// Leer replica e imprimirla por pantalla
	mensaje = LeerReply(socket_control);
	printf("%s",mensaje);
	
	// Control de replicas
	if (ControlReplicas(mensaje) != 1){
		fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
		free(mensaje);	
		return (-1);
	}
	free (mensaje);
	
	//Aceptar la conexion del servidor si no es el modo pasivo
	if (estado.pasivo==0) {
		len = sizeof(recoge_dir);
		if( (socket_serv = accept(socket_datos, (struct sockaddr *) &recoge_dir,  &len)) < 0)    {
		        fprintf(stderr, "Error de accept()\n");
			perror("miftp");
		        exit(1);
		}
		// ya no necesitamos escuchar por mas tiempo en socket_datos
		CierraConexionDatos(socket_datos);
		// Hacemos esto para que en lo siguiente valga el mismo codigo tanto para el modo pasivo como
		// para el no pasivo
		socket_datos = socket_serv;
	}

	//Recibimos el archivo y lo mostramos por pantalla
	bytes_enviados=MostrarArchivo (socket_datos);
	
	//Cerrar conexion de datos
	CierraConexionDatos(socket_datos);
	
	// Leer replica e imprimirla por pantalla
	mensaje = LeerReply(socket_control);
	printf("%s",mensaje);
	
	// Control de replicas
	if (ControlReplicas(mensaje) != 2)	{
		fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
		free(mensaje);
		return (-1);
	}
	free (mensaje);

	//Establecemos estado binario si al ejecutar el comando 
	//estabamos en estado binario
	if (estado.binario==1)	{
		SetEstadoBinario(1,socket_control);
	}
		
	//Devolver bytes enviados
	return (bytes_enviados);
}

/*--------------------------------------------------------------------------
Funcion: ejecutar_lista()
Sintaxis: int ejecutar_lista(int socket_control, char **args)
Prototipo en: miftp.h
Argumentos: int scoket_control: Entero que contiene el descriptor del socket
	    establecido para la conexion de control.
	    char **args: "puntero" a un vector que contiene punteros a los 
	    argumentos (tipo cadena) que se han introducido al ejecutar el
	    la ordet "lista".
Descripcion: Funcion que la realiza la recepcion del archivo que contiene
 	     la lista 'avanzada' de archivos que hay en el servidor y lo
 	     muestra por pantalla.
Llama a: AbrirConexionDatos(), AbrirConexionDatosPasivo(), EnviarComando(), 
         LeerReply(), ControlReplicas(), CierraConexionDatos(), 
         MostrarArchivo(), SetEstadoBinario().
Fecha ultima modificacion: 27-03-09 
Ultima modificacion: Modificada para anadir modo binario.
---------------------------------------------------------------------------*/
int ejecutar_lista(int socket_control, char **args) {
	struct sockaddr_in recoge_dir;	// para recoger datos en la funcion accept. No tiene ninguna utilidad en el resto de la funcion
	int socket_datos; 		// socket_datos: descriptor de socket que en el caso de una apertura pasiva se corresponde con el socket para transmitir archivos y en el caso de no pasivo es
					// en un primer momento socket de escucha sobre el que se hace el accept y que luego pasara a contener el socket de la conexion de datos 
	int socket_serv;		// socket_serv: descriptor de socket que almancena ek retorno del socket conectado devuelto por accept
	char comando [COM_MAX];		// cadena donde se almacena el comado FTP que vamos a enviar al servidor
	char *mensaje;			// Puntero donde se almacena la direccion del primer caracter de la cadena de respuesta obtenida por LeerReply()
	int bytes_enviados;		// Numero de bytes enviados en la transmision de.archivo EnviarArchivo()
	int len;			// variable auxiliar para usar la funcion accept()

        /*Comprobamos si estamos en modo pasivo o no, y actuamos en consecuencia
        abriendo la conexion de datos que corresponda*/
        if (estado.pasivo==0)
                socket_datos=AbrirConexionDatos(socket_control);
        else
                socket_datos=AbrirConexionDatosPasivo(socket_control);

        /*Para transferir la lista de archivos, necesitamos que el formato de transferencia sea ascii,
        por lo que en caso de estar en binario enviamos el comando de ascii*/

        if (estado.binario==1)  {
                SetEstadoBinario(0,socket_control);
        }

        /*Comprobamos los argumentos. Debe ser 0 o 1. Si no hay enviamos el comando
        solo, si hay alguno enviamos el argumento junto al comando*/
        if(args [1] == NULL)      {
                sprintf(comando, "LIST\r\n");
                EnviarComando(socket_control, comando);
        }
        else {
                sprintf(comando, "LIST %s\r\n", args[1]);
                EnviarComando(socket_control, comando);
        }

        // Leer replica e imprimirla por pantalla
        mensaje = LeerReply(socket_control);
        printf("%s",mensaje);

        // Control de replicas
        if (ControlReplicas(mensaje) != 1){
                fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
                free(mensaje);
                return (-1);
        }
        free (mensaje);

        //Aceptar la conexion del servidor si no es el modo pasivo
        if (estado.pasivo==0) {
                len = sizeof(recoge_dir);
                if( (socket_serv = accept(socket_datos, (struct sockaddr *) &recoge_dir,  &len)) < 0)    {
                        fprintf(stderr, "Error de accept()\n");
                        perror("miftp");
                        exit(1);
                }
                // ya no necesitamos escuchar por mas tiempo en socket_datos
                CierraConexionDatos(socket_datos);
                // Hacemos esto para que en lo siguiente valga el mismo codigo tanto para el modo pasivo como
                // para el no pasivo
                socket_datos = socket_serv;
        }

        //Recibimos el archivo y lo mostramos por pantalla
        bytes_enviados=MostrarArchivo (socket_datos);

        //Cerrar conexion de datos
        CierraConexionDatos(socket_datos);

        // Leer replica e imprimirla por pantalla
        mensaje = LeerReply(socket_control);
        printf("%s",mensaje);

        // Control de replicas
        if (ControlReplicas(mensaje) != 2)      {
                fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
                free(mensaje);
                return (-1);
        }
        free (mensaje);

        //Establecemos estado binario si al ejecutar el comando
        //estabamos en estado binario
        if (estado.binario==1)  {
                SetEstadoBinario(1,socket_control);
        }

        //Devolver bytes enviados
        return (bytes_enviados);
}

/*--------------------------------------------------------------------------
Funcion: ejecutar_comando_local()
Sintaxis: void ejecutar_comando_local(char *comando)
Prototipo en: miftp.h
Argumentos: char *comando: Puntero a cadena de caracteres que contiene el 
	    comando que queremos ejecutar en el shell de forma local
Descripcion: Funcion que ejecuta una orden en el shell sh local.
Llama a: 	 
Fecha ultima modificacion: 18-03-2009. 
Ultima modificacion: Desarrollo de la funcion.
---------------------------------------------------------------------------*/
void ejecutar_comando_local(char *comando)
{
	int pid;	// Identificador del proceso nuevo. Es devuelto por fork()
	
	pid = fork();
	
	if ( pid > 0 ) {
		if ( wait( (int *)0) < 0 ) {
			fprintf(stderr,"Error en wait()\n");
			perror("miftp");
			return;
		}
	}
	else if (pid == 0) {
		// ejecutamos comando a traves del shell
		execlp("/bin/sh","sh","-c",comando,(char*)0);
		// Si se llega aqui es que execlp() no ha tenido exito
		printf("No se ha podido ejecutar el comando\n");
		perror("miftp");
		return;
	}
	else {
		fprintf(stderr,"No se podido crear el hijo\n");
		perror("miftp");
		return;
	}
}

/*--------------------------------------------------------------------------
Funcion: ejecutar_quit()
Sintaxis: int ejecutar_quit(int socket_control)
Prototipo en: miftp.h
Argumentos: int scoket_control: Entero que contiene el descriptor del socket
	    establecido para la conexion de control.
Descripcion: Funcion que se encarga de comunicar al servidor ftp el cierre
	     de la conexion.
Llama a: EnviarComando(), LeerReply(), ControlReplicas().
Fecha ultima modificacion: 15-03-09 
Ultima modificacion: Desarrollo de la funcion.
---------------------------------------------------------------------------*/
void ejecutar_quit(int socket_control) {
	char *mensaje;		// Puntero donde se almacena la direccion del primer caracter de la cadena de respuesta obtenida por LeerReply()
	
        EnviarComando(socket_control, "QUIT\r\n");

        // Leer replica e imprimirla por pantalla
        mensaje = LeerReply(socket_control);
        printf("%s",mensaje);

        // Control de replicas
        if (ControlReplicas(mensaje) != 2)      {
                fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
		free(mensaje);
                return;
        }
        free (mensaje);

        return;
}

/*--------------------------------------------------------------------------
Funcion: borrar_archivo()
Sintaxis: void borrar_archivo(int socket_control,  char **args)
Prototipo en: miftp.h
Argumentos: int scoket_control: Entero que contiene el descriptor del socket
	    establecido para la conexion de control.
	    
	    char **args: "puntero" a un vector que contiene punteros a los 
	    argumentos (tipo cadena) que se han introducido al ejecutar el
	    la ordet "borrar".
Descripcion: Funcion que se encarga de borrar los archivos cuyo nombre se
	     introduce como argumentos en el servidor remoto ftp.
Llama a: EnviarComando(), LeerReply(), ControlReplicas().	 
Fecha ultima modificacion:19-03-09 
Ultima modificacion: Desarrollo de la funcion.
---------------------------------------------------------------------------*/
void borrar_archivo(int socket_control,  char **args) {
        //Enviamos comando de tipo transferencia sentido cliente->servidor
	char comando [COM_MAX];		// cadena donde se almacena el comado FTP que vamos a enviar al servidor
	char *mensaje;			// Puntero donde se almacena la direccion del primer caracter de la cadena de respuesta obtenida por LeerReply()

	// Comprobamos que hay un numero correcto de argumentos
	if(args [1]==NULL)      {
                printf ("Numero de argumentos invalido\n");
                return;
        }

        sprintf(comando, "DELE %s\r\n", args[1]);
        EnviarComando(socket_control, comando);

        // Leer replica e imprimirla por pantalla
        mensaje = LeerReply(socket_control);
        printf("%s",mensaje);

        // Control de replicas
        if (ControlReplicas(mensaje) != 2)      {
                fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
		free(mensaje);
                return;
        }
        free (mensaje);

        return;
}

/*--------------------------------------------------------------------------
Funcion: borrar_directorio()
Sintaxis: void borrar_directorio(int socket_control,  char **args)
Prototipo en: miftp.h
Argumentos: int scoket_control: Entero que contiene el descriptor del socket
	    establecido para la conexion de control.
	    char **args: "puntero" a un vector que contiene punteros a los 
	    argumentos (tipo cadena) que se han introducido al ejecutar el
	    la ordet "borrardir".
Descripcion: Funcion que se encarga de borrar los directorios cuyo nombre se
	     introduce como argumentos en el servidor remoto ftp.
Llama a: EnviarComando(), LeerReply(), ControlReplicas().	 
Fecha ultima modificacion:19-03-09 
Ultima modificacion: Desarrollo de la funcion.
---------------------------------------------------------------------------*/
void borrar_directorio(int socket_control,  char **args) {
        //Enviamos comando de tipo transferencia sentido cliente->servidor
	char comando [COM_MAX];		// cadena donde se almacena el comado FTP que vamos a enviar al servidor
	char *mensaje;			// Puntero donde se almacena la direccion del primer caracter de la cadena de respuesta obtenida por LeerReply()

	// Comprobamos que hay un numero correcto de argumentos
	if(args [1]==NULL)      {
                printf ("Numero de argumentos invalido\n");
                return;
        }

        sprintf(comando, "RMD %s\r\n", args[1]);
        EnviarComando(socket_control, comando);

        // Leer replica e imprimirla por pantalla
        mensaje = LeerReply(socket_control);
        printf("%s",mensaje);

        // Control de replicas
        if (ControlReplicas(mensaje) != 2)      {
		fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
		free(mensaje);
                return;
        }
        free (mensaje);

        return;
}

/*--------------------------------------------------------------------------
Funcion: crear_directorio()
Sintaxis: void crear_directorio(int socket_control,  char **args)
Prototipo en: miftp.h
Argumentos: int scoket_control: Entero que contiene el descriptor del socket
	    establecido para la conexion de control.
	    char **args: "puntero" a un vector que contiene punteros a los 
	    argumentos (tipo cadena) que se han introducido al ejecutar el
	    la ordet "creardir".
Descripcion: Funcion que se encarga de crear directorios cuyo nombre se
	     introduce como argumentos en el servidor remoto ftp.
Llama a: EnviarComando(), LeerReply(), ControlReplicas().	 
Fecha ultima modificacion:19-03-09 
Ultima modificacion: Desarrollo de la funcion.
---------------------------------------------------------------------------*/
void crear_directorio(int socket_control,  char **args) {
        char comando [COM_MAX];		// cadena donde se almacena el comado FTP que vamos a enviar al servidor
	char *mensaje;			// Puntero donde se almacena la direccion del primer caracter de la cadena de respuesta obtenida por LeerReply()

	// Comprobamos que hay un numero correcto de argumentos
	if(args [1]==NULL)      {
                printf ("Numero de argumentos invalido\n");
                return;
        }

        sprintf(comando, "MKD %s\r\n", args[1]);
        EnviarComando(socket_control, comando);
        // Leer replica e imprimirla por pantalla
        mensaje = LeerReply(socket_control);
        printf("%s",mensaje);

        // Control de replicas
        if (ControlReplicas(mensaje) != 2)      {
                fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
		free(mensaje);
		return;
        }
        free (mensaje);

        return;
}

/*--------------------------------------------------------------------------
Funcion: cambiar_directorio()
Sintaxis: void cambiar_directorio(int socket_control,  char **args)
Prototipo en: miftp.h
Argumentos: int scoket_control: Entero que contiene el descriptor del socket
	    establecido para la conexion de control.
	    char **args: "puntero" a un vector que contiene punteros a los 
	    argumentos (tipo cadena) que se han introducido al ejecutar el
	    la ordet "cd".
Descripcion: Funcion que se encarga de cambiar al directorio cuyo nombre se
	     introduce como argumentos en el servidor remoto ftp.
Llama a: EnviarComando(), LeerReply(), ControlReplicas().	 
Fecha ultima modificacion:19-03-09 
Ultima modificacion: Desarrollo de la funcion.
---------------------------------------------------------------------------*/
void cambiar_directorio(int socket_control,  char **args) {
	char comando [COM_MAX];		// cadena donde se almacena el comado FTP que vamos a enviar al servidor
	char *mensaje;			// Puntero donde se almacena la direccion del primer caracter de la cadena de respuesta obtenida por LeerReply()

	// Comprobamos que hay un numero correcto de argumentos
	if(args [1]==NULL)      {
                printf ("Numero de argumentos invalido\n");
                return;
        }

        sprintf(comando, "CWD %s\r\n", args[1]);
        EnviarComando(socket_control, comando);
        
        // Leer replica e imprimirla por pantalla
        mensaje = LeerReply(socket_control);
        printf("%s",mensaje);
        
        // Control de replicas
        if (ControlReplicas(mensaje) != 2)      {
                fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
        }
        free (mensaje);
        return;
}

/*--------------------------------------------------------------------------
Funcion: mostrar_directorio()
Sintaxis: void mostrar_directorio(int socket_control)
Prototipo en: miftp.h
Argumentos: int scoket_control: Entero que contiene el descriptor del socket
	    establecido para la conexion de control.
Descripcion: Funcion que muestra la ubicacion en el servidor remoto ftp.
Llama a: EnviarComando(), LeerReply(), ControlReplicas().	 
Fecha ultima modificacion:19-03-09 
Ultima modificacion: Desarrollo de la funcion.
---------------------------------------------------------------------------*/
void mostrar_directorio(int socket_control) {
	char *mensaje;			// Puntero donde se almacena la direccion del primer caracter de la cadena de respuesta obtenida por LeerReply()
	
        EnviarComando(socket_control, "PWD\r\n");
        
        // Leer replica e imprimirla por pantalla
        mensaje = LeerReply(socket_control);
        printf("%s",mensaje);
        
        // Control de replicas
        if (ControlReplicas(mensaje) != 2)      {
                fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
        }
        free (mensaje);
        return;
}

/*--------------------------------------------------------------------------
Funcion: ejecutar_pasivo()
Sintaxis: void ejecutar_pasivo()
Prototipo en: miftp.h
Argumentos:
Descripcion: Funcion que se encarga de establecer el modo pasivo en la 
	     comunicacion con el servidor. Simplemente se encarga de cambiar
	     el valor de la variable int estado.pasivo.
Llama a: 	 
Fecha ultima modificacion:24-03-09 
Ultima modificacion: Desarrollo de la funcion.
---------------------------------------------------------------------------*/
void ejecutar_pasivo()	{
	if (estado.pasivo==0)	{
		estado.pasivo=1;
		printf ("Modo pasivo establecido\n");
	}

	else if (estado.pasivo==1)	{
		estado.pasivo=0;
		printf ("Modo no pasivo establecido\n");
	}
	return;
}

/*--------------------------------------------------------------------------
Funcion: ejecutar_binario()
Sintaxis: void ejecutar_binario(int socket_control)
Prototipo en: miftp.h
Argumentos: int scoket_control: Entero que contiene el descriptor del socket
	    establecido para la conexion de control.
Descripcion: Funcion que se encarga de establecer el modo binario en la comu-
	     nicacion.
Llama a: SetEstadoBinario().	 
Fecha ultima modificacion:27-03-09 
Ultima modificacion: Desarrollo de la funcion.
---------------------------------------------------------------------------*/
void ejecutar_binario(int socket_control) {
	if (estado.binario==0)	{
		estado.binario=1;
		SetEstadoBinario (estado.binario, socket_control);
	}

	else if (estado.binario==1)	{
		printf ("Modo binario ya estaba establecido\n");
	}

	return;
}

/*--------------------------------------------------------------------------
Funcion: ejecutar_ascii()
Sintaxis: void ejecutar_ascii(int socket_control)
Prototipo en: miftp.h
Argumentos: int scoket_control: Entero que contiene el descriptor del socket
	    establecido para la conexion de control.
Descripcion: Funcion que se encarga de establecer el modo ascii en la comu-
	     nicacion.
Llama a: SetEstadoBinario().	 
Fecha ultima modificacion:27-03-09 
Ultima modificacion: Desarrollo de la funcion.
---------------------------------------------------------------------------*/
void ejecutar_ascii(int socket_control) {
	if (estado.binario==1)	{
		estado.binario=0;
		SetEstadoBinario(estado.binario, socket_control);
	}

	else if (estado.binario==0)	{
		printf ("Modo ascii ya estaba establecido\n");
	}
	return;
}	

/*--------------------------------------------------------------------------
Funcion: ejecutar_syst()
Sintaxis: void ejecutar_syst(int socket_control)
Prototipo en: miftp.h
Argumentos: int scoket_control: Entero que contiene el descriptor del socket
	    establecido para la conexion de control.
Descripcion: Funcion que sirve para conocer el tipo de sistema que es
	     el otro lado de la comunicacion.
Llama a: SetEstadoBinario().	 
Fecha ultima modificacion:28-03-09 
Ultima modificacion: Desarrollo de la funcion.
---------------------------------------------------------------------------*/
void ejecutar_syst(int socket_control) {
	char *mensaje;			// Puntero donde se almacena la direccion del primer caracter de la cadena de respuesta obtenida por LeerReply()

        EnviarComando(socket_control, "SYST\r\n");

        // Leer replica e imprimirla por pantalla
        mensaje = LeerReply(socket_control);
        printf("%s",mensaje);

        // Control de replicas
        if (ControlReplicas(mensaje) != 2)      {
                fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
        }
        free (mensaje);
        return;
}


/*--------------------------------------------------------------------------
Funcion: ejecutar_shlocal()
Sintaxis: void ejecutar_shlocal()
Prototipo en: miftp.h
Argumentos: No tiene.
Descripcion: Funcion que abre un shell para que el usuario pueda ejecutar comandos
	locales mas facilmente
Llama a:
Fecha ultima modificacion:28-04-09
Ultima modificacion: Desarrollo de la funcion.
---------------------------------------------------------------------------*/

void ejecutar_shlocal() {
	int pid;        // Identificador del proceso nuevo. Es devuelto por fork()
	char *shell;	// Puntero al shell del usuario
	char *ptrshell; // Puntero para buscar el nombre del shell en la cadena apuntada por shell
	int ptr;	// Variable intermedia para buscar el nombre del shell

        pid = fork();
        
        if ( pid > 0 ) {
                if ( wait( (int *)0) < 0 ) {
                        fprintf(stderr,"Error en wait()\n");
                        perror("miftp");
                        return;
                }
        }
        else if (pid == 0) {
		ptrshell = shell = getenv("SHELL");
		while((ptr = strfind(ptrshell,"/")) != -1) {
			ptrshell = ptrshell + ptr + 1;
		}
                // ejecutamos el shell
		fprintf(stdout,"Abriendo %s ...\n",ptrshell);
                execlp(shell,ptrshell,(char*)0);
                // Si se llega aqui es que execlp() no ha tenido exito
                printf("No se ha podido ejecutar el comando %s %s\n",shell,ptrshell);
                perror("miftp");
                return;
        }
        else {
                fprintf(stderr,"No se podido crear el hijo\n");
                perror("miftp");  
                return;
        }
	
}

/*--------------------------------------------------------------------------
Funcion: ejecutar_cdl()
Sintaxis: void ejecutar_cdl(char **args)
Prototipo en: miftp.h
Argumentos: char **args: "puntero" a un vector que contiene punteros a los 
	    argumentos (tipo cadena) que se han introducido al ejecutar el
	    la ordet "cdl".
Descripcion: Se encarga de ejecutar el comando para cambiar de directorio de  trabajo
		el cliente
Llama a: 	 
Fecha ultima modificacion:29-04-09 
Ultima modificacion: Desarrollo de la funcion.
---------------------------------------------------------------------------*/

void ejecutar_cdl(char **args) {
	if (args[1] == NULL) {
		fprintf(stdout,"cdl: Numero de argumentos insuficiente\n");
		return;
	}
	if (chdir(args[1]) == -1) {
		fprintf(stderr,"No se pudo cambiar de directorio a %s\n",args[1]);
                perror("miftp");
                return;

        }
	fprintf(stdout,"Cambio de directorio a: %s\n",args[1]);
	return;
}

/*--------------------------------------------------------------------------
Funcion: ejecutar_ayuda()
Sintaxis: void ejecutar_ayuda(char **args)
Prototipo en: miftp.h
Argumentos: char **args: "puntero" a un vector que contiene punteros a los 
	    argumentos (tipo cadena) que se han introducido al ejecutar el
	    la ordet "help o ?".
Descripcion: Ejecuta el comando ayuda mostrando si no se introdujeron argumentos
	la lista de todos los comandos posibles y si hay por argumento el nombre
	de algún comando se muestra una breve descripción de lo que hace
Llama a: 	 
Fecha ultima modificacion:29-04-09 
Ultima modificacion: Desarrollo de la funcion.
---------------------------------------------------------------------------*/

void ejecutar_ayuda(char **args) {
	char comandos[] = "Lista de los comandos:\n!\t\tborrar\t\tget\t\tlista\t\tpwd\n?\t\tborrardir\thelp\t\tlist\t\tquit\nascii\t\tcd\t\tlcd\t\tpasivo\t\tshell\nbinario\t\tcreardir\tput\t\tsyst\n";
	if (args[1] == NULL ) {
		fprintf(stdout,"%s",comandos);
		return;
	}
	else {
		if (!strcmp(args[1], "get")) {
			fprintf(stdout,"get:\tObtiene un archivo del servidor\n");
		}
		else if (!strcmp(args[1],"put")) {
			fprintf(stdout,"put:\tTransfiere un archivo al servidor\n");
		}
		else if (!strcmp(args[1], "!")) {
			fprintf(stdout,"!(comando):\tEjecuta un comando local\n");
		}
		else if (!strcmp(args[1],"quit")){
			fprintf(stdout,"quit:\tCierra la sesión ftp y sale de la aplicación\n");
		}
		else if (!strcmp(args[1],"list")){
			fprintf(stdout,"list:\tLista el contenido de un directorio remoto\n");
		}
		else if (!strcmp(args[1],"lista")){
			fprintf(stdout,"lista:\tLista el contenido de un directorio remoto\n");
		}
		else if (!strcmp(args[1],"borrar")){
			fprintf(stdout,"borrar:\tBorra un archivo remoto \n");
		}
		else if (!strcmp(args[1],"borrardir")){
			fprintf(stdout,"borrardir\tBorra un directorio remoto\n");          
		}
		else if (!strcmp(args[1],"creardir")){
			fprintf(stdout,"creardir:\tCrea un directorio remoto\n");
		}
		else if (!strcmp(args[1],"cd")){
			fprintf(stdout,"cd:\tCambia el directorio de trabajo remoto\n");
		}
		else if (!strcmp(args[1],"pwd")){
			fprintf(stdout,"pwd:\tImprime el directorio de trabajo remoto\n");
		}
		else if (!strcmp(args[1],"pasivo")){
			fprintf(stdout,"pasivo:\tPasa a modo de transferencia pasiva\n");
		}
		else if (!strcmp(args[1],"binario")){
			fprintf(stdout,"binario:\tPasa a modo de transferencia binario\n");
		}
		else if (!strcmp(args[1],"ascii")){
			fprintf(stdout,"ascii:\tPasa a modo de transferencia ascii\n");
		}
		else if (!strcmp(args[1],"syst")){
			fprintf(stdout,"syst:\tMuestra el SO y versión del servidor\n");
		}
		else if (!strcmp(args[1],"shell")){
			fprintf(stdout,"shell:\tAbre un shell local\n");
		}
		else if (!strcmp(args[1],"lcd")){
			fprintf(stdout,"lcd:\tCambia el directorio local de trabajo\n");
		}
		else if (!strcmp(args[1],"?")) {
			fprintf(stdout,"?:\tImprime la ayuda local\n");
		}
		else if (!strcmp(args[1],"help")){
			fprintf(stdout,"help:\tImprime la ayuda local\n");
		}
		else {
			fprintf(stdout,"No existe ningún comando %s\n",args[1]);
		}
		return;
	}
}
