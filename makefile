FICHERO1 = miftp
FICHERO2 = funciones
FICHERO3 = comandos
FICHERO4 = colaboradoras


FICHEROS-CABECERA =	librerias.h miftp.h
FICHERO-EJECUTABLE =	$(FICHERO1)
CC = gcc
LIBS = -lnsl -lsocket -lgen

all:	$(FICHERO-EJECUTABLE)

$(FICHERO-EJECUTABLE):	$(FICHERO1).o $(FICHERO2).o $(FICHERO3).o $(FICHERO4).o
			$(CC) -g $(FICHERO1).o $(FICHERO2).o $(FICHERO3).o $(FICHERO4).o -o $@ $(LIBS)

$(FICHERO1).o:	$(FICHERO1).c $(FICHEROS-CABECERA)
	$(CC) -g -c $(FICHERO1).c

$(FICHERO2).o:	 $(FICHERO2).c $(FICHEROS-CABECERA)
	$(CC) -g -c $(FICHERO2).c

$(FICHERO3).o:	 $(FICHERO3).c $(FICHEROS-CABECERA)
	$(CC) -g -c $(FICHERO3).c

$(FICHERO4).o:   $(FICHERO4).c $(FICHEROS-CABECERA)
	$(CC) -g -c $(FICHERO4).c

clean:
	\rm *.o
	\rm $(FICHERO-EJECUTABLE)
