/* vichy_test.c
 * A simple example of a C program to test some of the
 * operations of the "/dev/vichy" device (a.k.a "vichy0")
 */
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

int main() {
   int fd, result, len;
   char buf[10];
   const char *str;
   if ((fd = open("/dev/vichy", O_WRONLY)) == -1) {
      perror("1. open failed");
      return -1;
   }

   str = "abcde"; 
   len = strlen(str);
   if ((result = write(fd, str, len)) != len) {
      perror("1. write failed");
      return -1;
   }
   close(fd);

   if ((fd = open("/dev/vichy", O_RDONLY)) == -1) {
      perror("2. open failed");
      return -1;
   }
   if ((result = read(fd, &buf, sizeof(buf))) != len) {
      fprintf(stdout, "1. read failed, buf=%s",buf);
      return -1;
   } 
   buf[result] = '\0';
   if (strncmp (buf, str, len)) {
      fprintf (stdout, "failed: read back \"%s\"\n", buf);
   } else {
      fprintf (stdout, "passed. string that was written is: %s\n", buf);
   }
   close(fd);
   return 0;
}
