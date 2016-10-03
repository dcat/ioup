/**
 *      ioup.c - upload files to pub.iotek.org
 *
 *
 *      Copyright (c) 2014, Broseph <dcat (at) iotek (dot) org>
 *
 *      Permission to use, copy, modify, and/or distribute this software for any
 *      purpose with or without fee is hereby granted, provided that the above
 *      copyright notice and this permission notice appear in all copies.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *      WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *      MERCHANTABILITY AND FITNESS IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *      ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *      WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *      ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *      OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <curl/curl.h>

#define IOUP_BASE	"http://pub.iotek.org/"
#define IOUP_LIST	IOUP_BASE "p/list.php"
#define IOUP_REMOVE	IOUP_BASE "remove.php"
#define IOUP_UPLOAD	IOUP_BASE "post.php"
#define IOUP_TMP_FILE	"/tmp/ioup.stdin"

typedef struct {
	const char *xt;
	char *token,*name,*file;
	bool list,f_list,remove,std_in;
} ioup_t;

typedef struct {
	char *ptr;
	size_t len;
} string;

/* get string after last dot */
const char *
last_dot (const char *s) {
	const char *d = strrchr(s, '.');
	return ! d || d == s ? NULL : d + 1;
}

void
init_string(string *s) {
	s->len = 0;
	s->ptr = malloc(s->len+1);
	if (s->ptr == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->ptr[0] = '\0';
}

size_t
writefunc(void *ptr, size_t size, size_t nmemb, string *s) {
	size_t new_len = s->len + size*nmemb;
	s->ptr = realloc(s->ptr, new_len+1);
	if (s->ptr == NULL) {
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(s->ptr+s->len, ptr, size*nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size*nmemb;
}

void
prepend_url (string *s) {
	char *s_p;
	for (s_p = s->ptr; *s_p != '\0'; s_p++) {
		if (s_p == s->ptr) {
			printf("%s", IOUP_BASE);
		}
		else if (*s_p == '\n' && *(s_p+1) != '\0') {
			printf("\n%s", IOUP_BASE);
		}
		else {
			printf("%c", *s_p);
		}
	}
}

void
io_post (ioup_t io) {
	FILE *in,*out;
	char *url = io.list ? IOUP_LIST : IOUP_UPLOAD;
	int chr;
	CURL *c;
	CURLcode res;

	struct curl_httppost *formpost  = NULL;
	struct curl_httppost *lastptr   = NULL;
	struct curl_slist *headerlist   = NULL;
	struct stat sstat;

	static const char buf[] = "Expect:";

	/* read from stdin */
	if (io.std_in) {
		/* interactive mode? */
		fstat(0, &sstat);
		if (S_ISCHR(sstat.st_mode))
			puts("^C: exit, ^D: post");
		
		io.file = IOUP_TMP_FILE;
		io.name = "stdin";
		out     = fopen(io.file, "w");
		in      = stdin;

		/* write to temp file */
		if (in && out) {
			while (!feof(in)) {
				chr = fgetc(in);
				if (chr != EOF)
					fputc(chr, out);
			}
		}

		fclose(in);
		fclose(out);
	} else
		io.xt = last_dot(io.file);

	curl_global_init(CURL_GLOBAL_ALL);

	curl_formadd(	&formpost,
			&lastptr,
			CURLFORM_COPYNAME, "token",
			CURLFORM_COPYCONTENTS, io.token,
			CURLFORM_END);

	curl_formadd(	&formpost,
			&lastptr,
			CURLFORM_COPYNAME, "is_ioup",
			CURLFORM_COPYCONTENTS, "1",
			CURLFORM_END);

	if (!io.list && !io.remove) {
		curl_formadd(	&formpost,
				&lastptr,
				CURLFORM_COPYNAME, "xt",
				CURLFORM_COPYCONTENTS, io.xt,
				CURLFORM_END);

		curl_formadd(	&formpost,
				&lastptr,
				CURLFORM_COPYNAME, "pdata",
				CURLFORM_FILE, io.file,
				CURLFORM_END);
	}

	if (io.remove) {
		url = IOUP_REMOVE;
		curl_formadd(	&formpost,
				&lastptr,
				CURLFORM_COPYNAME, io.file,
				CURLFORM_COPYCONTENTS, "1",
				CURLFORM_END);
	}

	curl_formadd(	&formpost,
			&lastptr,
			CURLFORM_COPYNAME, "submit",
			CURLFORM_COPYCONTENTS, "sent",
			CURLFORM_END);

	c = curl_easy_init();
	headerlist = curl_slist_append(headerlist, buf);

	if (c) {
		curl_easy_setopt(c, CURLOPT_URL, url);
		curl_easy_setopt(c, CURLOPT_HTTPHEADER, headerlist);
		curl_easy_setopt(c, CURLOPT_HTTPPOST, formpost);


		string s;
		init_string(&s);

		curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, writefunc);
		curl_easy_setopt(c, CURLOPT_WRITEDATA, &s);

		res = curl_easy_perform(c);
		if (res != CURLE_OK)
			fprintf(stderr, "curl error: %s",
					curl_easy_strerror(res));

		if (io.std_in)
			unlink(io.file);

		curl_easy_cleanup(c);
		curl_formfree(formpost);
		curl_slist_free_all(headerlist);

		if (io.f_list)
			prepend_url(&s);
		else
			printf("%s\n", s.ptr);

		free(s.ptr);
	}
}

int main (int argc, char *argv[]) {
	ioup_t io;
	/* if TOKEN is defined, hardcode it */
	io.token = getenv("IOUP_TOKEN");
	io.list = io.remove = io.f_list = io.std_in = false;
	io.file = (argc >= 2) ? argv[1] : NULL;
	io.name = (argc >= 2) ? argv[1] : "stdin";

	if (argc >= 2 && argv[1][0] == '-') {
		switch  (argv[1][1]) {
			case 'l':
				io.list = true;
				break;
			case 'L':
				io.f_list = true;
				io.list = true;
				break;
			case 'r':
				if (argc > 1) {
					printf("%s\n", argv[2]);
					io.file = argv[2];
					io.remove = true;
				}
				break;
			case 'V':
				printf("ioup-%s\n", VERSION);
				return 0;
		}
	}
	
	if (argc < 2) io.std_in = true;
	io_post(io);

	return 0;
}
