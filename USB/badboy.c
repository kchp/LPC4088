
#include "badboy.h"
#include <stdio.h>
#include <ctype.h>

char *s_gets(char *st, int n)
{
	char *ret_val;
	int i = 0;

	ret_val = fgets(st, n, stdin);
	if(ret_val)
	{
		while(st[i] != '\n' && st[i] != '\0')
			i++;
		if(st[i] == '\n')
			st[i] = '\0';
		else
			while(getchar() != '\n')
				continue;
	}
	return ret_val;
}

char *toUP(char *st)
{
	int i = 0;
	while(st[i] != '\0')
	{
		if(st[i] >= 'A' && st[i] <= 'Z')
			tolower(st[i]);
		i++;
	}
	return st;
}

char *toLOW(char *st)
{
	int i = 0;
	while(st[i] != '\0')
	{
		if(st[i] >= 'a' && st[i] <= 'z')
			toupper(st[i]);
		i++;
	}
	return st;
}
