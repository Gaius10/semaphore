#ifndef EVOLUTIVOS_H
#define EVOLUTIVOS_H
#define populacao 10
#define tam_genetico 6
#define geracoes 45
char vert_liga(int avaliado[], int carros[]);
char hori_liga(int avaliado[], int carros[]);
void genocidio(int pais[][tam_genetico]);
void predacao_por_sintese(int pai[][tam_genetico],int indice);
void mutacao(int filho[]);
void evolucao(int pai[][tam_genetico], int melhor[]);
int avaliacao(int avaliado[]);
void pior_de_todos(int pais[][tam_genetico]);
void darwin(int pais[][tam_genetico], int melhor[]);
void geral();
#endif