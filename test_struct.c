#include <stdio.h>

/* ============================================================
   TEST: gestione struct e typedef struct
   ============================================================ */

/* --- typedef struct multi-riga (caso classico) --- */
typedef struct {
    int x;
    int y;
} Point;

/* --- typedef struct con tag e alias (riga singola, nessun corpo) --- */
typedef struct ColorTag Color;

/* --- struct pura (non typedef): definizione con corpo multi-riga --- */
struct ColorTag {
    float r;
    float g;
    float b;
};

/* --- typedef struct inline (corpo tutto su una riga) --- */
typedef struct { double re; double im; } Complex;

/* --- typedef struct con nome INVALIDO --- */
typedef struct { int val; } 9BadName;

/* --- variabili globali con tipi struct --- */
Point    p_global;           /* tipo valido (typedef), nome valido */
Color    c_global;           /* tipo valido (typedef via tag), nome valido */
Complex  z_global;           /* tipo valido (typedef inline), nome valido */
struct ColorTag raw_global;  /* tipo valido (struct pura), nome valido */
struct ColorTag 7invalid;    /* tipo valido, nome NON valido               */
UnknownType bad_global;      /* tipo NON valido, nome valido               */

int main(void)
{
    /* --- dichiarazioni locali --- */
    Point    p_local;           /* valida, usata       */
    Color    c_local;           /* valida, NON usata   */
    Complex  z_local;           /* valida, usata       */
    struct ColorTag raw_local;  /* valida, usata       */
    int      unused_int;        /* valida, NON usata   */

    /* --- body --- */
    p_local.x = 10;
    p_local.y = 20;

    z_local.re = 1.5;
    z_local.im = 2.5;

    raw_local.r = 0.5f;

    p_global.x = p_local.x + (int)z_local.re;
    c_global.r = raw_local.r;
    z_global.im = z_local.im;
    raw_global.g = 0.8f;

    printf("p_local=(%d,%d) z_local=(%.1f,%.1f)\n",
           p_local.x, p_local.y, z_local.re, z_local.im);

    return 0;
}
