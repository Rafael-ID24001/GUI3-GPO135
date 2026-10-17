/*
 * ============================================================================
 *  SIMULADOR DE ALGORITMOS DE REEMPLAZO DE PAGINAS
 * ============================================================================
 *  Materia : GPO135
 *  Objetivo: Recibir una secuencia de referencias a memoria (numeros de
 *            pagina) y simular el comportamiento de una memoria fisica de
 *            N marcos (frames) aplicando tres algoritmos de reemplazo:
 *              1) FIFO  -> First In, First Out
 *              2) LRU   -> Least Recently Used
 *              3) OPT   -> Optimo (Belady)
 *  Compilacion: gcc -Wall -Wextra -o simulador simulador.c
 *  Ejecucion (modo interactivo): ./simulador
 *  Ejecucion con argumentos: ./simulador <num_marcos> 7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_MARCOS   64      /* Maximo numero de marcos que soporta el simulador */
#define MAX_REFS    512      /* Maximo numero de referencias en la secuencia   */

// Separador
static void separador(void)
{
    printf("-------------------------------------------------------------------------\n");
}

// * =========================================================================
// *  ALGORITMO FIFO (First In, First Out)
// * =========================================================================
static int simular_fifo(const int *refs, int n_refs, int n_marcos)
{
    int marcos[MAX_MARCOS];   /* Contenido actual de cada marco (-1 = libre) */
    int siguiente = 0;        /* indice del marco a expulsar (la cola)       */
    int fallos = 0;
    int i, j;

    /* Inicializamos todos los marcos como libres */
    for (i = 0; i < n_marcos; i++)
        marcos[i] = -1;

    printf("\n=== FIFO (num_marcos = %d) ===\n", n_marcos);
    separador();

    /* Recorremos la secuencia de referencias, una por una */
    for (i = 0; i < n_refs; i++) {
        int pagina = refs[i];
        int hit = 0;

        /* 1: Buscar la pagina en memoria */
        for (j = 0; j < n_marcos; j++) {
            if (marcos[j] == pagina) {
                hit = 1;
                break;
            }
        }

        if (hit) {
            printf("Ref %2d: pagina %2d -> HIT   ", i + 1, pagina);
        } else {
            /* 2: FALLO de pagina */
            fallos++;

            /* 3: Hay algun marco libre? */
            int libre = -1;
            for (j = 0; j < n_marcos; j++) {
                if (marcos[j] == -1) {
                    libre = j;
                    break;
                }
            }

            if (libre != -1) {
                /* Colocamos la pagina en el marco libre */
                marcos[libre] = pagina;
                printf("Ref %2d: pagina %2d -> FALLO (carga en marco libre) ",
                       i + 1, pagina);
            } else {
                /* 4: No hay libres: expulsamos la mas antigua */
                printf("Ref %2d: pagina %2d -> FALLO (reemplaza %2d)      ",
                       i + 1, pagina, marcos[siguiente]);
                marcos[siguiente] = pagina;
                siguiente = (siguiente + 1) % n_marcos; /* avance circular */
            }
        }

        /* Imprimimos el estado de los marcos tras procesar la referencia */
        printf("| marcos:");
        for (j = 0; j < n_marcos; j++) {
            if (marcos[j] == -1)
                printf("  -");
            else
                printf(" %2d", marcos[j]);
        }
        printf("\n");
    }

    separador();
    printf("Total de fallos de pagina (FIFO): %d\n", fallos);
    return fallos;
}

/* =========================================================================
 *  ALGORITMO LRU (Least Recently Used)
 * ========================================================================= */
static int simular_lru(const int *refs, int n_refs, int n_marcos)
{
    int marcos[MAX_MARCOS];
    int usado_en[MAX_MARCOS];
    int fallos = 0;
    int i, j;

    for (i = 0; i < n_marcos; i++) {
        marcos[i] = -1;
        usado_en[i] = -1;      /* Nadie ha usado estos marcos todavia */
    }

    printf("\n=== LRU (num_marcos = %d) ===\n", n_marcos);
    separador();

    for (i = 0; i < n_refs; i++) {
        int pagina = refs[i];
        int hit = -1;

        /* 1: Buscar la pagina en los marcos */
        for (j = 0; j < n_marcos; j++) {
            if (marcos[j] == pagina) {
                hit = j;
                break;
            }
        }

        if (hit != -1) {
            /* 2: HIT: actualizamos la marca de tiempo del marco */
            usado_en[hit] = i;
            printf("Ref %2d: pagina %2d -> HIT   ", i + 1, pagina);
        } else {
            /* 4/5: FALLO */
            fallos++;
            int libre = -1;
            for (j = 0; j < n_marcos; j++) {
                if (marcos[j] == -1) {
                    libre = j;
                    break;
                }
            }

            if (libre != -1) {
                marcos[libre] = pagina;
                usado_en[libre] = i;      /*4: recien cargada = uso mas reciente */
                printf("Ref %2d: pagina %2d -> FALLO (carga en marco libre) ",
                       i + 1, pagina);
            } else {
                /* 5: Buscamos el marco con la marca de tiempo mas antigua */
                int lru = 0;
                for (j = 1; j < n_marcos; j++) {
                    if (usado_en[j] < usado_en[lru])
                        lru = j;
                }
                printf("Ref %2d: pagina %2d -> FALLO (reemplaza %2d)      ",
                       i + 1, pagina, marcos[lru]);
                marcos[lru] = pagina;
                usado_en[lru] = i;        /* ahora es el mas reciente */
            }
        }

        printf("| marcos:");
        for (j = 0; j < n_marcos; j++) {
            if (marcos[j] == -1)
                printf("  -");
            else
                printf(" %2d", marcos[j]);
        }
        printf("\n");
    }

    separador();
    printf("Total de fallos de pagina (LRU): %d\n", fallos);
    return fallos;
}

/* =========================================================================
 *  ALGORITMO OPT (Optimo de Belady)
 * ========================================================================= */
static int simular_opt(const int *refs, int n_refs, int n_marcos)
{
    int marcos[MAX_MARCOS];
    int fallos = 0;
    int i, j, k;

    for (i = 0; i < n_marcos; i++)
        marcos[i] = -1;

    printf("\n=== OPT / Belady (num_marcos = %d) ===\n", n_marcos);
    separador();

    for (i = 0; i < n_refs; i++) {
        int pagina = refs[i];
        int hit = 0;

        /* 1: Buscar la pagina */
        for (j = 0; j < n_marcos; j++) {
            if (marcos[j] == pagina) {
                hit = 1;
                break;
            }
        }

        if (hit) {
            printf("Ref %2d: pagina %2d -> HIT   ", i + 1, pagina);
        } else {
            /* 3/4: FALLO */
            fallos++;
            int libre = -1;
            for (j = 0; j < n_marcos; j++) {
                if (marcos[j] == -1) {
                    libre = j;
                    break;
                }
            }

            if (libre != -1) {
                marcos[libre] = pagina;
                printf("Ref %2d: pagina %2d -> FALLO (carga en marco libre) ",
                       i + 1, pagina);
            } else {
                /* Calculamos para cada marco, en que referencia futura se
                 * usara su pagina. Elegimos el que tenga el uso mas lejano.
                 */
                int victima = -1;
                int mas_lejano = -1;

                for (j = 0; j < n_marcos; j++) {
                    int prox = INT_MAX;   /* "infinito": no vuelve a usarse */
                    for (k = i + 1; k < n_refs; k++) {
                        if (refs[k] == marcos[j]) {
                            prox = k;
                            break;
                        }
                    }
                    if (prox > mas_lejano) {
                        mas_lejano = prox;
                        victima = j;
                    }
                }

                printf("Ref %2d: pagina %2d -> FALLO (reemplaza %2d)      ",
                       i + 1, pagina, marcos[victima]);
                marcos[victima] = pagina;
            }
        }

        printf("| marcos:");
        for (j = 0; j < n_marcos; j++) {
            if (marcos[j] == -1)
                printf("  -");
            else
                printf(" %2d", marcos[j]);
        }
        printf("\n");
    }

    separador();
    printf("Total de fallos de pagina (OPT): %d\n", fallos);
    return fallos;
}

static int leer_secuencia(int *refs)
{
    int n = 0;
    int valor;

    printf("Ingrese la secuencia de referencias (numeros de pagina) separadas\n");
    printf("por espacios o Enter. Termine con -1 o con EOF (Ctrl+D):\n> ");

    while (n < MAX_REFS && scanf("%d", &valor) == 1) {
        if (valor == -1)          /* condicion para terminar*/
            break;
        refs[n++] = valor;
    }

    return n;
}

int main(int argc, char *argv[])
{
    int refs[MAX_REFS];
    int n_refs = 0;
    int n_marcos;
    int f_fifo, f_lru, f_opt;
    int i;

    printf("=========================================================\n");
    printf("   SIMULADOR DE REEMPLAZO DE PAGINAS (FIFO / LRU / OPT)  \n");
    printf("=========================================================\n");

    if (argc >= 3) {
        /* --- Modo con argumentos: argv[1] = marcos, resto = referencias --- */
        n_marcos = atoi(argv[1]);
        for (i = 2; i < argc && n_refs < MAX_REFS; i++)
            refs[n_refs++] = atoi(argv[i]);
    } else {
        /* --- Modo interactivo --- */
        printf("Numero de marcos de memoria fisica: ");
        if (scanf("%d", &n_marcos) != 1) {
            fprintf(stderr, "Error: debe ingresar un numero de marcos.\n");
            return 1;
        }
        n_refs = leer_secuencia(refs);
    }

    if (n_marcos <= 0 || n_marcos > MAX_MARCOS) {
        fprintf(stderr, "Error: num_marcos debe estar entre 1 y %d.\n", MAX_MARCOS);
        return 1;
    }
    if (n_refs <= 0) {
        fprintf(stderr, "Error: no se ingreso ninguna referencia valida.\n");
        return 1;
    }

    /* Mostramos la entrada recibida */
    printf("\nMarcos            : %d\n", n_marcos);
    printf("Secuencia (%d refs):", n_refs);
    for (i = 0; i < n_refs; i++)
        printf(" %d", refs[i]);
    printf("\n");

    /* Ejecutamos los tres algoritmos sobre la MISMA secuencia */
    f_fifo = simular_fifo(refs, n_refs, n_marcos);
    f_lru  = simular_lru (refs, n_refs, n_marcos);
    f_opt  = simular_opt (refs, n_refs, n_marcos);

    /* -------------------------------------------------------------------
     * RESUMEN COMPARATIVO
     * -------------------------------------------------------------------
     *  Menos fallos = mejor. Se marca el mejor algoritmo (excluyendo OPT,
     *  que es la cota teorica ideal).                                    */
    printf("\n===================== RESUMEN COMPARATIVO =====================\n");
    printf("  FIFO : %3d fallos de pagina\n", f_fifo);
    printf("  LRU  : %3d fallos de pagina\n", f_lru);
    printf("  OPT  : %3d fallos de pagina  (cota inferior teorica)\n", f_opt);

    /* Determinamos el mejor algoritmo real (FIFO o LRU) */
    if (f_lru < f_fifo)
        printf("  -> LRU obtuvo MENOS fallos que FIFO en esta secuencia.\n");
    else if (f_fifo < f_lru)
        printf("  -> FIFO obtuvo MENOS fallos que LRU en esta secuencia.\n");
    else
        printf("  -> FIFO y LRU empataron en esta secuencia.\n");

    printf("===============================================================\n");
    return 0;
}
