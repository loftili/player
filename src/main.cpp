#include <stdio.h>
#include <string.h>

int l_substr(const char *needle, const char *stack) {
  int i = 0, j = 0, m = 0,
      l = strlen(stack),
      n = strlen(needle);

  char np, sp;

  for(i = 0; i < l; i++) {
    for(j = 0; j < n; j++) {
      np = needle[j];
      sp = stack[i + j];
      if(np != sp) break;
    }

    if(j == n) return i;
  }


  return -1;
}

void trim(char* str) {
  char* newline_post = strchr(str, '\n');
  if(newline_post)
    *newline_post = '\0';
}

int main(int argc, char* argv[]) {
  char *needle = 0,
       *haystack = 0,
       *result;

  int substr_result = 0;

  bool exit_command = false;

  while(!exit_command) {
    haystack = new char[100];
    needle = new char[100];

    printf("plese enter two strings (exit to exit): \nhaystack: ");
    fgets(haystack, 99, stdin);

    result = strstr(haystack, "exit");
    if(result) {
      exit_command = true;
      break;
    }

    printf("needle: ");

    result = strstr(needle, "exit");
    if(result) {
      exit_command = true;
      break;
    }

    fgets(needle, 99, stdin);

    trim(needle);
    trim(haystack);

    printf("comparing needle[%s] to haystack[%s] \n", needle, haystack);
    substr_result = l_substr(needle, haystack);
    printf("result: [%d]\n", substr_result);

    delete needle;
    delete haystack;
  }

  return 0;
}
