 /**************************************************************************/
/*                                                                        */
/* P1 - MI amb sockets TCP/IP - Part I                                    */
/* Fitxer ser.c que implementa el servidor de MI sobre la capa de         */
/* transport TCP (fent crides a la "nova" interfície de la capa de        */
/* transport o "nova" interfície de sockets).                             */
/* Autors: Lluc Pagés Pérez, Joel Carrasco Mora                           */
/*                                                                        */
/**************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "MIp1v4-mi.h"

//Missatge per acabar la conversa
const char finish[5] = "exit";

//Demana nickname per pantalla
int llegir_nickname(char *str);

//Demana per pantalla ip i port Desitjat
int llegir_portTCP(int *portTCP);

//Invercanvia nickname
int intercanviNickname(int scon, char * nickname, int type);

//Obtenir ip
int getIpAddr(char * ip);

//Main
int main(int argc,char *argv[]){
	//Variables
	int sesc, scon, sck;

	char nick_loc[303], nick_rem[303], buffer[303];
	int nick_long = 303, buff_long = 303;

	char ip_loc[16] = "0.0.0.0", ip_rem[16];
	int port_loc, port_rem;

	char acabar = ' ';

	/* -------  Funcions -------- */

	//Llegim nickname
	nick_long = llegir_nickname((char *)&nick_loc);
	if(nick_long == -1){
		perror("Error en el llegir_nickname");
		exit(-1);
	}

	while(acabar != 'S'){
		//Tornem a inicialitzar
		nick_long = 303;
		buff_long = 303;
		strcpy(buffer, " ");

		getIpAddr((char *)&ip_loc);
		printf("Ip local: %s\n", ip_loc);

		//Creem el socket servidor
		sesc = -1;
		port_loc = 1499;
		while(sesc == -1 && port_loc < 65535){ //Cercar port TCP lliure
			port_loc++;
			sesc = MI_IniciaEscPetiRemConv(port_loc);
		}
		if(sesc == -1 && port_loc == 65535){ //No s'ha trobat cap port TCP lliure
			perror("Error en el MI_IniciaEscPetiRemConv");
			MI_AcabaEscPetiRemConv(sesc);
			exit(-1);
		}

		//Escoltar peticions
		sck = MI_HaArribatPetiConv(sesc);
		if(sck == 0){
			MI_AcabaEscPetiRemConv(sesc);
			printf("Entra IP remota: ");
			scanf("%s",ip_rem);
			llegir_portTCP(&port_rem);
			scon = MI_DemanaConv((char *)ip_rem, port_rem, (char *)&ip_loc, &port_loc, (char *)&nick_loc, (char *)&nick_rem);
			if(scon == -1){
				perror("Error en el MI_DemanaConv");
				MI_AcabaConv(scon);
				exit(-1);
			}
		} //Client
		else if(sck == -1){
			MI_AcabaEscPetiRemConv(sesc);
			exit(-1);
		} //Error
		else{
			scon = MI_AcceptaConv(sesc, (char *)ip_rem, &port_rem, (char *)&ip_loc, &port_loc, (char *)&nick_loc, (char *)&nick_rem);
			if(scon == -1){
				perror("Error en el MI_AcceptaConv");
				MI_AcabaEscPetiRemConv(sesc);
				MI_AcabaConv(scon);
				exit(-1);
			}
			MI_AcabaEscPetiRemConv(sesc);
		} //Accepta

		printf("\n------- XAT -------\n");

		while (buff_long != -2 && strcmp(buffer, finish) != 0) {
			sck = MI_HaArribatLinia(scon);
			memset(buffer, 0, 303);
			if(sck == 0){
				buffer[read(0, buffer, 299) - 1] = '\0';
				if(strcmp(buffer, finish) == 0) printf("------ FI XAT ------\n"), MI_AcabaConv(scon);
				else MI_EnviaLinia(scon, buffer);
			} //Envia
			else if(sck != -1){
				buff_long = MI_RepLinia(scon, buffer);
				if(buff_long != -2){
					write(1, nick_rem, strlen(nick_rem));
					write(1, ": ", 2);
					write(1, buffer, buff_long);
				}
				else{
					printf("Usuari remot desconnectat\n");
					printf("------ FI XAT ------");
				}
				printf("\n");
			} //Rep
			else{
				MI_AcabaConv(scon);
				exit(-1);
			} //Error
		}

		acabar = ' ';
		printf("\nVols tancar l'aplicació? [S/n]\n");
		while(acabar != 'S' && acabar != 'n') scanf(" %c",&acabar);
	}

}

//Codi funcions

int llegir_nickname (char *str){
	///Seleccionar nickname
	printf("Enter Nickname: ");
	gets(str);
	return strlen(str);
}

int llegir_portTCP(int *portTCP){
	///Seleccionar port TCP
	printf("Set TCP port: ");
	scanf("%d", portTCP);
	return 0;
}

int getIpAddr(char * ip){

	FILE *f;
	char line[100] , *p , *c;

	int fd;
	struct ifreq ifr;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;

	f = fopen("/proc/net/route" , "r");

	while(fgets(line , 100 , f))
	{
		p = strtok(line , " \t");
		c = strtok(NULL , " \t");

		if(p!=NULL && c!=NULL)
		{
			if(strcmp(c , "00000000") == 0)
			{
				strncpy(ifr.ifr_name, p, IFNAMSIZ-1);
				ioctl(fd, SIOCGIFADDR, &ifr);
				close(fd);
				stpcpy(ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
				break;
			}
		}
	}

  return 0;
}
