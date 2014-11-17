/************************************************************************ 
Fichero: miftp.c 
Programa: miftp
Otros ficheros del programa:  miftp.c, funciones.c, comandos.c, 
			      colaboradoras.c, miftp.h
Version: 1.5
Fecha: 29-03-2009 
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
extern int errno;

/*--------------------------------------------------------------------------
Funcion: main()
Sintaxis: int main(int argc, char **argv)
Prototipo en: miftp.h
Argumentos: int argc: Entero que contiene el numero de argumentos introduci-
	    dos al ejecutar el programa.
	    char **argv: "puntero" a un vector que contiene punteros a los 
	    argumentos (tipo cadena) que se han introducido al ejecutar el
	    programa.
Descripcion: Funcion principal del programa. Realiza los pasos mas primarios
	     que ocurren en la comunicacion ftp. Tambien realiza un control
	     del numero de argumentos introducidos por la linea de comandos.
Llama a: AbreConexionControl(), Autenticacion(), Interfaz(). 
	 CierraConexionControl(), Inicializacion().
Fecha ultima modificacion: 01-04-09 
Ultima modificacion: Anadido Inicializacion(). 
---------------------------------------------------------------------------*/

int main(int argc, char **argv)	 {
	int socket_control;
	
	if (argc != 2)	{
		fprintf(stderr,"Uso: miftp <servidor>\n");
		exit(1);
	}
	
	socket_control=AbreConexionControl(argv[1]);
	
	Autenticacion(socket_control);
	
	Inicializacion(socket_control);
	
	Interfaz(socket_control);
	
	CierraConexionControl(socket_control);
	
	// Finaliza el programa con 0
	exit (0);
}	
