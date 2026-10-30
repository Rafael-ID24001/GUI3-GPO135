#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N 10
#define GRANDE 400000

int main() {
    char *bloques[N];
    int tamanos[N];

    // FASE 1: reservar 10 bloques de 1KB-20KB
    srand(42);
    for (int i = 0; i < N; i++) {
        tamanos[i] = (rand() % 20 + 1) * 1024;
        bloques[i] = malloc(tamanos[i]);
        printf("bloques[%2d] = %6d bytes @ %p\n", i, tamanos[i], bloques[i]);
    }

    // FASE 2: liberar los IMPARES (intercalado) → quedan huecos separados
    for (int i = 1; i < N; i += 2) {
        free(bloques[i]);
        bloques[i] = NULL;
    }

    // FASE 3: pedir UN bloque grande → no cabe en los huecos → heap crece
    char *grande = malloc(GRANDE);

    /* ... despues de malloc(GRANDE), antes de liberar: */
    printf("\n[DORMIRE 60 SEG] Mi PID es %d - analizame con pmap!\n", getpid());
    sleep(60);

    // Limpieza
    for (int i = 0; i < N; i++)
        if (bloques[i]) free(bloques[i]);
    free(grande);
    return 0;
}