#include <stdio.h>

/* ============================
   VARIABILI GLOBALI
   ============================ */

// valide
int g1;
unsigned int g2;

// non valide
int 9abc;              // nome non valido
unsigned long pippo;   // tipo valido, ma NON usata

// typedef non valido
typedef int 12tipo;    // nome typedef non valido

// typedef valido
typedef long myLong;

// uso typedef prima della definizione → errore
myType x_global;

/* ============================
   INIZIO MAIN
   ============================ */

int main(void)
{
    /* ============================
       DICHIARAZIONI LOCALI
       ============================ */

    // valide
    int a;
    int _b;
    myLong ml;          // typedef valido
    unsigned int u;
    long long z;

    // non valide
    int 3var;           // nome non valido
    tipo inesistente;   // tipo non valido
    myType y;           // typedef non definito

    // multiple
    int c, d, e;        // solo c e d usate

    /* ============================
       BODY
       ============================ */

    a = 10;
    u = 5;
    ml = a + u;
    z = ml * 2;

    c = z;
    d = c + 1;

    printf("a=%d u=%u ml=%ld z=%lld c=%d d=%d\n", a, u, ml, z, c, d);
   
    return 0;
}
