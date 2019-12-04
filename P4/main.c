/*
// Projeto SO - exercicio 2
// Sistemas Operativos, DEI/IST/ULisboa 2017-18

Grupo 61

Aluno : Luís Pedro Olivera Ferreira, num:83500
Aluno : Maxwell SMart Ntido Junior , num:79457
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

#include "matrix2d.h"

pthread_mutex_t   espera_mutex;
pthread_mutex_t   saiu_mutex;
pthread_mutex_t   max_mutex;
pthread_cond_t    espera_por_todos;
pthread_cond_t    wait_to_iterate;

pid_t             wpid,not_wpid;

int               status;
int               n_saves;

double max=0; //guarda o valor de diferenca maximo dessa iteracao(todas as tarefas) 
int tem_esperar=0; //numero de threads que estao na barreira
int saiu=0; //numero de threads que sairam da barreira
int finish=0; //diz se o sinal SIGINT foi enviado
int end=0;  //termina iteracoes se o sinal foi enviado
DoubleMatrix2D  *matrix,*matrix_aux,*tmp;
time_t          seconds,i_seconds; //segundos totais e os segundos iniciais, respetivamente

// Recebe o sinal. Indica que recebeu para as threads terminarem e diz que e preciso guardar a matrix.
void signalhandler(){
    finish=1;
    signal(SIGINT, signalhandler);
}
    


typedef struct{
    double maxD;
    int n_iter;
    int first_line;
    int last_line;
    int colunas;
    int n_tarefas;
    char *filename;
    int periodoS;
    }info_t;
    
/*--------------------------------------------------------------------
| Function: simul
---------------------------------------------------------------------*/

void *simul(void *information) {
    
    //inicializacao das variaveis
    info_t *slave_info=(info_t *)information;
    
    const int first_line=slave_info->first_line;
    const int last_line=slave_info->last_line;
    const int iter=slave_info->n_iter;
    const int col=slave_info->colunas;
    const int tarefas=slave_info->n_tarefas;
    const double maxD=slave_info->maxD;
    double diference;
    int k,i,j;
    double value;
    
   	//atualiza matrix
   	for (k=0;k<iter;k++){
                   
        //nao comeca iteracao seguinte enquanto todas as threads nao sairem da barreira
        if(pthread_mutex_lock(&saiu_mutex) != 0) {
            fprintf(stderr, "\nErro ao bloquear mutex\n");
            exit(-1);
        }       
        
        while(saiu!=0){
            if(pthread_cond_wait(&wait_to_iterate,&saiu_mutex) != 0) {
                fprintf(stderr, "\nErro ao esperar pela variável de condição\n");
                exit(-1);
            }
        }
        if(pthread_mutex_unlock(&saiu_mutex) != 0) {
            fprintf(stderr, "\nErro ao desbloquear mutex\n");
            exit(-1);
        }
        
        for (i = first_line; i <= last_line; i++){
            for (j = 1; j < col+1; j++) {
                value = (dm2dGetEntry(matrix, i-1, j) + dm2dGetEntry(matrix, i+1, j) + dm2dGetEntry(matrix, i, j-1) + dm2dGetEntry(matrix, i, j+1) ) / 4.0;
                dm2dSetEntry(matrix_aux, i, j, value);
                diference=value-dm2dGetEntry(matrix,i,j);
                               
                if(pthread_mutex_lock(&max_mutex) != 0) {
                    fprintf(stderr, "\nErro ao bloquear mutex\n");
                    exit(-1);
                }
                if (diference>max){
                    max=diference;
                }
                if(pthread_mutex_unlock(&max_mutex) != 0) {
                    fprintf(stderr, "\nErro ao desbloquear mutex\n");
                    exit(-1);
                }
            }
        }
        
        if(pthread_mutex_lock(&espera_mutex) != 0) {
            fprintf(stderr, "\nErro ao bloquear mutex\n");
            exit(-1);
        }      
       
        tem_esperar++;
        
        //troca matrizes apenas uma vez
        
        if (tem_esperar==tarefas){            
            tmp = matrix_aux;
            matrix_aux = matrix;
            matrix = tmp;
        }
        
        
        //barreira
        while (tem_esperar!=tarefas){
            if(pthread_cond_wait(&espera_por_todos,&espera_mutex) != 0) {
                fprintf(stderr, "\nErro ao esperar pela variável de condição\n");
                exit(-1);
            }
        }
        
        if(pthread_cond_broadcast(&espera_por_todos) != 0) {
            fprintf(stderr, "\nErro ao desbloquear variável de condição\n");
            exit(-1);
        }

        if(pthread_mutex_unlock(&espera_mutex) != 0) {           
            fprintf(stderr, "\nErro ao desbloquear mutex\n");
            exit(-1);
        }
        
        //Guarda a matrix quando necessario. Sendo sempre a primeira thread, nao e necessario lock.
        if (first_line==1){
            seconds=time(NULL);
            const int periodoS=slave_info->periodoS;
            if (periodoS!=0){
                if ((seconds-i_seconds-n_saves*periodoS)>periodoS){ //seconds-iseconds e o tempo que passou desde o inicio do programa. Tirando o numero de vezes que o programa guardou a matriz(ou nao, se ainda estivesse um fillho a correr) vezes o periodo, temos o tempo desde a ultima salvaguarda. 
                    n_saves++;
                    not_wpid=waitpid(-1, &status, WNOHANG); //Se ainda estiver um filho a correr, nao guarda a matrix.
                    if (not_wpid!=0){
                        int pid;
                        const char *filename=slave_info->filename;
                        char filename_aux[strlen(filename)+2];
                        strcpy(filename_aux,filename);
                        strcat(filename_aux,"~");
                        pid=fork();
                        if (pid==0){
                            FILE *filepointer=fopen(filename_aux,"w");
                            dm2dPrint(filepointer,matrix);
                            fclose(filepointer);
                            rename(filename_aux,filename);
                            exit(1);
                        }
                        else if (pid<0){
                            fprintf(stderr, "\nErro ao criar filho\n");
                        }
                    }
                }                
            }
        }     
                
        if (max<maxD||end==1){            
            return 0;
        }
        
        if(pthread_mutex_lock(&saiu_mutex) != 0) {
            fprintf(stderr, "\nErro ao bloquear mutex\n");
            exit(-1);
        }     
        
        saiu++;
        //reinicializa variaveis. se durante as iteracoes ocorreu o sinal, indica as threads que tem de terminar.
        if (saiu==tarefas){
            tem_esperar=0;
            max=0;      
            saiu=0;
            if (finish==1){
                n_saves=-1;
                end=1;
            }
            if(pthread_cond_broadcast(&wait_to_iterate) != 0) {
                fprintf(stderr, "\nErro ao desbloquear variável de condição\n");
                exit(-1);
            }
        }
        
        if(pthread_mutex_unlock(&saiu_mutex) != 0) {           
            fprintf(stderr, "\nErro ao desbloquear mutex\n");
            exit(-1);
        }
        
        
    }
    return 0;
}
  

/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
---------------------------------------------------------------------*/

int parse_integer_or_exit(char const *str, char const *name)
{
  int value;
 
  if(sscanf(str, "%d", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
--------------------------------- i------------------------------------*/

double parse_double_or_exit(char const *str, char const *name)
{
  double value;

  if(sscanf(str, "%lf", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}


/*--------------------------------------------------------------------
| Function: main
---------------------------------------------------------------------*/

int main (int argc, char** argv) {

    if(argc != 11) {
        fprintf(stderr, "\nNumero invalido de argumentos.\n");
        fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iteracoes trab maxD fichS periodoS\n\n");
        return 1;
    }

    /* argv[0] = program name */
    int N = parse_integer_or_exit(argv[1], "N");
    double tEsq = parse_double_or_exit(argv[2], "tEsq");
    double tSup = parse_double_or_exit(argv[3], "tSup");
    double tDir = parse_double_or_exit(argv[4], "tDir");
    double tInf = parse_double_or_exit(argv[5], "tInf");
    int iteracoes = parse_integer_or_exit(argv[6], "iteracoes");
    int trab = parse_integer_or_exit(argv[7], "trab");
    double maxD = parse_double_or_exit(argv[8], "maxD");
    char* fichS = argv[9];
    int periodoS = parse_integer_or_exit(argv[10], "periodoS");

    if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iteracoes < 1 || trab<1 ||N%trab!=0 || maxD<0 || periodoS<0) {
        fprintf(stderr, "\nErro: Argumentos invalidos.\n"
        " Lembrar que N >= 1, temperaturas >= 0, iteracoes >= 1, N is divisble by trab, trab>=1, maxD>=0 e periodoS>=0\n\n");
        return 1;
    }
    i_seconds=time(NULL);
    signal(SIGINT, signalhandler);
    
    FILE *ficheiro;
    ficheiro=fopen(fichS,"r");

    if (ficheiro!=NULL){
        matrix=readMatrix2dFromFile(ficheiro, N+2, N+2);
        fclose(ficheiro);
    }    
    
    if(matrix==NULL){
        matrix=dm2dInitiate(matrix, N, tSup,tInf,tEsq,tDir);        
    }
    matrix_aux=dm2dInitiate(matrix_aux, N,tSup,tInf,tEsq,tDir);        
        
    if(pthread_mutex_init(&espera_mutex, NULL) != 0) {
        fprintf(stderr, "\nErro ao inicializar mutex\n");
        return -1;
    }
    if(pthread_mutex_init(&saiu_mutex, NULL) != 0) {
        fprintf(stderr, "\nErro ao inicializar mutex\n");
        return -1;
    }
    if(pthread_mutex_init(&max_mutex, NULL) != 0) {
        fprintf(stderr, "\nErro ao inicializar mutex\n");
        return -1;
    }
    if(pthread_cond_init(&espera_por_todos, NULL) != 0) {
        fprintf(stderr, "\nErro ao inicializar variável de condição\n");
        return -1;
    }
    if(pthread_cond_init(&wait_to_iterate, NULL) != 0) {
        fprintf(stderr, "\nErro ao inicializar variável de condição\n");
        return -1;
    }
    
    info_t *info;
    info=(info_t*)malloc(trab*sizeof(info_t)); //array de estruturas com a informacao a passar a cada thread
    pthread_t *threads;
    threads=(pthread_t*)malloc(trab*sizeof(pthread_t)); //array com id de cada thread
    if (info==NULL||threads==NULL){
        fprintf(stderr, "\nErro ao alocar memoria\n");
    }
    
    int i;
    for (i=0;i<trab;i++){
        //atualiza info para cada thread
        info[i].maxD=maxD;
        info[i].n_iter=iteracoes;
        info[i].first_line=i*N/trab+1;
        info[i].last_line=(i+1)*N/trab;        
        info[i].colunas=N;
        info[i].n_tarefas=trab;
        info[i].filename=fichS;
        info[i].periodoS=periodoS;
        if (pthread_create(&threads[i],NULL,simul, &info[i])!=0){
            fprintf(stderr, "\nErro ao criar um escravo\n");
        }
    }

    //Saida
    for (i=0;i<trab;i++){
        if (pthread_join(threads[i],NULL)){
            fprintf(stderr, "\nErro ao esperar por um escravo\n");
            return -1;
        }   
    }

    dm2dPrint(stdout,matrix);

    wpid=wait(&status);
    if (end==0){
        unlink(fichS);
    }
   
   if(pthread_mutex_destroy(&espera_mutex) != 0) {
        fprintf(stderr, "\nErro ao destruir mutex\n");
        exit(EXIT_FAILURE);
    }
    if(pthread_mutex_destroy(&saiu_mutex) != 0) {
        fprintf(stderr, "\nErro ao destruir mutex\n");
        exit(EXIT_FAILURE);
    }
    if(pthread_mutex_destroy(&max_mutex) != 0) {
        fprintf(stderr, "\nErro ao destruir mutex\n");
        exit(EXIT_FAILURE);
    }
    if(pthread_cond_destroy(&espera_por_todos) != 0) {
        fprintf(stderr, "\nErro ao destruir variável de condição\n");
        exit(EXIT_FAILURE);
    }
    if(pthread_cond_destroy(&wait_to_iterate) != 0) {
        fprintf(stderr, "\nErro ao destruir variável de condição\n");
        exit(EXIT_FAILURE);
    }
  
   dm2dFree(matrix);
   dm2dFree(matrix_aux);
   free(threads);
   free(info);  
  
  return 0;
}
