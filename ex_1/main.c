/*
// Projeto SO - exercicio 2
// Sistemas Operativos, DEI/IST/ULisboa 2017-18

Grupo 61

Aluno : Luís Pedro Olivera Ferreira, num:83500
Aluno : Maxwell SMart Ntido Junior , num:79457
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "matrix2d.h"
#include "mplib3.h"



typedef struct{

    int n_iter;
    int t_id;
    int colunas;
    int n_tarefas;
    double dir;
    double esq;
    double baixo;
    double cima;
    }info_t;
    
    
    
/*--------------------------------------------------------------------
| Function: simul
---------------------------------------------------------------------*/

void *simul(void *information) {

    //inicializacao das variaveis e criacao das matrizes
    info_t *slave_info=(info_t *)information;
    DoubleMatrix2D *matrix, *matrix_aux, *tmp;
    int k,i,j;
    double value;
    int id=slave_info->t_id;
    int iter=slave_info->n_iter;
    int col=slave_info->colunas;
    int tarefas=slave_info->n_tarefas;
    int linhas=col/tarefas;
    double up=slave_info->cima;
    double down=slave_info->baixo;
    double left=slave_info->esq;
    double right=slave_info->dir;
    double recebe_linha[col+2];
    matrix=dm2dNew(linhas+2,col+2);
    matrix_aux=dm2dNew(linhas+2,col+2);
    
    //poe as temperaturas iniciais nas bordas das matrizes
    dm2dSetColumnTo (matrix, 0, left);
    dm2dSetColumnTo (matrix, col+1, right);
    dm2dSetColumnTo (matrix_aux, 0, left);
    dm2dSetColumnTo (matrix_aux, col+1, right);
    
    if (id==0){
        dm2dSetLineTo (matrix, 0, up);
        dm2dSetLineTo (matrix_aux, 0, up);
    }
    if (id==tarefas-1){
        dm2dSetLineTo (matrix, linhas+1, down);
        dm2dSetLineTo (matrix_aux, linhas+1, down);
    }
	
   	//atualiza matrix
   	for (k=0;k<iter;k++){ 
        for (i = 1; i < linhas+1; i++){
            for (j = 1; j < col+1; j++) {
                value = (dm2dGetEntry(matrix, i-1, j) + dm2dGetEntry(matrix, i+1, j) + dm2dGetEntry(matrix, i, j-1) + dm2dGetEntry(matrix, i, j+1) ) / 4.0;
                dm2dSetEntry(matrix_aux, i, j, value);
            }
        }
		
		// se id for par, recebe e depois envia uma linha
        if (id%2==0){
            if (id!=0){ // se nao e a primeira thread, troca mensagens com a thread anterior (e atualiza a respetiva linha)
                receberMensagem(id-1,id,recebe_linha, sizeof(double)*(col+2));
                dm2dSetLine(matrix_aux,0,recebe_linha);
                enviarMensagem(id,id-1,dm2dGetLine(matrix_aux,1), sizeof(double)*(col+2));
                
            }
            if (id!=tarefas-1){ // se nao e a ultima thread, troca com a thread seguinte
                receberMensagem(id+1,id,recebe_linha, sizeof(double)*(col+2)); 
                dm2dSetLine(matrix_aux,linhas+1,recebe_linha);
                enviarMensagem(id,id+1,dm2dGetLine(matrix_aux,linhas), sizeof(double)*(col+2));
            }
        }
            
        //se id for impar, envia e depois recebe uma linha
        if (id%2==1){         
            //se impar, nunca e a primeira thread
            enviarMensagem(id,id-1,dm2dGetLine(matrix_aux,1), sizeof(double)*(col+2));
            receberMensagem(id-1,id,recebe_linha, sizeof(double)*(col+2));
            dm2dSetLine(matrix_aux,0,recebe_linha);
            
            if (id!=tarefas-1){
                enviarMensagem(id,id+1,dm2dGetLine(matrix_aux,linhas), sizeof(double)*(col+2));
                receberMensagem(id+1,id,recebe_linha,sizeof(double)*(col+2)); 
                dm2dSetLine(matrix_aux,linhas+1,recebe_linha);
            }
        }
		
        tmp = matrix_aux;
        matrix_aux = matrix;
        matrix = tmp;
    }
    //Saida, envia para a main
    for (i=1;i<linhas+1;i++){
        enviarMensagem(id,tarefas,dm2dGetLine(matrix,i),sizeof(double)*(col+2));
    }
    dm2dFree(matrix);
    dm2dFree(matrix_aux);
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

    if(argc != 9) {
        fprintf(stderr, "\nNumero invalido de argumentos.\n");
        fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iteracoes trab csz\n\n");
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
    int csz = parse_integer_or_exit(argv[8], "csz");

    fprintf(stderr, "\nArgumentos:\n"
	" N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d trab=%d csz=%d\n",
	N, tEsq, tSup, tDir, tInf, iteracoes, trab, csz);

    if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iteracoes < 1 || N%trab!=0 || csz<0 || trab<0) {
        fprintf(stderr, "\nErro: Argumentos invalidos.\n"
        " Lembrar que N >= 1, temperaturas >= 0, iteracoes >= 1, N is divisble by trab, trab>0 e csz>=0\n\n");
    return 1;
    }
    
    DoubleMatrix2D *matrix_final; //matriz resultado, vai receber resultados das threads
    matrix_final=dm2dNew(N+2,N+2);
    dm2dSetLineTo(matrix_final,0,tSup);
    dm2dSetLineTo(matrix_final,N+1,tInf);

    
    info_t info[trab]; //array de estruturas com a informacao a passar a cada thread
    pthread_t *threads;
    threads=(pthread_t*)malloc(trab*sizeof(pthread_t)); //array com id de cada thread
    
    if (inicializarMPlib(csz, trab+1)==-1){
        printf("Erro ao inicializarMPlib.\n"); return 1;
    }
     
    int i;
    for (i=0;i<trab;i++){
        //atualiza info para cada thread
        info[i].t_id=i;
        info[i].n_iter=iteracoes;
        info[i].colunas=N;
        info[i].n_tarefas=trab;
        info[i].cima=tSup;
        info[i].baixo=tInf;
        info[i].esq=tEsq;
        info[i].dir=tDir;
    
        pthread_create(&threads[i],NULL,simul, &info[i]);
    }
    
    //Saida
    double recebe_linha[N+2];
    int k;
    //para cada thread (i) copia cada linha(k) para a matriz principal, e acaba a thread
    for (i=0;i<trab;i++){
            for (k=1;k<(N/trab+1);k++){
                receberMensagem(i,trab,recebe_linha,sizeof(double)*(N+2));
                dm2dSetLine(matrix_final,N/trab*i+k,recebe_linha);
        }
        if (pthread_join(threads[i],NULL)){
            fprintf(stderr, "\nErro ao esperar por um escravo\n");
            return -1;
        }
    }  
        
  dm2dPrint(matrix_final);
  
  dm2dFree(matrix_final);
  free(threads);
  libertarMPlib();
  
  
  return 0;
}
