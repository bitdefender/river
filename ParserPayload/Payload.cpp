#include "http_parser.h"

#include <malloc.h>
#include <assert.h>
#include <string.h>
//#include <stdio.h>
#include <stdlib.h>

static http_parser *parser = nullptr;

static int currently_parsing_eof;

#define MAX_HEADERS 13
#define MAX_ELEMENT_SIZE 2048
#define MAX_CHUNKS 16

#define TRUE 1
#define FALSE 0

struct message {
	const char *name; // for debugging purposes
	const char *raw;
	enum http_parser_type type;
	enum http_method method;
	int status_code;
	char response_status[MAX_ELEMENT_SIZE];
	char request_path[MAX_ELEMENT_SIZE];
	char request_url[MAX_ELEMENT_SIZE];
	char fragment[MAX_ELEMENT_SIZE];
	char query_string[MAX_ELEMENT_SIZE];
	char body[MAX_ELEMENT_SIZE];
	size_t body_size;
	const char *host;
	const char *userinfo;
	uint16_t port;
	int num_headers;
	enum { NONE = 0, FIELD, VALUE } last_header_element;
	char headers[MAX_HEADERS][2][MAX_ELEMENT_SIZE];
	int should_keep_alive;

	int num_chunks;
	int num_chunks_complete;
	int chunk_lengths[MAX_CHUNKS];

	const char *upgrade; // upgraded body

	unsigned short http_major;
	unsigned short http_minor;

	int message_begin_cb_called;
	int headers_complete_cb_called;
	int message_complete_cb_called;
	int message_complete_on_eof;
	int body_is_final;
};

static struct message messages[5];
static int num_messages;
static http_parser_settings *current_pause_parser;

/*void my_memset(void *buffer, unsigned int value, unsigned int size) {
	for (unsigned int i = 0; i < size; ++i) {
		((char *)buffer)[i] = value;
	}
}

void my_memcpy(void *dest, const void *src, unsigned int size) {
	for (unsigned int i = 0; i < size; ++i) {
		((char *)dest)[i] = ((char *)src)[i];
	}
}

int my_strlen(const char *str) {
	const char *c = str;
	while (*c) c++;

	return c - str;
}

int my_strnlen(const char *str, int n) {
	for (int i = 0; i < n; ++i) {
		if ('\0' == str[i]) {
			return i;
		}
	}
	return n;
}*/

void parser_init(enum http_parser_type type) {
	num_messages = 0;

	parser = (http_parser *)malloc(sizeof(*parser));
	http_parser_init(parser, type);

	/*my_*/memset(&messages, 0, sizeof(messages));
}

void parser_free() {
	assert(parser);
	free(parser);
	parser = nullptr;
}

size_t strlncat(char *dst, size_t len, const char *src, size_t n) {
	size_t slen;
	size_t dlen;
	size_t rlen;
	size_t ncpy;

	slen = /*my_*/strnlen(src, n);
	dlen = /*my_*/strnlen(dst, len);

	if (dlen < len) {
		rlen = len - dlen;
		ncpy = slen < rlen ? slen : (rlen - 1);
		/*my_*/memcpy(dst + dlen, src, ncpy);
		dst[dlen + ncpy] = '\0';
	}

	assert(len > slen + dlen);
	return slen + dlen;
}

size_t strlcat(char *dst, const char *src, size_t len) {
	return strlncat(dst, len, src, (size_t)-1);
}

size_t strlncpy(char *dst, size_t len, const char *src, size_t n) {
	size_t slen;
	size_t ncpy;

	slen = /*my_*/strnlen(src, n);

	if (len > 0) {
		ncpy = slen < len ? slen : (len - 1);
		/*my_*/memcpy(dst, src, ncpy);
		dst[ncpy] = '\0';
	}

	assert(len > slen);
	return slen;
}

size_t strlcpy(char *dst, const char *src, size_t len) {
	return strlncpy(dst, len, src, (size_t)-1);
}


int message_begin_cb(http_parser *p) {
	assert(p == parser);
	messages[num_messages].message_begin_cb_called = TRUE;
	return 0;
}

int request_url_cb(http_parser *p, const char *buf, size_t len) {
	assert(p == parser);
	strlncat(messages[num_messages].request_url,
		sizeof(messages[num_messages].request_url),
		buf,
		len);
	return 0;
}

int response_status_cb(http_parser *p, const char *buf, size_t len)
{
	assert(p == parser);
	strlncat(messages[num_messages].response_status,
		sizeof(messages[num_messages].response_status),
		buf,
		len);
	return 0;
}

int
header_field_cb(http_parser *p, const char *buf, size_t len)
{
	assert(p == parser);
	struct message *m = &messages[num_messages];

	if (m->last_header_element != m->FIELD)
		m->num_headers++;

	strlncat(m->headers[m->num_headers - 1][0],
		sizeof(m->headers[m->num_headers - 1][0]),
		buf,
		len);

	m->last_header_element = m->FIELD;

	return 0;
}

int header_value_cb(http_parser *p, const char *buf, size_t len) {
	assert(p == parser);
	struct message *m = &messages[num_messages];

	strlncat(m->headers[m->num_headers - 1][1],
		sizeof(m->headers[m->num_headers - 1][1]),
		buf,
		len);

	m->last_header_element = m->VALUE;

	return 0;
}

void check_body_is_final(const http_parser *p) {
	if (messages[num_messages].body_is_final) {
		/*fprintf(stderr, "\n\n *** Error http_body_is_final() should return 1 "
			"on last on_body callback call "
			"but it doesn't! ***\n\n");*/
		assert(0);
		abort();
	}
	messages[num_messages].body_is_final = http_body_is_final(p);
}


int headers_complete_cb(http_parser *p) {
	assert(p == parser);
	messages[num_messages].method = (http_method)parser->method;
	messages[num_messages].status_code = parser->status_code;
	messages[num_messages].http_major = parser->http_major;
	messages[num_messages].http_minor = parser->http_minor;
	messages[num_messages].headers_complete_cb_called = TRUE;
	messages[num_messages].should_keep_alive = http_should_keep_alive(parser);
	return 0;
}

int body_cb(http_parser *p, const char *buf, size_t len) {
	assert(p == parser);
	strlncat(messages[num_messages].body,
		sizeof(messages[num_messages].body),
		buf,
		len);
	messages[num_messages].body_size += len;
	check_body_is_final(p);
	// printf("body_cb: '%s'\n", requests[num_messages].body);
	return 0;
}

int message_complete_cb(http_parser *p) {
	assert(p == parser);
	if (messages[num_messages].should_keep_alive != http_should_keep_alive(parser))
	{
		/*fprintf(stderr, "\n\n *** Error http_should_keep_alive() should have same "
			"value in both on_message_complete and on_headers_complete "
			"but it doesn't! ***\n\n");*/
		assert(0);
		abort();
	}

	if (messages[num_messages].body_size &&
		http_body_is_final(p) &&
		!messages[num_messages].body_is_final)
	{
		/*fprintf(stderr, "\n\n *** Error http_body_is_final() should return 1 "
			"on last on_body callback call "
			"but it doesn't! ***\n\n");*/
		assert(0);
		abort();
	}

	messages[num_messages].message_complete_cb_called = TRUE;

	messages[num_messages].message_complete_on_eof = currently_parsing_eof;

	num_messages++;
	return 0;
}

int chunk_header_cb(http_parser *p) {
	assert(p == parser);
	int chunk_idx = messages[num_messages].num_chunks;
	messages[num_messages].num_chunks++;
	if (chunk_idx < MAX_CHUNKS) {
		messages[num_messages].chunk_lengths[chunk_idx] = p->content_length;
	}

	return 0;
}

int chunk_complete_cb(http_parser *p) {
	assert(p == parser);

	/* Here we want to verify that each chunk_header_cb is matched by a
	* chunk_complete_cb, so not only should the total number of calls to
	* both callbacks be the same, but they also should be interleaved
	* properly */
	assert(messages[num_messages].num_chunks ==
		messages[num_messages].num_chunks_complete + 1);

	messages[num_messages].num_chunks_complete++;
	return 0;
}

static http_parser_settings settings = { 
	message_begin_cb,
	request_url_cb,
	response_status_cb,
	header_field_cb,
	header_value_cb,
	headers_complete_cb,
	body_cb,
	message_complete_cb,
	chunk_header_cb,
	chunk_complete_cb
};

size_t parse(const char *buf, size_t len) {
	size_t nparsed;
	currently_parsing_eof = (len == 0);
	nparsed = http_parser_execute(parser, &settings, buf, len);
	return nparsed;
}

void test_simple(const char *buf) {
	parser_init(HTTP_REQUEST);

	enum http_errno err;

	parse(buf, /*my_*/strlen(buf));
	err = HTTP_PARSER_ERRNO(parser);
	parse(NULL, 0);

	parser_free();
}

extern "C" {
	__declspec (dllexport) char payloadBuffer[4096];
	__declspec (dllexport) int Payload() {
		test_simple(payloadBuffer);
		return 0;
	}
};

#include <Windows.h>
BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    lpvReserved
) {
	return TRUE;
}