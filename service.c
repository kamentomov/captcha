#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

char** split_by_one_char (char* a_str, const char a_delim) {
  char** result    = 0;
  size_t count     = 0;
  char* tmp        = a_str;
  char* last_comma = 0;
  char delim[2];
  delim[0] = a_delim;
  delim[1] = 0;

  /* Count how many elements will be extracted. */
  while (*tmp) {
    if (a_delim == *tmp) {
      count++;
      last_comma = tmp;
    }
    tmp++;
  }

  /* Add space for trailing token. */
  count += last_comma < (a_str + strlen(a_str) - 1);

  /* Add space for terminating null string so caller
     knows where the list of returned strings ends. */
  count++;

  result = malloc(sizeof(char*) * count);

  if (result) {
    size_t idx  = 0;
    char* token = strtok(a_str, delim);

    while (token) {
      assert(idx < count);
      *(result + idx++) = strdup(token);
      token = strtok(0, delim);
    }
    assert(idx == count - 1);
    *(result + idx) = 0;
  }

  return result;
}

#define MAX_BUF 1024
#define QUESTION 0
#define ANSWER 1

int main() {
  FILE* pipe_tolisp;
  FILE* pipe_fromlisp;
  const char* tolisp = "/home/kamen/mighty/cgi-bin/tolisp.tmp";
  const char* fromlisp = "/home/kamen/mighty/cgi-bin/fromlisp.tmp";
  char* line = NULL;
  size_t len = 0;
  ssize_t result;
  char buf[MAX_BUF];
  char** tokens;
  int service_type;
  char query_string[MAX_BUF];
  char path_info[MAX_BUF];

  /* Study the CGI environment */
  strncpy(query_string, getenv("QUERY_STRING"), MAX_BUF);
  strncpy(path_info, getenv("PATH_INFO"), MAX_BUF);

  printf("Content-type: text/html\n\n");
  printf(
         "<html>\n"
         "<head>\n"
         "<title>Text Captcha</title>\n"
         "</head>\n"
         "<body>\n"
         "<h1>Text Captcha</h1>\n"
         );

  /* Determine the service type */
  if (strncmp(path_info, "/question", 4) == 0)
    service_type = QUESTION;
  else if (strncmp(path_info, "/answer", 4) == 0)
    service_type = ANSWER;
  else {
    printf("<p>The operation is not supported.</p>\n");
    return 0;
  }

  /* Forward request to the Lisp process */
  pipe_tolisp = fopen(tolisp, "w");
  if (pipe_tolisp == NULL) exit(EXIT_FAILURE);
  len = fwrite(path_info, 1, strlen(path_info), pipe_tolisp);
  if (service_type == ANSWER) fputs("&", pipe_tolisp);
  len = fwrite(query_string, 1, strlen(query_string), pipe_tolisp);
  fputs("\n", pipe_tolisp);
  fclose(pipe_tolisp);

  /* Receive response from the Lisp process */ 
  pipe_fromlisp = fopen(fromlisp, "r");
  if (pipe_fromlisp == NULL) exit(EXIT_FAILURE);
  len = getline(&line, &len, pipe_fromlisp);
  line[len-1] = 0; // remove the newline character
  fclose(pipe_fromlisp);

  /* Process the response */
  tokens = split_by_one_char(line, '|');
  if(!tokens) return 1;
  switch (service_type) {
  case QUESTION: /* the form's method could be ANSWER but this limits the 
               of supported web browsers */
    printf("<form action=\"/cgi-bin/a.out/put\" method=\"get\">\n"
           "<p>%s</p>\n"
           "<input type=\"text\" name=\"answer\">\n"
           "<input type=\"hidden\" name=\"ix\" value=\"%d\">\n"
           "<input type=\"submit\">\n"
           "</form>\n",
           *(tokens + 0),
           atoi(*(tokens + 1)));
    break;
  case ANSWER:
    { //ix, answer, correct
      int ix = atoi(*(tokens + 0));
      int answer = atoi(*(tokens + 1));
      int correct = atoi(*(tokens + 2));
      printf(answer==correct ? "<p>Correct!</p>\n": "<p>Incorrect!</p>\n");
      printf("<br>\n"
             "<a href=\"/cgi-bin/a.out/question\">Try again</>\n");
    }
  }
  for(int i = 0; *(tokens + i); i++)
    free(*(tokens + i));
  free(tokens);
  free(line);

  /* mkfifo(tolisp, 0640); */
  /* mkfifo(fromlisp, 0640); */
  /* unlink(tolisp); */
  /* unlink(fromlisp); */

  printf(
         "<address>\n"
         "<a href=\"mailto:ktomov@hotmail.com\"></a>\n"
         "</address>\n"
         "</body>\n"
         "</html>\n"
         );
  return 0;
}
