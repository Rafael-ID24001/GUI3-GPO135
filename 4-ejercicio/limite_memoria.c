/*
 * limite_memoria.c
 * ============================================================================
 * Objetivo:
 *   Demostrar que ocurre cuando un proceso intenta superar un limite de
 *   memoria virtual impuesto con "ulimit -v [kB]".
 *
 * Descripcion:
 *   El programa actua como un lanzador (padre). Realiza lo siguiente:
 *
 *     1. Crea un proceso hijo con fork()  -> NO se usa ningun proceso
 *        critico del sistema; el ejemplo es totalmente propio y aislado.
 *     2. El proceso hijo se impone a si mismo un limite de memoria virtual
 *        (RLIMIT_AS) mediante setrlimit(). Este limite es EXACTAMENTE el
 *        equivalente en codigo C del comando de shell "ulimit -v [kB]".
 *     3. El hijo, ya limitado, intenta reservar mas memoria de la permitida
 *        con malloc() en un bucle, hasta que la asignacion falla.
 *     4. El padre espera al hijo con waitpid() e informa como termino.
 *
 *   De este modo se reproduce de forma segura el efecto de "ulimit -v".
 *
 * Equivalencia con el shell:
 *   Limite en C          -->  setrlimit(RLIMIT_AS, &rlim)
 *   Limite en shell      -->  ulimit -v <kB>   (limite de espacio de
 *                             direcciones, en kilobytes)
 *
 * Compilacion: gcc -Wall -o limite_memoria limite_memoria.c
 * Ejecucion: ./limite_memoria [limite_en_kB]
 * Ejemplo:  ./limite_memoria 65536     (limite de 64 MB)
 *
 * Si no se pasa argumento se usa 65536 kB (64 MB) por defecto.
 * ============================================================================
 */

#include <stdio.h>      /* fprintf, printf, stderr                 */
#include <stdlib.h>     /* malloc, free, exit, strtoul             */
#include <string.h>     /* memset                                  */
#include <unistd.h>     /* fork, getpid, sleep                     */
#include <sys/wait.h>   /* waitpid, macros WIFEXITED/WIFSIGNALED   */
#include <sys/resource.h> /* setrlimit, getrlimit, RLIMIT_AS        */
#include <errno.h>      /* errno                                   */
#include <signal.h>     /* para interpretar senales de terminacion */

/* Tamanio de cada bloque reservado (1 MiB) */
#define MB (1024UL * 1024UL)

/* Limite por defecto: 64 MiB, expresados en kilobytes (como ulimit -v) */
#define LIMITE_DEFECTO_KB 65536UL

/*
 * Impone el limite de memoria virtual (RLIMIT_AS) al proceso actual.
 *
 * RLIMIT_AS limita el "address space" (espacio de direcciones) del proceso,
 * medido en BYTES. El comando "ulimit -v <kB>" usa kilobytes, por eso aqui
 * convertimos kB -> bytes multiplicando por 1024.
 *
 * Se fijan ambos valores del struct rlimit:
 *   - rlim_cur (soft limit): limite efectivo actual.
 *   - rlim_max (hard limit): limite maximo permitido para rlim_cur.
 *
 * Devuelve 0 si tuvo exito, -1 si hubo error.
 */
static int aplicaLimiteMemoria(unsigned long limite_kb)
{
    struct rlimit rl;

    rl.rlim_cur = (rlim_t)limite_kb * 1024UL; /* limite suave  (en bytes) */
    rl.rlim_max = (rlim_t)limite_kb * 1024UL; /* limite duro   (en bytes) */

    if (setrlimit(RLIMIT_AS, &rl) != 0) {
        perror("setrlimit(RLIMIT_AS)");
        return -1;
    }
    return 0;
}

/*
 * Codigo que ejecuta el proceso HIJO.
 * Primero se impone el limite y despues intenta reservar memoria hasta
 * sobrepasarlo. Cada bloque se "toca" con memset para que el sistema
 * realmente comprometa la memoria (no solo reserve el rango de direcciones).
 */
static void hijoIntentaExceder(unsigned long limite_kb)
{
    unsigned long reservado = 0;
    void *bloque;
    const char *patron = "Patron de prueba";

    printf("[hijo] PID=%d. Imponiendo limite de memoria virtual = %lu kB (%lu MB)\n",
           (int)getpid(), limite_kb, limite_kb / 1024UL);

    if (aplicaLimiteMemoria(limite_kb) != 0)
        exit(EXIT_FAILURE);

    /* Bucle que intenta reservar 1 MiB por iteracion, superando el limite */
    while (1) {
        bloque = malloc(MB);
        if (bloque == NULL) {
            /* malloc devolvio NULL: se alcanzo la barrera del limite */
            fprintf(stderr,
                    "[hijo] malloc fallo tras reservar %lu MB (errno=%d: %s)\n",
                    reservado / MB, errno, strerror(errno));
            break;
        }

        /* Se escribe en el bloque para forzar el uso real de la memoria */
        memset(bloque, 0, MB);
        (void)patron; /* referencia para evitar warning*/

        reservado += MB;
        printf("[hijo] Reservados %lu MB\n", reservado / MB);
    }

    printf("[hijo] Fin del hijo: no se pudo superar el limite impuesto.\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    unsigned long limite_kb = LIMITE_DEFECTO_KB;
    pid_t pid;
    int estado;

    /* Permitir pasar el limite como argumento (en kB) */
    if (argc > 1) {
        char *fin;
        unsigned long v = strtoul(argv[1], &fin, 10);
        if (*fin != '\0' || v == 0) {
            fprintf(stderr, "Uso: %s [limite_en_kB] (ej: %s 65536)\n",
                    argv[0], argv[0]);
            return EXIT_FAILURE;
        }
        limite_kb = v;
    }

    printf("[padre] Iniciando ejemplo de RLIMIT_AS / ulimit -v\n");
    printf("[padre] Limite a aplicar al hijo: %lu kB (%lu MB)\n",
           limite_kb, limite_kb / 1024UL);

    /* Se crea un proceso propio para el experimento (no se usa un proceso
     * critico del sistema) */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* ===== Proceso HIJO ===== */
        hijoIntentaExceder(limite_kb);
        /* hijoIntentaExceder nunca retorna (termina con exit) */
    }

    /* ===== Proceso PADRE ===== */
    if (waitpid(pid, &estado, 0) < 0) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    printf("\n[padre] El hijo finalizo.\n");
    if (WIFEXITED(estado)) {
        printf("[padre] Termino normalmente con codigo de salida %d.\n",
               WEXITSTATUS(estado));
        printf("[padre] Conclusion: malloc() fallo al alcanzar el limite; "
               "el proceso pudo continuar y salir con gracia.\n");
    } else if (WIFSIGNALED(estado)) {
        printf("[padre] Termino por la senal %d (%s).\n",
               WTERMSIG(estado), strsignal(WTERMSIG(estado)));
        printf("[padre] Conclusion: el sistema mato al proceso al exceder "
               "la memoria permitida.\n");
    }

    return EXIT_SUCCESS;
}
