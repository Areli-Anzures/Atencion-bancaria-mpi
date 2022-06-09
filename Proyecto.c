/*Areli Azures Villarreal 		10/Marzo/2020
	Proyecto 2, Programa que simula un sistema de
	atencion bancaria donde hay N clientes, una sala 
	de espera y 3 ejecutivos, esto fue realizado con 
	procesos y la libreria de mpi.h*/

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

#define MAX 10
//Etiquetas para el envio y recepcion de datos
#define Sala 0

#define HayLugar 0
#define Hlugar 1
#define PideTurno 2
#define DaTurno 3
#define HayCliente 4
#define Hcliente 5
#define DaCliente 6
#define ClieAtendido 7
#define CLIENTE 8
#define Listo 9
#define PasaAtencion 10
#define TERMINA 11

//metodos 
void SalaEspera(int id, int size);
void Ejecutivo(int id, int size);
void Clientes(int id, int size);

int main(int argc, char *argv[])
{
    int id, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    // printf("%d  id\n", id); 
    MPI_Barrier(MPI_COMM_WORLD);
	//el proceso cero es la sala de espera
    if(id==0)
        SalaEspera(id, size);
	//los procesos 1,2 y 3 son los ejecutivos
    else if(id==1 || id==2 || id==3)
        Ejecutivo(id, size);
	//todos los demas son clientes
    else 
        Clientes(id, size);        
    MPI_Finalize();
    return 0;
}

//Sala de espera
void SalaEspera(int id, int size)   
{
    int cliente[3];
    int BufferEspera[MAX][3];
    int termina=0;
    int idEmisor;
    int siguiente=0;
    int frente=0;
    int lugar=0;
    int turno=0;
    int x, uno=1, cero=0;
    MPI_Status reporte;
    MPI_Request request;
    // printf("Abriendo sala\n");
    for(int i=1; i < size; i++)
    {
        MPI_Send(&x, 1, MPI_INT, i, Listo, MPI_COMM_WORLD);
    }
    // printf("Sala abierta\n");
    while(termina<(size-4))
    {
        MPI_Recv(&idEmisor, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &reporte);
		//Si el mensaje fue de un cliente, esta pidiendo un lugar
        if(reporte.MPI_TAG==HayLugar)
        {
            // si hay lugar en la sala de espera se asigna un lugar
            if(lugar<MAX)
            { // envia un mensaje indicando que si hubo lugar
                MPI_Isend(&uno, 1, MPI_INT, idEmisor, Hlugar, MPI_COMM_WORLD, &request);
                MPI_Recv(&idEmisor, 3, MPI_INT, idEmisor, PideTurno, MPI_COMM_WORLD, &reporte);
                MPI_Send(&turno, 1, MPI_INT, idEmisor, DaTurno, MPI_COMM_WORLD);
                BufferEspera[siguiente][0]=idEmisor;
                BufferEspera[siguiente][1]=turno;
                BufferEspera[siguiente][2]=0;
                
                siguiente=(siguiente+1)%MAX;
                turno++;
                lugar++;
            }
            else {// si no hay lugar envia un mensaje indicando que no hay lugar
                // printf("%d No hay lugar\n", reporte.MPI_SOURCE);
                MPI_Send(&cero, 1, MPI_INT, idEmisor, Hlugar, MPI_COMM_WORLD); 
            }
        } //si el mensaje recibido es de un ejecutivo, esta pidiendo cliente
        else if(reporte.MPI_TAG==HayCliente)
        {
            // si hay clientes esperando, se le asigna un cliente alejecutivo
            if(lugar>0)
            {   
                // se envia un mensaje diciendo que si hay clientes
                MPI_Send(&uno, 1, MPI_INT, idEmisor, Hcliente, MPI_COMM_WORLD); 
                //MPI_Recv(&idEmisor, 1, MPI_INT, idEmisor, DaCliente, MPI_COMM_WORLD, &reporte);
                cliente[0]=BufferEspera[frente][0];
                cliente[1]=BufferEspera[frente][1];
                cliente[2]=idEmisor;
                frente=(frente+1)%MAX;
                MPI_Send(cliente, 3, MPI_INT, idEmisor, CLIENTE, MPI_COMM_WORLD); 
                termina++;
				//envia un mensaje para que los procesos de los ejecutivos finalizen correctamente
                MPI_Send(&termina, 1, MPI_INT, idEmisor, TERMINA, MPI_COMM_WORLD); 
                lugar--;
            }
            else //si no hay clientes envia un mensaje para decir que no hay clientes por atender
                 MPI_Send(&cero, 1, MPI_INT, idEmisor, Hcliente, MPI_COMM_WORLD); 
        }
   }
   // printf("Esperando desinformados\n");
   for(int i = 0; i < 3; i++)
   {
		//espera a los ejecutivos que aun no saben que ya se atendieron todos los clientes
		//y les avisa que ya no hay mas clientes por atender
        MPI_Recv(&idEmisor, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &reporte);
        uno = -1;
        MPI_Send(&uno, 1, MPI_INT, idEmisor, Hcliente, MPI_COMM_WORLD); 
   }
}

//para los ejecutivos
void Ejecutivo(int id, int size)
{
    
    int termina=0;
    int cliente[3];
    int x ,uno=1;
    int cAtendidos = 0;
    MPI_Status reporte;
    MPI_Request request;
	//espera a que la sala de espera le indique que ya esta abierta
    MPI_Recv(&x, 1, MPI_INT, Sala, Listo, MPI_COMM_WORLD,&reporte);
    while(termina<(size-4))
    {
        do
        {
            //pregunta por cliente para atender, si hay cliente sale del while 
            MPI_Send(&id, 1, MPI_INT, Sala, HayCliente, MPI_COMM_WORLD);
            MPI_Recv(&x, 1, MPI_INT, Sala, Hcliente, MPI_COMM_WORLD,&reporte);
        } while(x==0);
        if(x==1){ //solicita su cliente para atender
            // printf("Pido cliente\n");
            MPI_Send(&id, 1, MPI_INT, Sala, DaCliente, MPI_COMM_WORLD); 
            MPI_Recv(cliente, 3, MPI_INT, Sala, CLIENTE, MPI_COMM_WORLD,&reporte);
            MPI_Recv(&termina, 1, MPI_INT, Sala, TERMINA, MPI_COMM_WORLD,&reporte);
			//llama a su cliente y lo contacta directamente 
            printf("    Llamo al cliente %d a la ventanilla %d\n", cliente[1], id);
            MPI_Send(&id, 1, MPI_INT, cliente[0], PasaAtencion, MPI_COMM_WORLD); 
            MPI_Recv(&x, 1, MPI_INT, cliente[0], Listo, MPI_COMM_WORLD, &reporte);
            // printf("Soy el ejecutivo %d y atiendo al cliente con turno: %d (%d)\n", id, cliente[1], termina);
            printf("\t **Soy el ejecutivo %d y atiendo al cliente con turno: %d\n", id, cliente[1]);
            sleep(rand()%7);
            cAtendidos++;
        } else {
            termina=size-4;
        }
        
        //imprimen el numero de clientes que atendieron 
    }
    printf("--->Termine soy el ejecutivo %d, atendi %d clientes\n", id, cAtendidos);
    if(id == 1) //solo el ejecutivo 1 imprime el total de clientes atendidos
    {
        printf("--->El total de clientes atendidos fue: %d\n", termina);
    }
}

//para los clientes
void Clientes(int id, int size)
{
    int x, idEjec, turno;
    MPI_Status reporte;
    MPI_Request request;
    int uno=1;
	//espera el mensaje de la sala a que este lista
    MPI_Recv(&x, 1, MPI_INT, Sala, Listo, MPI_COMM_WORLD,&reporte);
    do
    { //pregunta si hay lugar en la sala, si hay, sale del while
        MPI_Send(&id, 1, MPI_INT, Sala, HayLugar, MPI_COMM_WORLD);
        MPI_Recv(&x, 1, MPI_INT, Sala, Hlugar, MPI_COMM_WORLD,&reporte);
    } while(!x);
    // printf("%d Pido turno\n",id);
    MPI_Send(&id, 1, MPI_INT, Sala, PideTurno, MPI_COMM_WORLD); 
	//recibe su turno y espera el llamado del ejecutivo
    MPI_Recv(&turno, 1, MPI_INT, Sala, DaTurno, MPI_COMM_WORLD,&reporte);
    MPI_Recv(&idEjec, 1, MPI_INT, MPI_ANY_SOURCE, PasaAtencion, MPI_COMM_WORLD, &reporte);
    MPI_Send(&uno, 1, MPI_INT, idEjec, Listo, MPI_COMM_WORLD);
    printf("++Soy el cliente con turno %d y me atiende el ejecutivo %d\n", turno, idEjec);
}









