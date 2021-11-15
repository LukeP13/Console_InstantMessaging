/**************************************************************************/
/*                                                                        */
/* P1 - MI amb sockets TCP/IP - Part I                                    */
/* Fitxer mi.c que implementa la capa d'aplicació de MI, sobre la capa de */
/* transport TCP (fent crides a la "nova" interfície de la capa TCP o     */
/* "nova" interfície de sockets)                                          */
/* Autors: Lluc Pagés Pérez, Joel Carrasco Mora                           */
/*                                                                        */
/**************************************************************************/

/* Inclusió de llibreries, p.e. #include <sys/types.h> o #include "meu.h" */
/*  (si les funcions externes es cridessin entre elles, faria falta fer   */
/*   un #include del propi fitxer .h)                                     */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "MIp1v4-t.h"

/* Definició de constants, p.e., #define XYZ       1500                   */

/* Declaració de funcions INTERNES que es fan servir en aquest fitxer     */
/* (les seves definicions es troben més avall) per així fer-les conegudes */
/* des d'aqui fins al final de fitxer.                                    */

//Convertir info. en PMI
char * convertirPMI(const char *buffer, char type);

//Convertir PMI en info.
int desconvertirPMI(char *buffer);

//Saber el nombre de bytes de la info.
int bytesInfo(const char *buffer);

//Mostra IP i portTcp local i remot
int mostraIpPorts(const char *IPrem, const int *portTCPrem, const char *IPloc, const int *portTCPloc);



/* Inicia l’escolta de peticions remotes de conversa a través d’un nou    */
/* socket TCP, que té una @IP local qualsevol i el #port “portTCPloc”     */
/* (és a dir, crea un socket “servidor” o en estat d’escolta – listen –). */
/* Retorna -1 si hi ha error; l’identificador del socket d’escolta de MI  */
/* creat si tot va bé.                                                    */
int MI_IniciaEscPetiRemConv(int portTCPloc)
{
	int sesc;
	char *ip_loc = "0.0.0.0";
	sesc = TCP_CreaSockServidor(ip_loc, portTCPloc);
	return sesc;
}

/* Escolta indefinidament fins que arriba una petició local de conversa   */
/* a través del teclat o bé una petició remota de conversa a través del   */
/* socket d’escolta de MI d’identificador “SckEscMI” (un socket           */
/* “servidor”).                                                           */
/* Retorna -1 si hi ha error; 0 si arriba una petició local; SckEscMI si  */
/* arriba una petició remota.                                             */
int MI_HaArribatPetiConv(int SckEscMI)
{
	return T_HaArribatAlgunaCosa(NULL, SckEscMI);
}

/* Crea una conversa iniciada per una petició local que arriba a través   */
/* del teclat: crea un socket TCP “client” (en un #port i @IP local       */
/* qualsevol), a través del qual fa una petició de conversa a un procés   */
/* remot, el qual les escolta a través del socket TCP ("servidor") d'@IP  */
/* “IPrem” i #port “portTCPrem” (és a dir, crea un socket “connectat” o   */
/* en estat establert – established –). Aquest socket serà el que es farà */
/* servir durant la conversa.                                             */
/* Omple “IPloc*” i “portTCPloc*” amb, respectivament, l’@IP i el #port   */
/* TCP del socket del procés local.                                       */
/* El nickname local “NicLoc” i el nickname remot són intercanviats amb   */
/* el procés remot, i s’omple “NickRem*” amb el nickname remot. El procés */
/* local és qui inicia aquest intercanvi (és a dir, primer s’envia el     */
/* nickname local i després es rep el nickname remot).                    */
/* "IPrem" i "IPloc*" són "strings" de C (vectors de chars imprimibles    */
/* acabats en '\0') d'una longitud màxima de 16 chars (incloent '\0').    */
/* "NicLoc" i "NicRem*" són "strings" de C (vectors de chars imprimibles  */
/* acabats en '\0') d'una longitud màxima de 300 chars (incloent '\0').   */
/* Retorna -1 si hi ha error; l’identificador del socket de conversa de   */
/* MI creat si tot va bé.                                                 */
int MI_DemanaConv(const char *IPrem, int portTCPrem, char *IPloc, int *portTCPloc, const char *NicLoc, char *NicRem)
{
	int scon = TCP_CreaSockClient(IPloc, *portTCPloc);

	if(scon != -1){
		int demana = TCP_DemanaConnexio(scon, (char *)IPrem, portTCPrem);
		if(demana == -1) scon = -1; //Error al demanar connexio
		else{
			int nicLong = 303;
			char *nickPMIloc = convertirPMI(NicLoc, 'N');
			if(TCP_Envia(scon, nickPMIloc, strlen(nickPMIloc)) != -1){
				write(1, "Nick connectat: ", 17);

				if(TCP_Rep(scon, NicRem, nicLong) != -1){
					nicLong = bytesInfo(NicRem);
					desconvertirPMI(NicRem);
					write(1, NicRem, nicLong);

					if(TCP_TrobaAdrSockLoc(scon, IPloc, portTCPloc) != -1)
						mostraIpPorts(IPrem, &portTCPrem, IPloc, portTCPloc);
				} else scon = -1; //Error al rebre nickname
			} else scon = -1; //Error al enviar el nickname
		}
	}

	return scon;
}

/* Crea una conversa iniciada per una petició remota que arriba a través  */
/* del socket d’escolta de MI d’identificador “SckEscMI” (un socket       */
/* “servidor”): accepta la petició i crea un socket (un socket            */
/* “connectat” o en estat establert – established –), que serà el que es  */
/* farà servir durant la conversa.                                        */
/* Omple “IPrem*”, “portTCPrem*”, “IPloc*” i “portTCPloc*” amb,           */
/* respectivament, l’@IP i el #port TCP del socket del procés remot i del */
/* socket del procés local.                                               */
/* El nickname local “NicLoc” i el nickname remot són intercanviats amb   */
/* el procés remot, i s’omple “NickRem*” amb el nickname remot. El procés */
/* remot és qui inicia aquest intercanvi (és a dir, primer es rep el      */
/* nickname remot i després s’envia el nickname local).                   */
/* "IPrem*" i "IPloc*" són "strings" de C (vectors de chars imprimibles   */
/* acabats en '\0') d'una longitud màxima de 16 chars (incloent '\0').    */
/* "NicLoc" i "NicRem*" són "strings" de C (vectors de chars imprimibles  */
/* acabats en '\0') d'una longitud màxima de 300 chars (incloent '\0').   */
/* Retorna -1 si hi ha error; l’identificador del socket de conversa      */
/* de MI creat si tot va bé.                                              */
int MI_AcceptaConv(int SckEscMI, char *IPrem, int *portTCPrem, char *IPloc, int *portTCPloc, const char *NicLoc, char *NicRem)
{
	int scon = TCP_AcceptaConnexio(SckEscMI, IPrem, portTCPrem);
	if(scon != -1){
		int nicLong = 303;
		char *nickPMIloc = convertirPMI(NicLoc, 'N');

		write(1, "Nick connectat: ", 17);
		nicLong = TCP_Rep(scon, NicRem, nicLong);
		if(nicLong != -1){
			nicLong = bytesInfo(NicRem);
			desconvertirPMI(NicRem);
			write(1, NicRem, nicLong); //Mostrem el nick a partir del 4t caracter per amagar part PMI

			if(TCP_Envia(scon, nickPMIloc, strlen(nickPMIloc)) != -1){ //Enviem Nickname
				int res = TCP_TrobaAdrSockLoc(scon, IPloc, portTCPloc);
				if(res != -1) res = TCP_TrobaAdrSockRem(scon, IPrem, portTCPrem);
				if(res != -1) mostraIpPorts(IPrem, portTCPrem, IPloc, portTCPloc);
				else scon = -1; //Error al trobar adreçes remota i local
			}
			else scon = -1; //Eror al rebre el nickname
		}
		else scon = -1; //Error al enviar el nickname local
	}

	return scon;
}

/* Escolta indefinidament fins que arriba una línia local de conversa a   */
/* través del teclat o bé una línia remota de conversa a través del       */
/* socket de conversa de MI d’identificador “SckConvMI” (un socket        */
/* "connectat”).                                                          */
/* Retorna -1 si hi ha error; 0 si arriba una línia local; SckConvMI si   */
/* arriba una línia remota.                                               */
int MI_HaArribatLinia(int SckConvMI)
{
	return T_HaArribatAlgunaCosa(NULL, SckConvMI);
}

/* Envia a través del socket de conversa de MI d’identificador            */
/* “SckConvMI” (un socket “connectat”) la línia “Linia” escrita per       */
/* l’usuari local.                                                        */
/* "Linia" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0'), no conté el caràcter fi de línia ('\n') i té una longitud       */
/* màxima de 300 chars (incloent '\0').                                   */
/* Retorna -1 si hi ha error; el nombre de caràcters n de la línia        */
/* enviada (sense el ‘\0’) si tot va bé (0 <= n <= 299).                  */
int MI_EnviaLinia(int SckConvMI, const char *Linia)
{
	int n;
	char * pmi = convertirPMI(Linia, 'L');
	n = TCP_Envia(SckConvMI, pmi, strlen(pmi));
	if(n != -1) n -= 4;
	return n;
}

/* Rep a través del socket de conversa de MI d’identificador “SckConvMI”  */
/* (un socket “connectat”) una línia escrita per l’usuari remot, amb la   */
/* qual omple “Linia”, o bé detecta l’acabament de la conversa per part   */
/* de l’usuari remot.                                                     */
/* "Linia*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0'), no conté el caràcter fi de línia ('\n') i té una longitud       */
/* màxima de 300 chars (incloent '\0').                                   */
/* Retorna -1 si hi ha error; -2 si l’usuari remot acaba la conversa; el  */
/* nombre de caràcters n de la línia rebuda (sense el ‘\0’) si tot va bé  */
/* (0 <= n <= 299).                                                       */
int MI_RepLinia(int SckConvMI, char *Linia)
{
	int n = 303;
	n = TCP_Rep(SckConvMI, Linia, n);
	if(n == 0) n = -2;
	else if(n != -1){
		n = bytesInfo(Linia);
		desconvertirPMI(Linia);
	}
	return n;
}

/* Acaba la conversa associada al socket de conversa de MI                */
/* d’identificador “SckConvMI” (un socket “connectat”).                   */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int MI_AcabaConv(int SckConvMI)
{
	return TCP_TancaSock(SckConvMI);
}

/* Acaba l’escolta de peticions remotes de conversa que arriben a través  */
/* del socket d’escolta de MI d’identificador “SckEscMI” (un socket       */
/* “servidor”).                                                           */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int MI_AcabaEscPetiRemConv(int SckEscMI)
{
	return TCP_TancaSock(SckEscMI);
}

/* Si ho creieu convenient, feu altres funcions EXTERNES                  */

/* Definicio de funcions INTERNES, és a dir, d'aquelles que es faran      */
/* servir només en aquest mateix fitxer.                                  */

char * convertirPMI(const char *buffer, char type){
	int longitud = strlen(buffer); //Longitud del buffer
	char *pmi = malloc(longitud + 4); //Fem espai per crear el PMI
	pmi[0]=type; //Guardem el tipus 'N' nickname o 'L' linia
	int i = 3;
	for(i; i > 0; i--){ //Guardem la longitud en bytes de la info.
		if(longitud > 0){
			pmi[i] = (longitud % 10) + '0';
			longitud /= 10;
		}
		else pmi[i] = '0';
	}
	strcpy(pmi + 4, buffer); //Guardem la info.
	return pmi;
}

int desconvertirPMI(char *buffer){
	memcpy(buffer, buffer+4, 299);
}

int bytesInfo(const char *buffer){
	char longitud[3];
	longitud[0] = buffer[1];
	longitud[1] = buffer[2];
	longitud[2] = buffer[3];
	int n = atoi(longitud); //Passem de char a integer
	return n;
}

int mostraIpPorts(const char *IPrem, const int *portTCPrem, const char *IPloc, const int *portTCPloc){
	printf("\n"); //Mostrem direccio IP local i remot
	printf("Local server Adress-> @IP: %s, TCP port: %d\n", IPloc, *portTCPloc);
	printf("Remote server Adress-> @IP: %s, TCP port: %d\n", IPrem, *portTCPrem);
}
