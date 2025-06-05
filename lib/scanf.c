#include <stdio.h>
#include <string.h>
#include <n7OS/keyboard.h>
#include <ctype.h>

#define STRSIZE 255

char getchar(void) {
    char tmpchar;

    while ((tmpchar = kgetch()) == -1);
    return tmpchar;
}

char *gets(char *s) {
    char str[STRSIZE];
    char c;
    int count= 0;
   //shell_mess_col = _kgetcolumn (); // a modifier
   // shell_mess_line = _kgetline ();
	memset(str, '\0', STRSIZE);
    do {
	    c= getchar();	
	    if (c == '\b' && count>0)
            count--;
	    else
	        str[count++]= c;
    } while ((count<255) || (c != '\n' && c != '\r'));

    str[count--] = '\0';
    strcpy(s, str);

    return s;
}

int scanf (const char *format, ...)
{
    va_list scan;
    char input[STRSIZE];
    int count= 0;
    char maxchars[5] = {0};
    int i=0, nmax=0;

    char *s_ptr;
    int *i_ptr;

    va_start (scan, format);

    for (; *format; format++) {

	if (*format == '%') {
	    gets(input);
        
            count += strlen(input);

            if (isdigit(*++format)) {
              while (isdigit(*format)) {
                maxchars[i++] = *format;
                format++;
              }
              maxchars[i] = '\0';
              nmax = atoi(maxchars);
            }

	    switch (*format) {
	    case 's':
		s_ptr = va_arg (scan, char *);
                if (nmax == 0 || strlen(input) <= nmax)
					s_ptr = strncpy (s_ptr, input, strlen (input));
                else
					s_ptr = strncpy (s_ptr, input, nmax);
		break;

	    case 'd':
		i_ptr = va_arg (scan, int *);

                if (nmax != 0 && strlen(input) > nmax)
                  input[nmax] = '\0';
		*i_ptr = atoi(input);
                break;
	    }
	}
    }
    va_end (scan);
    return count;
}
