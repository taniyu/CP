#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

#define SIZE 100000000
#define MAX  100000000
#define LENGTH 10
#define tvaltof(ts, tus) ((ts) + (double)(tus)/1000000)

typedef struct {
  int left;
  int right;
  int depth;
} Range;

void quick_sort(Range range);
void _qsort(Range range);
int partition(Range range);
void swap(int i, int j);
void print_arr();
void init_arr();
void insertion_sort(Range range);

int x[SIZE];

int main(void)
{
  Range range;
  struct timeval start, end;
  range.left = 0;
  range.right = SIZE;
  range.depth = 0;
  srand((unsigned) time(NULL));
  init_arr();
  /* print_arr(); */
  gettimeofday(&start, NULL);
  quick_sort(range);
  gettimeofday(&end, NULL);
  /* print_arr(); */
  printf("%f\n", (double)tvaltof(end.tv_sec, end.tv_usec) - tvaltof(start.tv_sec, start.tv_usec));
  return 0;
}

void quick_sort(Range range)
{
  range.right -= 1;
  _qsort(range);
}

void _qsort(Range range)
{
  int v;
  Range tmp = range;

  /* if ( (range.right - range.left) <= LENGTH ) { */
  /*   insertion_sort(tmp); */
  /*   return; */
  /* } */
  if (range.left >= range.right) { return; }
  v = partition(range);
  tmp.right = v-1;
  _qsort(tmp);
  tmp.right = range.right;
  tmp.left = v + 1;
  _qsort(tmp);
}

int partition(Range range)
{
  int i, j;
  int t, pivot;
  i = range.left - 1;
  j = range.right;
  pivot = x[range.right];
  for (;;) {
    while (x[++i] < pivot);
    while (i < --j && pivot < x[j]);
    if (i >= j) { break; }
    swap(i, j);
    /* print_arr(); */
  }
  /* print_arr(); */
  swap(i, range.right);
  return i;
}


void swap(int i, int j)
{
  int tmp;
  tmp = x[i];
  x[i] = x[j];
  x[j] = tmp;
}

void print_arr()
{
  int i;
  for ( i = 0; i < SIZE; i++ ) {
    printf("%d ", x[i]);
  }
  puts("");
}

void init_arr()
{
  int i;
  for ( i = 0; i < SIZE; i++ ) {
    x[i] = rand() / (RAND_MAX + 1.0) * MAX;
  }
}

void insertion_sort(Range range)
{
  int i, j;
  range.left += 1;
  range.right += 1;
  for (i = range.left; i < range.right; i++) {
    j = i;
    while (j >= range.left && x[j-1] > x[j]) {
      swap(j - 1, j);
      j--;
    }
  }
}

