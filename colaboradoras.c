/************************************************************************ 
Fichero: colaboradoras.c 
Programa: miftp
Otros ficheros del programa:  miftp.c, funciones.c, comandos.c, 
			      colaboradoras.c, miftp.h
Version: 1.6 Fecha: 21-04-2009 
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
Funcion: LeerReply()
Sintaxis: char * LeerReply(int sockfd)
Prototipo en: miftp.h
Argumentos: int sockfd: Entero que contiene el descriptor del socket
	    establecido para la conexion de control del a que se leera la
	    respuesta por parte del servidor.
Descripcion: Funcion que lee la replica enviada por el sevidor por el socket 
	     de control (normalmente tras el envio de un comando).Devuelve una
	     cadena que contiene la respuesta enviada por el servidor.
Llama a: 	
Fecha ultima modificacion: 21-04-09 
Ultima modificacion: Anadidos comentarios.
---------------------------------------------------------------------------*/

char* LeerReply(int sockfd) {

	int 	tambloque;	// Tamano del bloque para leer
	int 	salir;		// salir es una variable que indica que tenemos que se ha leido un espacio 
				// precedido de tres digitos en la linea y que va a haber que salir
				// en el proximo \r\n
	int 	bytes;		// Numero de bytes leidos en total
	char 	*mensaje;  	// puntero a la cadena recibida del servidor
	char 	*ptr;		// Puntero auxiliar que usamos para no perder la referencia a mensaje al leer
	int	n;		// Numero de bytes leidos en cada llamada a read

	/*Reserva de memoria*/
	// El tamano inicial reservado para almacenar la respuesta va a ser de 60 bytes
	tambloque = 60;
	mensaje = malloc(tambloque);
	if(mensaje == NULL)	{
		fprintf(stderr,"Error en la reserva de memoria\n");
		perror("miftp");
		exit(1);
	}

	bytes = 0;
	salir = 0;
	ptr = mensaje;
	while(1)  {
		// Si el tamano reservado es menor ampliamos en 30 bytes
		if (bytes >= tambloque) {
			tambloque += 30;
			realloc(mensaje, tambloque);
			if(mensaje == NULL)	{
				fprintf(stderr,"Error en la reserva de memoria\n");
				perror("miftp");
				exit(1);
			}
			ptr = mensaje;
			// hay que desplazar el puntero el numero de bytes leidos hasta 
			// el momento antes de pedir la nueva reserva de memoria porque 
			// si no sobreescribimos lo que llevabamos leido
			ptr+= bytes;
		}

		// Leemos byte a byte
		n=read(sockfd, ptr, 1);
		if(n<0) {
			fprintf(stderr,"Error en la lectura del socket\n");
			perror("miftp");
			exit(1);
		}
		else if(n==0) {
			/* Si n es 0 es porque se ha le�do un final de archivo debido a que el
			servidor ha cerrado la conexion inesperadamente y, por tanto, dejamos
			de leer del socket y cerramos el programa*/
			exit(0);
		}
		
		// Actualizamos el valor de bytes
		bytes += n;
		// Cuando encontramos un espacio en blanco miramos si 
		// en los tres caracteres anteriores hay digitos
		// el primer digito es el primer caracter de la respuesta
		// o delante de los digitos hay un \r\n
		// y ademas Esto solo lo hacemos si el numero de bytes leidos es el suficiente
		if (bytes>= 4 ) {
			if (*ptr == ' ' && isdigit(*(ptr-1)) && isdigit(*(ptr-2)) && isdigit(*(ptr-3)) &&
				((ptr-3 == mensaje) || ( ptr-4 > mensaje && *(ptr-4)=='\n' && *(ptr-5)=='\r' ) ) ) {
				//Hemos encontrado el espacio que buscabamos
				salir = 1;
			}
			if (*ptr == '\n' && *(ptr-1) == '\r' && salir) {
				// Hemos encontrado el \r\n en una fila con tres digitos y espacio en blanco
				break;
			}
		}

		// Actualizamos el puntero
		ptr += n;
	}

	// Ejecucion correcta: anadimos caracter nulo y devolvemos el numero de bytes leidos
	mensaje[bytes] = 0;
	return mensaje;
}

/*--------------------------------------------------------------------------
Funcion: EnviarComando()
Sintaxis: void EnviarComando(int sockfd, char *comando)
Prototipo en: miftp.h
Argumentos: int sockfd: Entero que contiene el descriptor del socket
	    establecido para la conexion de control por la que se enviara
	    el comando.
	    char *comando: puntero que apunta a la cadena que contiene el 
	    comando a enviar.
Descripcion: Funcion que envia un comando al servidor
Llama a: 	
Fecha ultima modificacion: 21-04-09 
Ultima modificacion: Anadidos comentarios.
---------------------------------------------------------------------------*/
void EnviarComando(int sockfd, char *comando)
{

	char 	*ptr;		// Puntero a lo que nos queda por escribir
	int 	nleft;		// Numero de bytes que nos quedan por escribir
	int  	n;		// Bytes escritos en cada intento

	ptr = comando;
	nleft = strlen(comando);
	n = 0;
	while ( (n = write(sockfd, ptr,nleft) < nleft )) {
		if(n <= 0) {
			fprintf(stderr,"Error de write()\n");
			perror("miftp");
			exit(1);
		}
		ptr += n;
		nleft -= n;
	}

}

/*--------------------------------------------------------------------------
Funcion: ControlReplicas()
Sintaxis: int ControlReplicas(char *mensaje)
Prototipo en: miftp.h
Argumentos: char *mensaje: Puntero a la cadena sobre la que se va a hacer el 
	    control de replicas.
Descripcion: Se encarga de realizar el control de replicas de los mensajes
	     enviados por el servidor. Devuelve el primer digito del codigo
	     numerico de la replica del servidor.
Llama a: 	
Fecha ultima modificacion: 21-04-09 
Ultima modificacion: Anadidos comentarios.
---------------------------------------------------------------------------*/
int ControlReplicas(char *mensaje)
{

	char 	digitochar[1];		//Caracter para extraer el primer numero de la replica
	int 	digito;			//Primer numero de la replica
	strncpy (digitochar, mensaje, 1);
	digito = atoi (digitochar);
	switch (digito)	{
		case 1:
			return (1);
		case 2:
			return (2);
		case 3:
			return (3);
		case 4:
			return (4);
		case 5:
			return (5);
		}

}

/*-------------------------------------------------------------------------------
Funcion: AbrirConexionDatos()
Sintaxis: int AbrirConexionDatos(int socket_control)
Prototipo en: miftp.h
Argumentos: int socket_control: Entero que contiene el descriptor del socket de 
	    control establecido para la conexion de control por la que se enviara
	    el comando.
Descripcion: Funcion que se encarga de abrir una conexion de datos para la
	     transferencias de archivos.
Llama a: void EnviarComando(),
	 char * LeerReply(),
	 int ControlReplicas().
Fecha ultima modificacion: 21-04-09 
Ultima modificacion: Anadidos comentarios.
--------------------------------------------------------------------------------*/
int AbrirConexionDatos(int socket_control)
{
	int 			socket_datos;		// Descriptor del socket de datos
	struct sockaddr_in 	dir_datos;		// Estructura que almacena la direccion del servidor
	struct sockaddr_in 	dir_provisional;  	// Estructura provisional para conocer el puerto del servidor
	int sockad;
	char 			comando[COM_MAX];	// string donde vamos almacenar el comando FTP a enviar
	char 			*mensaje;		// Puntero al comienzo de la respuesta recibida del servidor
	int 			puerto;			// enteros para guardar el puerto del socket de datos
	char 			*dir_ip;		// puntero cadena para guardar la IP del socket de control
	
	// variables necesarias para separar la IP en cuatro numeros mediante el separador punto 
	int j;
	char *saveptr;
	char *token[4]; // una IP tiene cuatro numeros de tres o menos cifras separadas por puntos

	/*Creacion de nuevo socket*/
	if ( (socket_datos = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr,"Error de Socket en AbrirConexionDatos()\n");
		perror("miftp");
		exit(1);
	}
	
	/*Inicializacion de estructura de direccion*/
	bzero(&dir_datos, sizeof(dir_datos));
	dir_datos.sin_family = AF_INET;
	dir_datos.sin_port   = 0;
	dir_datos.sin_addr.s_addr = htonl(INADDR_ANY);
	/* Hacemos que el sistema operativo nos asigne un puerto */
	if (bind(socket_datos, (struct sockaddr *) &dir_datos, sizeof(dir_datos)) != 0)	{
		fprintf(stderr,"Error de bind()\n");
		perror("miftp");
		exit(1);
	}
	
	/* socket a la escucha*/
	if (listen(socket_datos, BACKLOG) != 0) {
		fprintf(stderr,"Error de listen()\n");
		exit(1);
	}
	
	// Obtenemos el numero de puerto que nos ha asignado el SO para luego enviarselo al servidor
	sockad = sizeof(dir_provisional);
	if (getsockname(socket_datos, (struct sockaddr *) &dir_provisional,&sockad) < 0 ) {
		fprintf(stderr,"Error al obtener el puerto asignado\n");
		perror("miftp");
		exit(1);
	}
	puerto = dir_provisional.sin_port;
	
	// Obtenemos la IP para luego enviarselo al servidor
	if (getsockname(socket_control, (struct sockaddr *) &dir_provisional,&sockad) < 0 ) {
		fprintf(stderr,"Error al intentar conocer la IP\n");
		perror("miftp");
		exit(1);
	}
	dir_ip = inet_ntoa(dir_provisional.sin_addr);
	
	// separamos la IP en dotted decimal en 4 numeros
	j=0;
	if ((token[j] = (char *)strtok_r(dir_ip, ".", &saveptr)) != NULL) { 
		j++;			
		while ((token[j] = (char *)strtok_r(NULL, ".", &saveptr)) != NULL) {
			j++; 
            	}
	} 
	
	// construimos el comando a enviar con sprintf
	// hay que convertir el numero de puerto a p1*256 + p2
	
	if ( sprintf(comando,"PORT %s,%s,%s,%s,%d,%d\r\n",token[0],token[1],token[2],token[3],puerto/256,puerto%256) < 0) {
		fprintf(stderr,"Error en sprint al construir el comando FTP PORT\n");
		perror("miftp");
		exit(1);
	}
	EnviarComando(socket_control, comando);
	
	// Leer replica e imprimirla por pantalla
	mensaje = LeerReply(socket_control);
	printf("%s",mensaje);
	
	// control de replicas
	if (ControlReplicas(mensaje) != 2 ) {
		// La replica no es la esperada. Salimos
		free(mensaje);
		exit(1);
	}
	// liberamos el espacio que reservo la funcion LeerReply
	free(mensaje);
	
	// si todo va bien devolvemos el socket de escucha para la conexion de datos
	return socket_datos;
}

/*---------------------------------------------------------------------------------
Funcion: AbrirConexionDatosPasivo()
Sintaxis: int AbrirConexionDatosPasivo(int socket_control)
Prototipo en: miftp.h
Argumentos: int socket_control: Entero que contiene el descriptor del socket de 
	    control establecido para la conexion de control por la que se enviara
	    el comando.
Descripcion: Funcion que se encarga de abrir una conexion de datos en modo pasivo
	     para la transferencias de archivos. Devuelve el socket de datos creado
	     o -1 en caso de error.
Llama a: void EnviarComando(),
	 char * LeerReply(),
	 int ControlReplicas().	
Fecha ultima modificacion: 21-04-09 
Ultima modificacion: Anadidos comentarios.
-----------------------------------------------------------------------------------*/
int AbrirConexionDatosPasivo(int socket_control)
{

	char 	comando[COM_MAX];      	// Cadena que contiene el comando a enviar
	char 	*p1;			// Puntero utilizado para buscar ( de la respuesta del servidor
	char 	*p2;			// Puntero utilizado para buscar ) de la respuesta del servidor
	char 	*cadena;		//Puntero a cadena que contiene la respuesta por parte del servidor	
	int 	socket_datos;		//Descriptor de la conexion de datos
	struct 	sockaddr_in servidor;	//Estructura con la direccion del servidor
	struct 	hostent *host;
	
	/*variables necesarias para separar la IP y puerto en 6 numeros mediante el separador coma */
	int j, digito1, digito2, puerto;
	char *saveptr;
	char *token[6]; // Lo que nos manda tiene 6 campos

	/*Enviamos comando indicando que es modo pasivo*/
        sprintf(comando, "PASV\r\n");
        EnviarComando(socket_control, comando);

 	/* Leer replica e imprimirla por pantalla*/
        cadena = LeerReply(socket_control);
        printf("%s",cadena);

	/* Control de replicas*/
        if (ControlReplicas(cadena) != 2)      {
                fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
                return -1;
        }
	
	/*Buscamos los parentesis dentro de mensaje para quedarnos
	con los datos que nos manda*/

	/*Buscamos (*/
	p1 = strchr(cadena,'(');
	/*Buscamos )*/
	p2 = strchr(cadena,')');
	/*Eliminamos el segundo parentesis*/
	*p2 = 0;
	/*Copiamos desde el caracter siguiente a ( hasta el final de cadena*/
	strcpy (cadena,p1+1);

	/* separamos la IP y puerto de lo que nos manda en 6 numeros*/
	j=0;
	if ((token[j] = (char *)strtok_r(cadena, ",", &saveptr)) != NULL) { 
		j++;			
		while ((token[j] = (char *)strtok_r(NULL, ",", &saveptr)) != NULL) {
			j++; 
            	}
	} 

	/*Generamos la IP con el formato "estandar" separada por puntos*/
	sprintf(cadena, "%s.%s.%s.%s",token[0],token[1],token[2],token[3]);

	/*Generamos el puerto*/
	digito1 = atoi (token[4]);
	digito2 = atoi (token[5]);
	puerto = (digito1*256) + digito2;

	/*Creacion de nuevo socket*/
	if ( (socket_datos = socket(AF_INET, SOCK_STREAM, 0)) < 0)	{
		fprintf(stderr,"Error de Socket\n");
		perror("miftp");
		exit(1);
	}

	/*Inicializacion de estructura de direccion*/
	bzero(&servidor, sizeof(servidor));
	servidor.sin_family = AF_INET;
	servidor.sin_port   = htons(puerto);

	/*Conversion dotted-decimal a 32 bits*/
	if ((servidor.sin_addr.s_addr = inet_addr(cadena)) == -1)	{
		fprintf(stderr,"Error de inet_addr() para %s\n",cadena);
		exit(1);
	}

	/* Establecimiento de conexion */
	if (connect(socket_datos, (struct sockaddr *) &servidor, sizeof(servidor)) < 0)	{
		fprintf(stderr,"Error de connect()\n");
		perror("miftp");
		exit(1);
	}
	
	free (cadena);
	return (socket_datos);
}

/*-------------------------------------------------------------------------------
Funcion: CierraConexionDatos()
Sintaxis: void CierraConexionDatos(int socket)
Prototipo en: miftp.h
Argumentos: int socket: Descriptor de socket de la conexion a cerrar.
Descripcion: Funcion que cierra una conexion
Llama a: 
Fecha ultima modificacion: 21-04-09 
Ultima modificacion: Anadidos comentarios.
--------------------------------------------------------------------------------*/
void CierraConexionDatos(int socket)
{
	if (close(socket) < 0 ) {
		fprintf(stderr,"Error en cierre del socket de datos\n");
		perror("miftp");
		exit(1);
	}
}

/*---------------------------------------------------------------------------------
Funcion: EnviarArchivo()
Sintaxis: int EnviarArchivo(int socket, char *nombre, int modo, int fd)
Prototipo en: miftp.h
Argumentos: int socket: Entero que contiene el descriptor del socket de 
	    datos establecido para la conexion de control por la que se enviara
	    el archivo.
	    char *nombre: Puntero a cadena que contiene el nombre del archivo.
	    int modo: Entero que indica si estamos en modo pasivo.
	    int fd: Descriptor a archivo que se enviara.
Descripcion: Funcion que envia los datos de un archivo. Devuelve los bytes enviados
	     por el socket.
Llama a:
Fecha ultima modificacion: 21-04-09 
Ultima modificacion: Anadidos comentarios.
-----------------------------------------------------------------------------------*/
int EnviarArchivo(int socket, char *nombre, int modo, int fd)
{
	char 	buffer[TAMBUFFER];  	// Cadena va almacenando los datos a enviar
	int	nread;  		// Tamaño de bytes leidos en cada llamada a read
	int 	n = 0; 			// Cantidad de bytes totales leidos.
	int 	bytesescritos = 0;	// Bytes escritos en total.
	int 	nleft;  		// Bytes que se han de enviar en el caso de leer \n
	char 	*ptr;  			// Puntero auxiliar para no perder la referencia
	char 	men[3] = "\r\n"; 	// Mensaje a enviar como retorno de carro.

	if (modo == 0) {
		/* Leemos los mensajes almacenados en el archivo de tambuffer en tambuffer y los enviamos por el socket */
		while( (nread=read(fd, buffer, 1)) != 0)	{
			if(nread<0)	{
				printf("Error en la lectura del archivo %s\n",nombre);
				perror("mitfp");
				return -1;
			}
			else	{
				/* Escribimos todo lo leido en el socket*/
				/* Cuando se envia en formato ascii hay que transformar los \n en \r\n porque
				estamos haciendo el programa para SO UNIX */
				if (*buffer == '\n') {
					nleft = 2;
					ptr = men; // Enviamos \r\n en lugar de \n
					while ( n < nleft) {
						n = write(socket, ptr,nleft);
						if(n < 0) {
							fprintf(stderr,"Error de write()\n");
							perror("miftp");
							return -1;
						} else if(n < nleft) {
							/* Si no fue posible enviar todo el mensaje 
							* de una vez, se transmite el resto */
							ptr += n;
							nleft -=n;
						}
						bytesescritos += n;
					}
				}
				else {
					// Caso en el que no hayamos leido \n
					n = write(socket, buffer, 1);
					if(n <= 0) {
						fprintf(stderr,"Error de write()\n");
						perror("miftp");
						exit(1);
					}
					bytesescritos += n;
				}
			}
		}
	}
	
	// Modo binario
	else if (modo == 1) {
		while( (nread=read(fd, buffer, TAMBUFFER)) != 0) {
			if(nread<0) {
				printf("Error en la lectura del archivo %s\n",nombre);
				return -1;
			}
			else {
				/* Escribimos todo lo le�do*/
				ptr = buffer;
				n = 0;
				while ( n < nread ) {
					ptr += n;
					nread -=n;
					n = write(socket, ptr, nread < TAMBUFFER ? nread : TAMBUFFER);
					if(n <= 0) {
						fprintf(stderr,"Error de write()\n");
						return -1;
					}
					bytesescritos += n;
				}
			}
		}
	}
	
	// Se ha hecho un mal uso de la funcion modo tiene que ser 1 o 0
	else {
		fprintf(stderr,"RecibirArchivo():Mal uso de la funcion\n");
		exit(0);
	}
	
	
	/* Cerramos el archivo*/
	if (close(fd) < 0 ) {
		fprintf(stderr,"Error al cerrar el archivo\n");
		perror("mitfp");
		exit(1);
	}
	return bytesescritos;
}

/*---------------------------------------------------------------------------------
Funcion: RecibirArchivo()
Sintaxis: int RecibirArchivo(int socket, char *nombre, int modo)
Prototipo en: miftp.h
Argumentos: int socket: Entero que contiene el descriptor del socket de 
	    datos establecido para la conexion de control por la que se enviara
	    el archivo.
	    char *nombre: Puntero a cadena que contiene el nombre del archivo.
	    int modo: Entero que indica si estamos en modo pasivo.
Descripcion: Funcion que envia los datos de un archivo. Devuelve los bytes escritos
	     en el archivo.
Llama a:
Fecha ultima modificacion: 21-04-09 
Ultima modificacion: Anadidos comentarios.
-----------------------------------------------------------------------------------*/
int RecibirArchivo(int socket, char *nombre, int modo)
{
	int	fd; 			 // descriptor de archivo creado al crear el archivo que queremos enviar
	char	buffer[TAMBUFFER]; 	 // Cadena va almacenando los datos a enviar
	char 	mensaje[2];		 // Cadena que se utiliza cuando /r y despues no se detecta /n
	int 	nread; 			 // Tamaño de bytes leidos en cada llamada a read
	int	n;  			 // Cantidad de bytes totales leidos.
	int 	bytesescritos = 0;  	 // Cantidad de bytes escritos en el archivo.
	int 	exito=0;		 // Entero que se usa para el control de los retornos de carro.

	// Si el archivo no existe lo creamos. Ademas asignamos todos los permisos posibles
	fd = open (nombre , O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO );	
	if (fd == -1) {
		fprintf(stderr,"Error de open()\n");
		perror("mitfp");
		return -1;
	}
	
	// Modo ascii
	if (modo == 0) {
		/*Leemos del socket y lo vamos almacenando en el archivo. Se va haciendo un control de datos
		a medida que se va leyendo para eliminar los \r*/
		while( (nread=read(socket, buffer, 1)) != 0) {
			if(nread<0) {
				printf("Error en la lectura del socket\n");
				perror("mitfp");
				return -1;
			}
			else {
				//Caso que comience detectando /r
				if (*buffer == '\r') {
					exito=1;
				}
				//Si detecta /n y el caracter previo era /r
				else if (*buffer=='\n' && exito==1) {
					exito=0;
					n = write(fd, "\n", 1);
					if(n <= 0)	
					{
						fprintf(stderr,"Error de write()\n");
						perror("miftp");
						return -1;
					}
					bytesescritos += n;
				}
				//Si no detecta /n pero el caracter previo era /r
				else if (exito==1 && *buffer!='\n')	{
					exito = 0;
					strcpy (mensaje, "\r");
					strcat (mensaje, buffer);
					n = write(fd, mensaje, 2);
					if(n <= 0)	{
						fprintf(stderr,"Error de write()\n");
						perror("miftp");
						return -1;
					}
					bytesescritos += n;
				}
				//Caso normal
				else	{
					n = write(fd, buffer, 1);
					if(n <= 0)	{
						fprintf(stderr,"Error de write()\n");
						perror("miftp");
						return -1;
					}
					bytesescritos += n;
				}
			}
		}
	}

	// Modo binario
	else if (modo == 1) {
		while( (n=read(socket, buffer, TAMBUFFER)) != 0) {
			if(n<1)	{
				printf("Error en lectura de socket...Saliendo!\n");
				return -1;
			} 
			else	{
				if (write(fd,buffer,n)<=0){
					fprintf(stderr,"Error al escribir en el fichero\n");
					return -1;
				}
				bytesescritos += n;
			}
		}
	}
	
	// Se ha hecho un mal uso de la funcion modo tiene que ser 1 o 0
	else {
		fprintf(stderr,"RecibirArchivo():Mal uso de la funci�n\n");
		exit(0);
	}
	/* Cerramos el archivo*/
	if (close(fd) < 0 ) {
		fprintf(stderr,"Error al cerrar el archivo\n");
		perror("mitfp");
		return -1;
	}
	return (bytesescritos);
}

/*---------------------------------------------------------------------------------
Funcion: MostrarArchivo()
Sintaxis: int MostrarArchivo(int socket)
Prototipo en: miftp.h
Argumentos: int socket: Entero que contiene el descriptor del socket de 
	    datos establecido para la conexion de control por la que se enviara
	    el archivo.
Descripcion: Funcion que recibe un archivo y se muestra por pantalla. Se usa para
	     el comando list.
Llama a:
Fecha ultima modificacion: 21-04-09 
Ultima modificacion: Anadidos comentarios.
-----------------------------------------------------------------------------------*/
int MostrarArchivo(int socket)
{
	char 	buffer[1];  		// Buffer de lectura de socket
	char 	mensaje[2];		// Cadena que se utiliza cuando /r y despues no se detecta /n
	int 	nread; 			// Tamaño de bytes leidos en cada llamada a read
	int 	n; 			// Cantidad de bytes totales leidos.
	int 	bytesescritos = 0;  	// Cantidad de bytes escritos en el archivo.
	int 	exito=0; 		// Entero que se usa para el control de los retornos de carro.

	/* Leemos del socket y lo vamos mostrando por pantalla. Se va haciendo un control de datos a medida que se va leyendo
	para eliminar los intros y ponerlo estilo unix */
	while( (nread=read(socket, buffer, 1)) != 0)		{
			if(nread<0)	{
				printf("Error en la lectura del socket\n");
				perror("mitfp");
				return -1;
			}
			else		{
				/*Mostramos por pantalla lo que vamos leyendo del socket. Buscamos final de linea estandar y se sustituye por el
				final de linea formato *unix*/
				
				//Caso que comience detectando /r
				if (*buffer == '\r') 	{
					exito=1;
				}
				//Si detecta /n y el caracter previo era /r
				else if (*buffer=='\n' && exito==1)	{
					exito=0;
					n =write(fileno(stdout),buffer,1);
					if(n <= 0)	
					{
						fprintf(stderr,"Error de fprintf()\n");
						perror("miftp");
						return -1;
					}
					bytesescritos += n;
				}
				//Si no detecta /n pero el caracter previo era /r
				else if (exito==1 && *buffer!='\n')	{
					exito = 0;
					strcpy (mensaje, "\r");
					strcat (mensaje, buffer);
					n =write(fileno(stdout),mensaje,2);
					if(n <= 0)	
					{
						fprintf(stderr,"Error de fprintf()\n");
						perror("miftp");
						return -1;
					}
					bytesescritos += n;
				}
				//Caso normal
				else	{
					n = write(fileno(stdout), buffer,1);
					if(n <= 0)	
					{
						fprintf(stderr,"Error de fprintf()\n");
						perror("miftp");
						return -1;
					}
					bytesescritos += n;
				}
			}
	}
	return (bytesescritos);
}

/*-------------------------------------------------------------------------------
Funcion: SetEstadoBinario()
Sintaxis: void SetEstadoBinario (int estado, int socket_control)
Prototipo en: miftp.h
Argumentos: int entero: Entero que indica el estado establecido de conexion
	    (ascii o binario).
	    int socket_control: Entero que contiene el descriptor del socket de 
	    control establecido para la conexion de control por la que se enviara
	    el comando.
Descripcion: Funcion que establece el modo de transferencia (ascii o pasivo).
Llama a: void EnviarComando(),
	 char * LeerReply(), 
	 int ControlReplicas().
Fecha ultima modificacion: 21-04-09 
Ultima modificacion: Anadidos comentarios.
--------------------------------------------------------------------------------*/
void SetEstadoBinario (int estado, int socket_control)
{
	char *mensaje;	//Puntero a cadena que contiene la replica del servidor.

	if (estado==0)	{
		EnviarComando(socket_control,"TYPE A\r\n");
	}

	else if (estado==1)	{
		EnviarComando(socket_control, "TYPE I\r\n");
	}
		
	// Leer replica e imprimirla por pantalla
	mensaje = LeerReply(socket_control);
	printf("%s",mensaje);
	
	// Control de replicas
	if (ControlReplicas(mensaje) != 2)	{
		fprintf(stderr,"Mensaje inesperado por parte del servidor\n");
	}
	free (mensaje);
	return;
}
