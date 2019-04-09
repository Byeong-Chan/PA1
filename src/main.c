#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>

char **get_argv(char *buf) {
	int len = 0;
	int capacity = 1, size = 0;
	char **argv = (char **)malloc(sizeof(char *));
	char *k;

	int quto = 0;
	for(k = buf; *k != '\0'; k++) {
		if(*k == ' ' || *k == '\n' || *k =='\"') {
			if(*k == '\"' && (k == buf || *(k - 1) != '\\')) {
				if(quto == 0) {
					len++;
					quto = 1;
					continue;
				}
				else {
					len++;
					quto = 0;
					continue;
				}
			}

			if(*k== '\"' && (k != buf && *(k - 1) == '\\')) {
				len++;
				continue;
			}

			if(*k == ' ' && quto == 1) {
				len++;
				continue;
			}

			if(len) {
				if(capacity == size) {
					capacity <<= 1;
					char **tmp = (char **)malloc(sizeof(char *) * capacity);
					for(int i = 0; i < size; i++) tmp[i] = argv[i];
					free(argv);
					argv = tmp;
				}
				argv[size] = (char *)malloc(sizeof(char) * (len + 1));
				argv[size][len] = '\0';
				strncpy(argv[size], k - len, len);
				size++;
			}

			len = 0;
		}
		else len++;
	}

	if(len) {
		if(capacity == size) {
			capacity <<= 1;
			char **tmp = (char **)malloc(sizeof(char *) * capacity);
			for(int i = 0; i < size; i++) tmp[i] = argv[i];
			free(argv);
			argv = tmp;
		}
		argv[size] = (char *)malloc(sizeof(char) * (len + 1));
		argv[size][len] = '\0';
		strncpy(argv[size], k - len, len);
		size++;
		len = 0;
	}

	if(capacity == size) {
		capacity <<= 1;
		char **tmp = (char **)malloc(sizeof(char *) * capacity);
		for(int i = 0; i < size; i++) tmp[i] = argv[i];
		free(argv);
		argv = tmp;
	}
	argv[size] = NULL;
	
	return argv;
}

void invertHome(char **buf) {
	char *home = getenv("HOME");
	int homelen = strlen(home);

	int len = strlen(*buf);
	int nextlen = len;
	for(int i = 0; i < len; i++) {
		if((*buf)[i] == '~') nextlen += homelen - 1;
	}

	char *tmp = (char *)malloc(sizeof(char) * (nextlen + 1));
	tmp[nextlen] = '\0';
	int size = 0;
	for(int i = 0; i < len; i++) {
		char v = (*buf)[i];
		if(v == '~') {
			tmp[size] = '\0';
			strcat(tmp, home);
			size += homelen;
		}
		else tmp[size++] = v;
	}

	free(*buf);
	(*buf) = tmp;
}

int main() {
	char buf[4096];
	while (fgets(buf, 4096, stdin)) {
		char **argv = get_argv(buf);

		for(char **k = argv; *k != NULL; k++) printf("%s\n", *k);

		if (strcmp(argv[0], "cd") == 0) {
			if(argv[1] == NULL) continue;
			invertHome(argv + 1);

			char pbuf[512];
			if(realpath(argv[1], pbuf) == NULL) printf("디렉토리가 잘못되었습니다.\n");
			else if(chdir(pbuf) == -1) printf("그런 디렉토리가 존재하지 않거나 잘못되었습니다.\n");
		}
		else if (argv[0][0] != '.' && argv[0][0] != '/') {
			pid_t pid = fork();
			int status;

			if (pid == -1) {
				fprintf(stderr, "Error occured during process creation\n");
				exit(EXIT_FAILURE);
			} else if (pid == 0) {
				execvp(argv[0], argv);
			} else {
				wait(&status);
			}
		}
		else {
		}

		for(char **k = argv; *k != NULL; k++) free(*k);
		free(argv);
	}

	return 0;
}
