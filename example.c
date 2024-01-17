void swap(char **p, char **q){
  char* t = *p; 
       *p = *q; 
       *q = t;
}
void print(char* a, char* b){
      printf("%c %c", *a, *b);
}
int main(){
      char a1 = 'k';
      char b1 = 'l'; 
      char *a = &a1;
      char *b = &b1;
      swap(&a,&b);
      print(a, b);
      return (int)a;
}
