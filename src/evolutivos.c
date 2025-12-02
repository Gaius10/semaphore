#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "evolutivos.h"
#define populacao 10
#define tam_genetico 6
#define geracoes 45
char vert_liga(int avaliado[], int carros[]){
    int vertical = avaliado[0]*carros[0]+avaliado[1]*carros[1] + avaliado[2];
    if(vertical >=0){
        return 'v';
    }
}
char hori_liga(int avaliado[], int carros[]){
    int horizontal = avaliado[3]*carros[0]+avaliado[4]*carros[1] + avaliado[5];
    if(horizontal >=0){
        return 'h';
    }
}
void genocidio(int pais[][tam_genetico]){
    for(int i=0;i<populacao;i++){
        for(int j=0;j<tam_genetico;j++){
            pais[i][j]=(rand()%200)-100;
        }
    }
}
void predacao_por_sintese(int pais[][tam_genetico], int indice_pior) {

    int consenso[tam_genetico];

    for (int j = 0; j < tam_genetico; j++) {
        float soma = 0.0f;
        for (int i = 0; i < populacao; i++) {
            soma += pais[i][j];
        }
        consenso[j] = soma / populacao;   // média sem restrição
    }

    memcpy(pais[indice_pior], consenso, tam_genetico * sizeof(int));
}
void mutacao(int filho[]){
    for(int i=0;i<tam_genetico/20;i++){
    int mutou = rand()%tam_genetico;
    filho[mutou] += ((rand()%20)-10);
    }
}
void evolucao(int pai[][tam_genetico], int melhor[]){
    for(int i=0;i<populacao;i++){
        for(int j=0;j<tam_genetico;j++){
        pai[i][j]=melhor[j];   
        } 
        mutacao(pai[i]);
    
}
}
int avaliacao(int avaliado[]){
    int pont = 0;
    for(int i=0;i<tam_genetico;i++){
            pont += avaliado[i];
        }
        return pont;
}
void pior_de_todos(int pais[][tam_genetico]){
    int indice =0;
    int pior_aval=avaliacao(pais[0]);
    for(int i=1;i<populacao;i++){
        if(pior_aval>avaliacao(pais[i])){
            pior_aval = avaliacao(pais[i]);
            indice = i;
        }
    }
    predacao_por_sintese(pais,indice);
}
void darwin(int pais[][tam_genetico], int melhor[]){
    int melhor_ava = avaliacao(pais[1]);
    for(int a=0;a<geracoes;a++){
        for(int i=0;i<populacao;i++){
            int candidato[tam_genetico];
            memcpy(candidato, pais[i], sizeof(candidato));
            int resultado = avaliacao(candidato);
            if(resultado>melhor_ava){
                melhor_ava = resultado;
                memcpy(melhor,candidato,sizeof(candidato));
            }
        }
        if(a%20==9){
            genocidio(pais);
        }
        else{
        pior_de_todos(pais);
        evolucao(pais,melhor);
        }
        printf("\n----MELHOR GERACIONAL----\n");
        for(int i=0;i<tam_genetico;i++){
            printf("%d",melhor[i]);
    }
    }
}
void geral() {
    unsigned seed = (unsigned) time(NULL) 
                  ^ (unsigned) clock();
    srand(seed);//Utiliza-se isso para garantir uma maior dispersão dos valores aleatórios
    int pais[populacao][tam_genetico];
    for(int i =0; i<populacao;i++){
        int filho[tam_genetico];
        for(int j = 0; j <tam_genetico; j++){
            int gene = (rand()%200+0)-100;
            filho[j]=gene;
            
        }
        for (int j = 0; j < tam_genetico; j++) {
            pais[i][j] = filho[j];
        }
    
    }
    
    int melhor_de_todos[tam_genetico];
    darwin(pais,melhor_de_todos);
    //int melhor_de_todos[tam_genetico];
    for(int i=0;i<tam_genetico;i++){
        printf("\n%d ",melhor_de_todos[i]);
    }
    
    return 0;
}





