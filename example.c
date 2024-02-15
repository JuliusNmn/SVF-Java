#include <stdio.h>
#include <stdlib.h>
typedef struct {
    int data;
    struct Node* next;
} Node;
void swap(char **p, char **q){
  char* t = *p; 
       *p = *q; 
       *q = t;
}
void print(char* a, char* b){
      printf("%c %c", *a, *b);
}

void* identity(void* x) {
      return x;
}
int returnConst5(){
      return 5;
}
Node* createNode(int val){
     Node* newNode = (Node*)malloc(sizeof(Node)); 
     newNode->data = val;
     return newNode;
}
Node* extremeIdentityFunction(Node* b) {
      Node a;
      a.next = &b;
      Node* x = identity(&a);
      Node* y = x->next;
      return y;

}
void printNodeVale(Node* n) {
      printf("%d\n", n->data);
}
int main(){
      char a1 = 'k';
      char b1 = 'l'; 
      char *a = &a1;
      char *b = &b1;
      swap(&a,&b);
      print(a, b);

      int constfive = returnConst5();
      Node* n = createNode(constfive);
      Node* nn = extremeIdentityFunction(n);
      printNodeValue(nn);
      return (int)a;
}
