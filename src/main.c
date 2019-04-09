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

	int quoto = 0;
	for(k = buf; *k != '\0'; k++) {
		if(*k == ' ' || *k == '\n' || *k =='\"') {
			if(*k == '\"' && (k == buf || *(k - 1) != '\\')) {
				if(quoto == 0) {
					len++;
					quoto = 1;
					continue;
				}
				else {
					len++;
					quoto = 0;
					continue;
				}
			}
			else if(*k== '\"' && (k != buf && *(k - 1) == '\\')) {
				len++;
				continue;
			}

			if(*k == ' ' && quoto == 1) {
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

char escape[256];

void convert_string(char **buf) {
	int size = 0;
	int backslash = 0;
	int quoto = 0;
	int vari = 0;
	int varilen = 0;

	int len = strlen(*buf);
	int capacity = len;

	char *tmp = (char *)malloc(sizeof(char) * (len + 1));
	char *k;

	for(k = *buf; *k != '\0'; k++) {
		if(backslash == 1) {
			if(!quoto) tmp[size++] = *k;
			else if(*k == '0') {
				tmp[size++] = '\\';
				tmp[size++] = '0';
			}
			else if(escape[*k] == 0) tmp[size++] = *k;
			else tmp[size++] = escape[*k];
			backslash = 0;
		}
		else if(*k == '\\') backslash = 1;
		else if(*k != '\"') {
			if(vari) {
				varilen++;
				continue;
			}
			if(quoto || *k != '$') tmp[size++] = *k;
			else {
				vari = 1;
				varilen = 0;
			}
		}
		else {
			quoto = 1 - quoto;
			if(vari) {
				vari = 0;
				char sbuf[128];
				sbuf[varilen] = '\0';
				strncpy(sbuf, k - varilen, varilen);
				char *s = getenv(sbuf);
				if(s != NULL) {
					int slen = strlen(s);
					if(size + slen > capacity) {
						char *ttmp = (char *)malloc(sizeof(char) * (capacity + slen + 1));
						capacity += slen;
						for(int i = 0; i < size; i++) ttmp[i] = tmp[i];
						free(tmp);
						tmp = ttmp;
					}
					tmp[size] = '\0';
					strcat(tmp, s);
					size += slen;
				}
			}
		}
	}
	if(vari) {
		vari = 0;
		char sbuf[128];
		sbuf[varilen] = '\0';
		strncpy(sbuf, k - varilen, varilen);
		char *s = getenv(sbuf);
		if(s != NULL) {
			int slen = strlen(s);
			if(size + slen > capacity) {
				char *ttmp = (char *)malloc(sizeof(char) * (capacity + slen + 1));
				capacity += slen;
				for(int i = 0; i < size; i++) ttmp[i] = tmp[i];
				free(tmp);
				tmp = ttmp;
			}
			tmp[size] = '\0';
			strcat(tmp, s);
			size += slen;
		}
	}
	tmp[size] = '\0';
	
	free(*buf);
	*buf = tmp;
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

void clear(char ***x) {
	for(char **k = *x; *k != NULL; k++) free(*k);
	free(*x);
}

int main() {
	escape['a'] = '\a';
	escape['b'] = '\b';
	escape['e'] = '\e';
	escape['f'] = '\f';
	escape['n'] = '\n';
	escape['r'] = '\r';
	escape['t'] = '\t';
	escape['v'] = '\v';
	escape['\\'] = '\\';
	escape['\''] = '\'';
	escape['\?'] = '\?';

	char buf[4096];
	while (fgets(buf, 4096, stdin)) {
		char **argv = get_argv(buf);

		for(int i = 0; argv[i] != NULL; i++) {
			convert_string(argv + i);
		}
		if (strcmp(argv[0], "cd") == 0) {
			if(argv[1] == NULL) {
				clear(&argv);
				continue;
			}
			invertHome(argv + 1);

			char pbuf[512];
			if(realpath(argv[1], pbuf) == NULL) printf("디렉토리가 잘못되었습니다.\n");
			else if(chdir(pbuf) == -1) printf("그런 디렉토리가 존재하지 않거나 잘못되었습니다.\n");
		}
		else if (strcmp(argv[0], "ready-to-score") == 0 || strcmp(argv[0], "auto-grade-pa0") == 0 || strcmp(argv[0], "report-grade") == 0) { // 현재가 프로젝트 경로라는 가정하에 진행됨
			char pbuf[512], qbuf[512];
			char str[512];
			str[0] = '\0';
			strcpy(str, "./scripts/");
			strcat(str, argv[0]);
			strcat(str, ".py");
			if(argv[1] == NULL) printf("인자가 부족합니다.\n");
			else if(realpath(str, pbuf) == NULL) printf("디렉토리가 잘못되었습니다.\n");
			else if(realpath(argv[1], qbuf) == NULL) printf("디렉토리가 잘못되었습니다.\n");
			else {
				pid_t pid = fork();
				int status;

				if(pid == -1) {
					fprintf(stderr, "somethings wrong\n");
					exit(EXIT_FAILURE);
				}
				else if(pid == 0) {
					execlp("python3", "python3", pbuf, qbuf, "");
					exit(EXIT_FAILURE);
				}
				else {
					wait(&status);
					/*if(status != 0)
						printf("실행하지 못했습니다.\n");*/
				}
			}
		}
		else if (argv[0][0] != '.' && argv[0][0] != '/' && argv[0][0] != '~') {
			int num;

			int io = -1;
			char *tmp;
			for(num = 0; argv[num] != NULL; num++) {
				if(strcmp(argv[num], "<") == 0) io = 0;
				if(strcmp(argv[num], ">") == 0) io = 1;
				if(io != -1) {
					tmp = argv[num];
					argv[num] = NULL;
					break;
				}
			}
			if(io != -1 && argv[num + 1] == NULL) {
				printf("인자가 부족합니다.\n");
				argv[num] = tmp;
				clear(&argv);
				continue;
			}
			if(io != -1 && (argv[num + 1][0] == '/' || argv[num + 1][0] == '.' || argv[num + 1][0] == '~')) {
				invertHome(argv + num + 1);
				char pbuf[512];
				if(realpath(argv[num + 1], pbuf) == NULL) {
					printf("디렉토리가 잘못되었습니다.\n");
					argv[num] = tmp;
					clear(&argv);
					continue;
				}
				free(argv[num + 1]);
				argv[num + 1] = (char *)malloc(sizeof(char) * 512);
				strcpy(argv[num + 1], pbuf);
			}

			pid_t pid = fork();
			int status;

			if (pid == -1) {
				fprintf(stderr, "Error occured during process creation\n");
				exit(EXIT_FAILURE);
			} else if (pid == 0) {
				if(io == 1) freopen(argv[num + 1], "w", stdout);
				if(io == 0) freopen(argv[num + 1], "r", stdin);
				execvp(argv[0], argv);
				exit(EXIT_FAILURE);
			} else {
				wait(&status);
				/*if(status != 0)
					printf("실행하지 못 했습니다.\n");*/
				if(io != -1) argv[num] = tmp;
			}
		}
		else {
			invertHome(argv);

			char pbuf[512];
			if(realpath(argv[0], pbuf) == NULL) {
				printf("디렉토리가 잘못되었습니다.\n");
				clear(&argv);
				continue;
			}


			free(argv[0]);
			argv[0] = (char *)malloc(sizeof(char) * 512);
			strcpy(argv[0], pbuf);

			int num;

			int io = -1;
			char *tmp;
			for(num = 0; argv[num] != NULL; num++) {
				if(strcmp(argv[num], "<") == 0) io = 0;
				if(strcmp(argv[num], ">") == 0) io = 1;
				if(io != -1) {
					tmp = argv[num];
					argv[num] = NULL;
					break;
				}
			}
			if(io != -1 && argv[num + 1] == NULL) {
				printf("인자가 부족합니다.\n");
				argv[num] = tmp;
				clear(&argv);
				continue;
			}
			if(io != -1 && (argv[num + 1][0] == '/' || argv[num + 1][0] == '.' || argv[num + 1][0] == '~')) {
				invertHome(argv + num + 1);
				char pbuf[512];
				if(realpath(argv[num + 1], pbuf) == NULL) {
					printf("디렉토리가 잘못되었습니다.\n");
					argv[num] = tmp;
					clear(&argv);
					continue;
				}
				free(argv[num + 1]);
				argv[num + 1] = (char *)malloc(sizeof(char) * 512);
				strcpy(argv[num + 1], pbuf);
			}

			pid_t pid = fork();
			int status;

			if (pid == -1) {
				fprintf(stderr, "Error occured during process creation\n");
				exit(EXIT_FAILURE);
			} else if (pid == 0) {
				if(io == 1) freopen(argv[num + 1], "w", stdout);
				if(io == 0) freopen(argv[num + 1], "r", stdin);
				execvp(argv[0], argv);
				exit(EXIT_FAILURE);
			} else {
				wait(&status);
				/*if(status != 0)
					printf("실행하지 못 했습니다.\n");*/
				if(io != -1) argv[num] = tmp;
			}
		}
		clear(&argv);
	}

	return 0;
}
