#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define populacao 10
#define tam_genetico 8
#define geracoes 45

char vert_liga(int avaliado[], int var[]) {
    int vertical = avaliado[0] * (var[0]- avaliado[1]) + avaliado[2] * var[1] + avaliado[3] * var[2];
    if (vertical >= 0) return 'v';
    return 'n';
}

char hori_liga(int avaliado[], int var[]){ 
    int vertical = avaliado[4] * (var[0]- avaliado[5]) + avaliado[6] * var[1] + avaliado[7] * var[2];
    if (vertical >= 0) return 'h';
    return 'n';
}

void genocidio(int pais[][tam_genetico]) {
    for (int i = 0; i < populacao; i++) {
        for (int j = 0; j < tam_genetico; j++) {
            pais[i][j] = (rand() % 200) - 100;
        }
    }
}

void predacao_por_sintese(int pais[][tam_genetico], int indice_pior) {
    int consenso[tam_genetico];
    for (int j = 0; j < tam_genetico; j++) {
        float soma = 0.0f;
        for (int i = 0; i < populacao; i++) soma += pais[i][j];
        consenso[j] = (int)(soma / populacao);
    }
    memcpy(pais[indice_pior], consenso, tam_genetico * sizeof(int));
}

void mutacao(int filho[]) {
    int n = tam_genetico / 4;
    if (n < 1) n = 1;
    for (int k = 0; k < n; k++) {
        int mutou = rand() % tam_genetico;
        int delta = (rand() % 21) - 10;
        filho[mutou] += delta;
    }
}

void evolucao(int pai[][tam_genetico], int melhor[]) {
    for (int i = 0; i < populacao; i++) {
        for (int j = 0; j < tam_genetico; j++) pai[i][j] = melhor[j];
        mutacao(pai[i]);
    }
}

int avaliacao(int avaliado[]) {
    int pont = 0;
    for (int i = 0; i < tam_genetico; i++) pont += avaliado[i];
    return pont;
}

void pior_de_todos(int pais[][tam_genetico]) {
    int indice = 0;
    int pior_aval = avaliacao(pais[0]);
    for (int i = 1; i < populacao; i++) {
        int av = avaliacao(pais[i]);
        if (pior_aval > av) {
            pior_aval = av;
            indice = i;
        }
    }
    predacao_por_sintese(pais, indice);
}

void darwin(int pais[][tam_genetico], int melhor[]) {
    int melhor_ava = avaliacao(pais[0]);
    memcpy(melhor, pais[0], sizeof(int) * tam_genetico);

    for (int i = 1; i < populacao; i++) {
        int av = avaliacao(pais[i]);
        if (av > melhor_ava) {
            melhor_ava = av;
            memcpy(melhor, pais[i], sizeof(int) * tam_genetico);
        }
    }

    for (int a = 0; a < geracoes; a++) {
        for (int i = 0; i < populacao; i++) {
            int candidato[tam_genetico];
            memcpy(candidato, pais[i], sizeof(candidato));
            int resultado = avaliacao(candidato);
            if (resultado > melhor_ava) {
                melhor_ava = resultado;
                memcpy(melhor, candidato, sizeof(candidato));
            }
        }

        if (a % 20 == 9) genocidio(pais);
        else {
            pior_de_todos(pais);
            evolucao(pais, melhor);
        }

        if (a % 10 == 0 || a == geracoes - 1) {
            printf("\n--- Geração %d (melhor=%d) ---\n", a, melhor_ava);
            for (int i = 0; i < tam_genetico; i++) printf("%d ", melhor[i]);
            printf("\n");
        }
    }
}

int main(void) {
    unsigned seed = (unsigned) time(NULL) ^ (unsigned) clock();
    srand(seed);

    int pais[populacao][tam_genetico];
    for (int i = 0; i < populacao; i++)
        for (int j = 0; j < tam_genetico; j++)
            pais[i][j] = (rand() % 200) - 100;

    int melhor_de_todos[tam_genetico];
    darwin(pais, melhor_de_todos);

    printf("\n=== Melhor final ===\n");
    for (int i = 0; i < tam_genetico; i++) printf("%d ", melhor_de_todos[i]);
    printf("\n");

    return 0;
}
