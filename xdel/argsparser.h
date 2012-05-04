#ifndef _ARGSPARSER_H_
#define _ARGSPARSER_H_

#define _CRT_SECURE_NO_WARNINGS
#include <tchar.h>

typedef struct _parsed_args
{
	TCHAR* program;

	int file_count;
	TCHAR** files;

	int arg_count;
	TCHAR** args;

} parsed_args;

parsed_args* parse_args(int argc, TCHAR* argv[]);
int has_arg(parsed_args* args, TCHAR* lng, TCHAR shrt);
void free_parsed_args(parsed_args* args);

#endif
