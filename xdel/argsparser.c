#include "argsparser.h"

#include <stdlib.h>

parsed_args* parse_args(int argc, TCHAR* argv[]) {
	int i;
	parsed_args* args = (parsed_args*)malloc(sizeof(parsed_args));
	args->program = argv[0];
	args->arg_count = 0;
	args->file_count = 0;
	args->args = (TCHAR**)malloc(sizeof(TCHAR*)*argc);
	args->files = (TCHAR**)malloc(sizeof(TCHAR*)*argc);
	for (i = 1; i < argc; ++i)
		if (argv[i][0] == _T('-'))
			args->args[args->arg_count++] = argv[i];
		else
			args->files[args->file_count++] = argv[i];
	return args;
}

int has_arg(parsed_args* args, TCHAR* lng, TCHAR shrt) {
	int i;
	for (i = 0; i < args->arg_count; ++i) {
		if (args->args[i][0] == _T('-') && args->args[i][1] == _T('-')) {
			if (lng && _tcscmp(args->args[i]+2, lng) == 0) return 1;
		} else if (shrt && _tcschr(args->args[i]+1, shrt)) return 1;
	}
	return 0;
}

void free_parsed_args(parsed_args* args) {
	free(args->files);
	free(args->args);
	free(args);
}
