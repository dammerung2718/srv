/* C stdlib */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>

/* needed for networking */
#include <sys/socket.h>
#include <netinet/in.h>

/* needed for multi-threadding */
#include <pthread.h>

/* needed for file systems */
#include <fcntl.h>
#include <sys/stat.h>

/* config */
#define PORT    8080
#define BUFSIZE (1024 * 1024)

/* types */
struct http_request {
	char *path;
	char *error;
};

/* functions */
static int
ends_with(const char *str, const char *suffix) {
	size_t str_len = strlen(str);
	size_t suffix_len = strlen(suffix);
	return (str_len >= suffix_len) && (!memcmp(str + str_len - suffix_len, suffix, suffix_len));
}

static int
parse_http(struct http_request *request, char *buffer)
{
	char s[1024];
	int i;

	assert(request != NULL);
	assert(buffer != NULL);

	memset(s, 0, sizeof(s));

	/* check method, can only be GET */
	if (strncmp(buffer, "GET ", 4) != 0) {
		sprintf(s, "unsupported method\n");
		request->error = strdup(s);
		return 1;
	}
	buffer += 4;

	/* get path */
	i = 0;
	while (!isspace(*buffer)) {
		s[i++] = *buffer++;
	}
	request->path = strdup(s);
	memset(s, 0, sizeof(s));

	/* done */
	return 0;
}

static void
write_response(int fd, int status_code, char *mime_type, char *body)
{
	char *buffer;

	buffer = malloc(BUFSIZE);
	snprintf(
		buffer,
		BUFSIZE,
			"HTTP/1.1 %d OK\r\n"
			"Content-Type: %s\r\n"
			"\r\n"
			"%s",
		status_code,
		mime_type,
		body
	);

	send(fd, buffer, strlen(buffer), 0);
	free(buffer);
}

static void *
serve(void *arg)
{
	char *buffer, *mime_type;
	int client, bytes_received, file;
	struct http_request request;
	struct stat file_stat;
	off_t file_size;

	client = *(int*)arg;

	/* read request */
	buffer = malloc(BUFSIZE);
	bytes_received = recv(client, buffer, BUFSIZE, 0);
	if (bytes_received == 0) {
		close(client);
		free(arg);
		free(buffer);
		return NULL;
	}

	if (parse_http(&request, buffer) != 0) {
		write_response(client, 400, "text/plain", "bad request");
		close(client);
		free(arg);
		free(buffer);
		return NULL;
	}
	printf("GET %s\n", request.path);

	/* open file */
	file = open(request.path + 1, O_RDONLY);
	if (file == -1) {
		write_response(client, 404, "text/plain", "not found");
		close(client);
		free(arg);
		free(buffer);
		return NULL;
	}

	/* get file size */
	fstat(file, &file_stat);
	file_size = file_stat.st_size;

	/* read file */
	buffer = realloc(buffer, file_size + 1);
	memset(buffer, 0, file_size + 1);
	read(file, buffer, file_size + 1);
	close(file);

	/* find mime type */
	mime_type = "text/plain";
	if (ends_with(request.path, ".html")) {
		mime_type = "text/html";
	}

	/* write response */
	write_response(client, 200, mime_type, buffer);
	close(client);
	free(buffer);

	/* done */
	free(arg);
	return NULL;
}

int
main()
{
	int server;
	struct sockaddr_in server_addr;

	/* create socket */
	if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	/* bind socket */
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);
	if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	/* start listening */
	if (listen(server, 10) < 0) {
		perror("listen failed");
		exit(EXIT_FAILURE);
	}

	/* main loop */
	printf("Listening on port %d...\n", PORT);
	while (true) {
		int *client = malloc(sizeof(int));
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);

		/* accept incoming request */
		if ((*client = accept(server, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
			perror("accept failed");
			continue;
		}

		/* spawn thread to respond */
		pthread_t thread_id;
		pthread_create(&thread_id, NULL, serve, (void*)client);
		pthread_detach(thread_id);
	}

	/* cleanup */
	close(server);
}
