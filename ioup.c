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
	bool list,remove,std_in;
} ioup_t;

/* get string after last dot */
const char *last_dot (const char *s) {
	const char *d = strrchr(s, '.');
	return ! d || d == s ? NULL : d + 1;
}

/* read $HOME/.iouprc */
#ifndef TOKEN
char *read_iouprc (void) {
	int i,c;
	FILE *fp;
	char *path,token[12],*r;

	asprintf(&path, "%s/.iouprc", getenv("HOME"));

	/* byte by byte read */
	if ((fp = fopen(path, "r"))) {
		for (i=0; i < 10; i++) {
			c = fgetc(fp);
			token[i] = c && c != EOF ? (char) c : 0;
		}
		token[11] = '\0';
	}
	else
		return NULL;

	r = malloc(sizeof(char)*10);
	strncpy(r, token, 10);

	return r;
}
#endif

void io_post (ioup_t io) {
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
		res = curl_easy_perform(c);
		if (res != CURLE_OK)
			fprintf(stderr, "curl error: %s",
					curl_easy_strerror(res));
		if (io.std_in)
			unlink(io.file);
		curl_easy_cleanup(c);
		curl_formfree(formpost);
		curl_slist_free_all(headerlist);
	}
}

int main (int argc, char *argv[]) {
	ioup_t io;
	/* if TOKEN is defined, hardcode it */
	io.token =
#ifdef TOKEN
	TOKEN;
#else
	read_iouprc();
#endif
	io.list = io.remove = io.std_in = false;
	io.file = (argc >= 2) ? argv[1] : NULL;
	io.name = (argc >= 2) ? argv[1] : "stdin";

	if (argc >= 2 && argv[1][0] == '-') {
		switch  (argv[1][1]) {
			case 'l':
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
	putchar('\n');

	return 0;
}
