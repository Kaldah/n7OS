#include <unistd.h>
#include <n7OS/processus.h>

/* code de la fonction
   int sleep(int seconds);
   pas d'argument -> appel à la fonction d'enveloppe syscall0
*/
syscall1(int, sleep, int, seconds)