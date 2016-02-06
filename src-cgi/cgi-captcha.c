//
// Copyright (c) 2016 Kamen Tomov, All Rights Reserved
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//   * Redistributions in binary form must reproduce the above
//     copyright notice, this list of conditions and the following
//     disclaimer in the documentation and/or other materials
//     provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR 'AS IS' AND ANY EXPRESSED
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// cgi-captcha.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define QUESTION 1
#define ANSWER 2

static const char* tolisp = "/tmp/tolisp.tmp";
static const char* fromlisp = "/tmp/fromlisp.tmp";
static const char* http200 = "Status: 200 OK \r\n";
static const char* http400 = "Status: 400 Bad Request \r\n";
static const char* http405 = "Status: 405 Method Not Allowed\r\n";

int determine_service_type (const char* path_info) {
  if (strcmp(path_info, "/question") == 0)
    return QUESTION;

  if (strcmp(path_info, "/answer") == 0)
    return ANSWER;

  return 0;
}

int forward_request_tolisp (const char* path_info,
                            const char* query_string,
                            int service_type) {
  FILE* pipe_tolisp = fopen(tolisp, "w");
  if (!pipe_tolisp)
    return 0;

  size_t len = strlen(path_info);
  
  if (len != fwrite(path_info, 1, len, pipe_tolisp)) {
    fclose(pipe_tolisp);
    return 0;
  }

  if (service_type == ANSWER) {
    fputs("?", pipe_tolisp); 
    fwrite(query_string, 1, strlen(query_string), pipe_tolisp);
  }

  fputs("\n", pipe_tolisp);
  fclose(pipe_tolisp);

  return 1;
}

int not_err_response (const char* response) {
  if (strncmp(response, "-1", 2) == 0)
    return 0;

  if (strncmp(response, "-2", 2) == 0) 
    return 0;

  return 1;
}

int receive_response_fromlisp (char** line) {
  FILE* pipe_fromlisp = fopen(fromlisp, "r");
  if (!pipe_fromlisp) return 0;

  size_t len = getline(line, &len, pipe_fromlisp);
  fclose(pipe_fromlisp);

  if (len == -1) {
    *line = strdup("-3");
    return 0;
  }

  if (len > 1) 
    (*line)[len-1] = 0; // remove the newline character

  if (!not_err_response(*line))
    return 0;
  return 1;
}

char** split_by_one_char (char* a_str, const char a_delim) {
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

  char** result = malloc(sizeof(char*) * count);

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

void generate_open_tag (const char* status) {
  printf("%s", status);
  printf("Content-type: text/html \n\n");
  printf("<html>\n"
         "<head>\n"
         "<title>Text CAPTCHA</title>\n"
         "</head>\n"
         "<body>\n"
         "<h1>Text CAPTCHA</h1>\n"
         );
}

void generate_close_tag () {
  printf("<address>\n"
         "<a href=\"mailto:ktomov@hotmail.com\"></a>\n"
         "</address>\n"
         "</body>\n"
         "</html>\n"
         );
}

int main() {
  char* query_string = getenv("QUERY_STRING");
  char* path_info = getenv("PATH_INFO");
  if (!(query_string && path_info)) {
    generate_open_tag(http405);
    printf("<p>Wrong environment! Please use it as a CGI script!</p>\n");
    generate_close_tag();
    return 1;  
  }

  /* Determine the service type */
  int service_type = determine_service_type(path_info);
  if (!service_type) {
    generate_open_tag(http405);
    printf("<p>No such service.</p>\n");
    generate_close_tag();
    return 1;  
  }

  /* Forward request to the Lisp process */
  if (!(forward_request_tolisp(path_info, query_string, service_type))) {
    generate_open_tag(http405);
    printf("<p>Failed to forward the request to the Lisp server.</p>\n");
    generate_close_tag();
    return 1;  
  }

  /* Receive response from the Lisp process */ 
  char* line = NULL;
  if (!(receive_response_fromlisp(&line))) {
    generate_open_tag(http405);
    printf("<p>Failed to receive a valid response from the Lisp server. Error %s.</p>\n",
           line);
    generate_close_tag();
    free(line);
    return 1;  
  }
  if (!line) {
    generate_open_tag(http405);
    printf("<p>Don't know how to process empty response.</p>\n");
    generate_close_tag();
    return 1;
  }

  /* Process the response */
  char** tokens = NULL; 
  if(!(tokens = split_by_one_char(line, '|'))) {
    generate_open_tag(http405);
    printf("<p>Failed to process response '%s' from the Lisp server.</p>\n", line);
    generate_close_tag();
    free(line);
    return 1;
  }

  switch (service_type) {
  case QUESTION: /* the form's method could be PUT but this limits the 
                    number of supported web browsers */
    generate_open_tag(http200);
    for(int i = 0; i < 2; i++)
      assert(*(tokens + i));
    printf("<form action=\"/cgi-bin/cgi-captcha/answer\" method=\"get\">\n"
           "<p>%s</p>\n"
           "<input type=\"text\" name=\"guess\">\n"
           "<input type=\"hidden\" name=\"ix\" value=\"%d\">\n"
           "<input type=\"submit\">\n"
           "</form>\n",
           *(tokens + 0),
           atoi(*(tokens + 1)));
    generate_close_tag();
    break;
  case ANSWER:
    for(int i = 0; i < 3; i++)
      assert(*(tokens + i));
    //the order is: id, guess, correct
    int guess = atoi(*(tokens + 1));
    int correct = atoi(*(tokens + 2));
    if (guess == correct) {
      generate_open_tag(http200); 
      printf("<p>Correct!</p>\n");
    } else {
      generate_open_tag(http400);
      printf("<p>Incorrect!</p>\n");
    }
    printf("<br>\n"
           "<a href=\"/cgi-bin/cgi-captcha-service/question\">Try again</>\n");
    generate_close_tag();
  }

  for(int i = 0; *(tokens + i); i++)
    free(*(tokens + i));
  free(tokens);
  free(line);

  return 0;
}
