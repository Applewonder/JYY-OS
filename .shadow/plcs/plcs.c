#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "thread.h"
#include "thread-sync.h"

#define MAXN 10000
int T, N, M;
typedef struct {
  int length;
  int if_fill_ed;
} DP_N;
char A[MAXN + 1], B[MAXN + 1];
DP_N dp[MAXN][MAXN];
int result;

typedef struct {
  int i, j;
} TASK_P;
typedef struct {
  TASK_P task_place;
  TASK_NODE* next;
} TASK_NODE;

TASK_NODE t_head[NTHREAD];

#define DP(x, y) (((x) >= 0 && (y) >= 0) ? dp[x][y].length : 0)
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MAX3(x, y, z) MAX(MAX(x, y), z)

void Tworker(int id) {
  if (id != 1) {
    // This is a serial implementation
    // Only one worker needs to be activated
    return;
  }

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < M; j++) {
      // Always try to make DP code more readable
      int skip_a = DP(i - 1, j);
      int skip_b = DP(i, j - 1);
      int take_both = DP(i - 1, j - 1) + (A[i] == B[j]);
      dp[i][j].length = MAX3(skip_a, skip_b, take_both);
    }
  }

  result = dp[N - 1][M - 1].length;
}

void add_cur_t(TASK_P* cur_t) {
  if (cur_t->j == 0 && cur_t->i > 0) {
    cur_t->i --;
  } else {
    cur_t->j ++;
  }
  return;
}

void add_task_node(int id, TASK_P* cur_t) {
  if (t_head[id] == NULL) {
    
  }
}

void build_task_list() {
  int t_sum = N + M + 1;
  int t_size = t_sum / T;
  if (t_sum % T) {
    t_size ++; 
  }
  TASK_P cur_t = {
    .i = N,
    .j = 0,
  };
  for (int i = 0; i < T; i++) {
    for (int j = 0; j < t_size; j++) {
      
      
      add_cur_t(&cur_t);
      if (cur_t.j >= M) {
        break;
      }

    }
    
  }
}

void Tworker_para(int id) {

}


int main(int argc, char *argv[]) {
  // No need to change
  assert(scanf("%s%s", A, B) == 2);
  N = strlen(A);
  M = strlen(B);
  T = !argv[1] ? 1 : atoi(argv[1]);

  // Add preprocessing code here
  for (int round = 0; round < N + M + 1; round++) {
    
  }

  for (int i = 0; i < T; i++) {
    create(Tworker);
  }
  join();  // Wait for all workers

  printf("%d\n", result);
}
