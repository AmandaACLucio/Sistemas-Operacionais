#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#define ERRO               1

//Ubuntu 20.04.2 LTS
//Amanda Aparecida Cordeiro Lucio

int Tarefa1 (int *comandoParaExecutar) {

    pid_t pid;

    int descritoresPipe[2], numeroAleatorio=0;
    //descritoresPipe[0] - leitura do pipe
    //descritoresPipe[1] - escrita no pipe

    //Criando o pipe e colocando em um if para caso gere erro
    if(pipe(descritoresPipe)){
        printf ("Falha na criação do Pipe.\n");
        return ERRO;
    }

    //Para gerar um novo processo é realizado um fork
    //if para detectar falhas

    pid = fork ();
    
    if (pid < 0){
        printf ("Falha no fork.");
        return ERRO;
    }

    //Trecho que o filho executa
    else if(pid==0){

        srand(time(NULL));
        numeroAleatorio = rand() % 100;
        printf("O número Aleatório gerado é %d \n", numeroAleatorio);
        
        //Escreve para o pai o número gerado informando seu tamanho
        write (descritoresPipe[1],&numeroAleatorio, sizeof(numeroAleatorio));

        //Processo filho fecha ponta de leitura
        close (descritoresPipe[0]);

        _exit(0);

    }
    //Trecho que o pai executa
    else{

        // Proceso pai fecha ponta aberta do pipe
        close (descritoresPipe[1]);

        // Lê o que filho escreveu através do pipe
        read(descritoresPipe[0], &numeroAleatorio, sizeof(numeroAleatorio));
        *comandoParaExecutar=numeroAleatorio;

        printf("O número Aleatório lido é %d \n", numeroAleatorio);

        wait(NULL); //esperando filho
        return 0;
    }
}

int Tarefa2(int *comandoParaExecutar){

    pid_t pidDoFilho = fork(); //Cria-se um filho como uma cópia completa do código do pai

    //Trecho que o filho executa
    if (pidDoFilho == 0) {

        //se o SIGUSR1 não foi chamado ainda
        if ((*comandoParaExecutar)==0){

            printf("Não há comando a executar \n");
            _exit(0);

        //Ping para o Google
        }else if(*comandoParaExecutar!=0 && *comandoParaExecutar%2==0){
            
            execlp("/bin/ping","ping","8.8.8.8","-c","5",NULL);        
        
        //Ping para Paris.testdebit.info
        }else if(*comandoParaExecutar%2!=0){

            execlp("/bin/ping","ping","paris.testdebit.info","-c","5","-i","2",NULL);        
        }

        return 0;

    //Trecho que o pai executa
    } else {

        wait(NULL); //esperando a finalização do filho
        return 0;
    }
}


int main(){

    pid_t pid;
    pid = getpid(); //getpid retorna pid do processo chamado
    printf("Pid é %d \n",pid); //exibindo pid

    sigset_t sigset; //máscara (set) do tipo sigset_t que será passada como parâmetro para bloqueadores

    
    int comandoParaExecutar, opcaoWhile, sinalEsperado, sinal;
    int *sinalptr = &sinal; //ponteiro utilizado em sigwait para receber o valor do sinal recebido

    comandoParaExecutar=0;
    opcaoWhile=0;

    //esvaziando máscara
    sigemptyset(&sigset);

    //adicionado na máscara para sigwait bloquear
    sigaddset(&sigset, SIGUSR1);
    sigaddset(&sigset, SIGUSR2);
    sigaddset(&sigset, SIGTERM);

    //sigprocmask permite continuar processo msm após receber um dos sinais
    sigprocmask( SIG_BLOCK, &sigset, NULL );

    while(opcaoWhile !=3){

        //variável registra sinal recebido e bloqueado
        sinalEsperado = sigwait(&sigset, sinalptr);

        if(sinalEsperado == -1){
            printf("Erro em sigwait\n");
            return 1;
        }else {
            //SIGUSR1 recebido
            if(*sinalptr == SIGUSR1){
                Tarefa1(&comandoParaExecutar);
            
            //SIGUSR2 recebido
            }else if(*sinalptr == SIGUSR2){
                Tarefa2(&comandoParaExecutar);
            
             //SIGTERM recebido           
            }else if(*sinalptr == SIGTERM){
                printf("SIGTERM foi recebido, logo o processo irá terminar\n");
                opcaoWhile=3;
            }
        }
    }

    return 0;
}
