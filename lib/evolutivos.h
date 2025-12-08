#ifndef EVOLUCAO_H
#define EVOLUCAO_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define populacao 10
#define tam_genetico 8
#define geracoes 45



void genocidio(int pais[][tam_genetico]);

void predacao_por_sintese(int pais[][tam_genetico], int indice_pior);
void mutacao(int filho[]);
void evolucao(int pai[][tam_genetico], int melhor[]);


int avaliacao(int avaliado[]);
void pior_de_todos(int pais[][tam_genetico]);


void darwin(int pais[][tam_genetico], int melhor[]);

void geral();
#endif
