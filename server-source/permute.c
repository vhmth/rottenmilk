#include <stdio.h>

int
print_string(char *s) {
  printf("%c%c%c%c%c%c\n", s[0], s[1], s[2], s[3], s[4], s[5]);
  return 0;
}

void 
permute(char *orig_word, char *output_word, int bitmask, int level) {
  for (int i = 0 ; i < 6 ; ++ i) {
	 if (bitmask & (1 << i)) {
		continue;  // already used this character
	 }
	 char letter = orig_word[i];
	 output_word[level] = letter;
	 if (level == 5) {
		print_string(output_word);
	 } else {
		permute(orig_word, output_word, bitmask | (1<<i), level + 1);
	 }
  }
}  

int
main() {
  char str[] = "abcdef";
  char out_str[] = "";
  permute(str, out_str, 0, 0);
}
