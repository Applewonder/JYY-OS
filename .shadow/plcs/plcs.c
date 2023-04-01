#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "thread.h"
#include "thread-sync.h"

#define MAXN 10000
#define MAX_THREAD 20
#define START_COL 0
#define START_ROW 1
#define END_ROW 2
#define LOCK mutex_lock(&lk)
#define UNLOCK mutex_unlock(&lk)
int T, N, M;
char A[MAXN + 1], B[MAXN + 1];
int dp[MAXN][MAXN];
int is_dp_filled[MAXN][MAXN];
int thread_todo_list[MAXN + MAXN][MAX_THREAD][3];
int result;
mutex_t lk = MUTEX_INIT();
cond_t cv = COND_INIT();
#define DP(x, y) (((x) >= 0 && (y) >= 0) ? dp[x][y] : 0)
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
      dp[i][j] = MAX3(skip_a, skip_b, take_both);
    }
  }

  result = dp[N - 1][M - 1];
}

int is_cond_satisfied(int i, int j) {
  return (is_dp_filled[i - 1][j] && is_dp_filled[i][j - 1] && is_dp_filled[i - 1][j - 1]);
}

void Tworker_para(int id) {
  for (int round = 0; round < N + M - 1; round++) {
    int start_col = thread_todo_list[round][id][START_COL];
    int start_row = thread_todo_list[round][id][START_ROW];
    int end_row = thread_todo_list[round][id][END_ROW];
    assert(start_row <= end_row);
    if ((start_col == start_row) && (start_col == end_row) && start_col == 0 && id != 1) continue;
    int cur_pos = 0;
    while (cur_pos + start_row <= end_row) {
      int need_filled_x = start_row + cur_pos;
      int need_filled_y = start_col - cur_pos;
      while (!is_cond_satisfied(need_filled_x, need_filled_y)) {
        cond_wait(&cv, &lk);
      }
      LOCK;
      int skip_a = DP(need_filled_x - 1, need_filled_y);
      int skip_b = DP(need_filled_x, need_filled_y - 1);
      int take_both = DP(need_filled_x - 1, need_filled_y - 1) + (A[need_filled_x] == B[need_filled_y]);
      dp[need_filled_x][need_filled_y] = MAX3(skip_a, skip_b, take_both);
      is_dp_filled[need_filled_x][need_filled_y] = 1;
      UNLOCK;
      
      cur_pos ++;
    }
  }
}

int main(int argc, char *argv[]) {
  // No need to change
  assert(scanf("%s%s", A, B) == 2);
  N = strlen(A);
  M = strlen(B);
  T = !argv[1] ? 1 : atoi(argv[1]);

  // Add preprocessing code here
  for (int round = 0; round < N + M - 1; round++) {
    int diagonal_start_col = (round < M) ? round : M - 1;
    int diagonal_start_row = (round < M) ? 0 : round - M + 1;
    int diagonal_end_row = (round < N) ? round : N - 1;
    int diagonal_size = diagonal_end_row - diagonal_start_row + 1;

    int start_row = diagonal_start_row;
    int start_col = diagonal_start_col;

    int block_size = diagonal_size / T;
    int need_thread_num = T;
    if (diagonal_size < T) {
      need_thread_num = diagonal_size;
      block_size = 1;
    } else if (diagonal_size % T != 0) {
      block_size ++;
    }
    for (int i = 1; i <= need_thread_num; i++) {
        int end_row = start_row + block_size - 1;
        if (end_row > diagonal_end_row) {
            end_row = diagonal_end_row;
        }
        thread_todo_list[round][i][START_COL] = start_col;
        thread_todo_list[round][i][START_ROW] = start_row;
        thread_todo_list[round][i][END_ROW] = end_row;
        start_col = start_col - (end_row - start_row) - 1; 
        start_row = end_row + 1;
    }
  }

  for (int i = 0; i < T; i++) {
    create(Tworker_para);
  }
  join();  // Wait for all workers
  result = dp[N - 1][M - 1];
  printf("%d\n", result);
}
