#include "test3_generated.h"

int main() {
  int **arr = new int*[10];
  for (int i = 0; i < 10; i++) arr[i] = new int[20];
  staged(arr);
  for (int i = 0; i < 10; i++) delete[] arr[i];
  delete[] arr;
}
