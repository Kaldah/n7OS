#include <ctype.h>

int isspace(int c) {
	return ((c) == ' ') || ((c) == '\f')
		|| ((c) == '\n') || ((c) == '\r')
		|| ((c) == '\t') || ((c) == '\v');
}

int isdigit(int c) {
	return ((c) >= '0' && (c) <= '9');
}

int isalpha(int c) {
	return (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'));
}

int isalnum(int c) {
	return (isdigit(c) || isalpha(c));
}	

int isupper(int c) {
	return ((c) >= 'A' && (c) <= 'Z');
}

int islower(int c) {
	return ((c) >= 'a' && (c) <= 'z');
}	

int isxdigit(int c) {
	return (isdigit(c) || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'));
}

#define OFFSET ('a' - 'A')

int tolower(int c) {
    return (isupper(c) ? c + OFFSET : c);
}

int toupper(int c) {
    return (islower(c) ? c - OFFSET : c);
}