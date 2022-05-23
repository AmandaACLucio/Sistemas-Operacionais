#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

#define SUCESSO            0
#define ERRO               1

//Ubuntu 20.04.2 LTS
//Amanda Aparecida Cordeiro Lucio

//mutex
pthread_mutex_t cortandoCabelo = PTHREAD_MUTEX_INITIALIZER; //variavel para proteger área restrita -corte
pthread_mutex_t verificandoCadeira = PTHREAD_MUTEX_INITIALIZER; //só um pode verificar as cadeiras por vez
pthread_mutex_t verificandoBarbeiro = PTHREAD_MUTEX_INITIALIZER; //só um pode verificar o barbeiro por vez

pthread_cond_t clienteDisponivel = PTHREAD_COND_INITIALIZER; //signal de cliente disponível para corte
pthread_cond_t barbeiroLivre = PTHREAD_COND_INITIALIZER; //signal de barbeiro disponível para corte

int clientesEsperando=0; //quantidade de clientes esperando
int cadeirasBarbearia=0; //quantidade de cadeiras no estabelecimento
int numeroTotaldeClientesAtendidos=0; //numero de clientes atendidos para identificação do struct
int clienteSendoAtendido = 0; //id do cliente que será atendido
int numeroTotalClientes=0;

int qntClientes=0; //quantidade de clientes que será atendida

bool inicioTurno=true; //Para indicar primeira entrada no barbeiro
bool cortando=false; //variável para controle de corte ocorrendo


//define fila de clientes
typedef struct Cliente_Fila
{
	int idCliente;
	char nameCliente;
	pthread_cond_t atual;
	struct Cliente_Fila *proximo;
}Fila;


//adicionando novo cliente a fila
Fila *AdicionandoNovoClienteFila(Fila **inicio, int id, char name)
{
	//clia nova instância do struct
	Fila *novoClienteFila = malloc(sizeof(Fila));

	novoClienteFila->idCliente= id;
	novoClienteFila->nameCliente= name;
	novoClienteFila->proximo= NULL;
	
	//inicializando variável de condição
	pthread_cond_init(&novoClienteFila->atual, NULL);

	if (*inicio == NULL){
		(*inicio) = novoClienteFila;
	}else{
		Fila *temporaria = *inicio;

		//procura calda da fila
		while (temporaria->proximo != NULL){
			temporaria = temporaria->proximo;
		}

		//faz último termo da fila apontar para o novo inserido
		(temporaria->proximo) = novoClienteFila;
	}
	return novoClienteFila;
}


Fila *FilaBarbearia = NULL; //cliando nova instância do struct para iniciar Fila da barbearia


//função para o cliente
void *cliente(void *arg){

	//tem a variavél posição na fila
	int idCliente;
	char * name;

	pthread_mutex_lock(&verificandoCadeira); //trava para verificação

	name = (char *) arg;

	//tem vaga
	if(clientesEsperando<cadeirasBarbearia){
		
		numeroTotaldeClientesAtendidos++;
		clientesEsperando++;
		idCliente = numeroTotaldeClientesAtendidos;

		//tem vaga, então vai ser adicionado a fila
		Fila *cliente= AdicionandoNovoClienteFila(&FilaBarbearia, idCliente, *name);
		printf("O cliente %c, com id %d, está ocupando cadeira\n", *name, idCliente);

		//envia sinal de cliente Disponível, desbloqueando thread em barbeiro bloqueada pela
		//variavel de condição

		pthread_cond_signal(&clienteDisponivel);
		pthread_mutex_unlock(&verificandoCadeira); //destrava cadeiras


		//trava enquanto verifica se o barbeiro "chamou" e se for o próximo
		//sai da cadeiro (definindo em barbeiro)
		pthread_mutex_lock(&verificandoBarbeiro); //trava barbeiro
		while(!(clienteSendoAtendido==idCliente) && (numeroTotalClientes!=qntClientes)){

			pthread_cond_wait(&cliente->atual, &verificandoBarbeiro); //espera vez do atendimento

		}
		pthread_mutex_unlock(&verificandoBarbeiro); //destrava barbeiro

		//impede que outro cliente acesse o corte, enquanto o barbeiro corta
		pthread_mutex_lock(&cortandoCabelo); //travando corte
		while(cortando){

			pthread_cond_wait(&barbeiroLivre, &cortandoCabelo);
		}

		pthread_mutex_unlock(&cortandoCabelo); //destravando corte

		return NULL;		

	//não tem vaga
	}else{
		numeroTotalClientes++;
		printf("Numero de vagas cheia, cliente %c foi embora\n", *name);
		pthread_mutex_unlock(&verificandoCadeira);
		
		//finaliza thread
		pthread_exit(0);
		return NULL;		
	}
}

void *barbeiro(void *arg){


	int tempoDeCorte=0;

	if(inicioTurno){

		printf("Estabelecimento do seu Zé está aberto...Será que alguém virá hoje?\n");
	}

	inicioTurno=false;

	while(1){

		//bloqueia cadeira
		pthread_mutex_lock(&verificandoCadeira); //verifica cadeira para definir próximo
				
		//espera enquanto não tem ninguém e o número de clientes que passaram pelo estabelecimento
		//não é igual ao esperado

		while(clientesEsperando==0 && (numeroTotalClientes!=qntClientes)){

			clienteSendoAtendido=0;
			pthread_cond_wait(&clienteDisponivel, &verificandoCadeira); //espera sinal de cliente disponivel
		}

		clientesEsperando--;

		//cliente que a fila aponta no momento
		Fila *cliente = FilaBarbearia;

		FilaBarbearia = FilaBarbearia->proximo; //retira primeiro da fila

		printf("O cliente %c se levantou, pois será o próximo atendido\n",cliente->nameCliente);

		pthread_mutex_unlock(&verificandoCadeira);

		//espera sinal da variável de condição
		pthread_cond_signal(&cliente->atual);

		pthread_mutex_lock(&cortandoCabelo); //inicia corte

		cortando = true;

		//sorteia tempoDeCorte
		srand(time(NULL));
        tempoDeCorte = rand() % 10;

		sleep(tempoDeCorte);

		printf("Barbeiro está cortando o cabelo do %c\n",cliente->nameCliente);

		cortando = false;

		printf("Cabelo  do %c cortado!\n", cliente->nameCliente);

		numeroTotalClientes++;

		//atingiu a meta, logo sai do loop e finaliza
		if(qntClientes==numeroTotalClientes){
			printf("Estabelecimento Fechado! O barbeiro atendeu %d clientes e %d clientes foram embora\n", numeroTotaldeClientesAtendidos, qntClientes-numeroTotaldeClientesAtendidos);
			_exit(0);
		}

		//após terminar corte, envia sinal de que está livre, desbloqueando a thread do próximo cliente
		pthread_cond_signal(&barbeiroLivre);

		pthread_mutex_unlock(&cortandoCabelo); //termina corte

	}

	return NULL;
}


int main() {

	//declarando variáveis para threads
	printf("Qual a quantidade de cliente prevista para hoje? ");
	scanf("%d", &qntClientes);
	printf("\nQuantas cadeiras tem o estabelecimento? ");
	scanf("%d", &cadeirasBarbearia);
	printf("\n");

	int tempoDeChegada, reiniciaLista;
	pthread_t barbeiroThr, *clientesThr;

	//identificando clientes
	char nomes[26] = {'A', 'B', 'C', 'D','F', 'G', 'H', 'I','J','K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};	


	//alocando threads de cliente
	clientesThr = (pthread_t *)malloc (qntClientes * sizeof (pthread_t));	

	//Cria as threads

	pthread_create(&barbeiroThr,NULL,barbeiro,NULL);

	for(int i=0; i<qntClientes; i++){

		srand(time(NULL));
        tempoDeChegada = rand() % 2+1;

		//quando usar todos os nomes, reinicia novamente
		reiniciaLista = i%26;

		pthread_create(&clientesThr[i],NULL,cliente,&nomes[reiniciaLista]);
		sleep(tempoDeChegada);
	}

	//Sicronizando threads
	pthread_join(barbeiroThr,NULL);

	//loop para sicronizar clientes no vetor de threads de clientes
	for(int i=0; i<qntClientes; i++){
		pthread_join(clientesThr[i],NULL);
	}

	return SUCESSO;
}