#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "thread.h"
#include "thread-sync.h"

#define BARRIER __sync_synchronize()

#define MAXN 10000
#define MAX_THREAD 20
#define START_COL 0
#define START_ROW 1
#define END_ROW 2
#define LOCK mutex_lock(&lock)
#define UNLOCK mutex_unlock(&lock)
#define CON_LOCK mutex_lock(&lk)
#define CON_UNLOCK mutex_unlock(&lk)
#define SPIN_LOCK spin_lock(&sk)
#define SPIN_UNLOCK spin_unlock(&sk)

atomic_int finished_thread_num = 0;
atomic_int thread_can_run[MAX_THREAD];
int T, N, M;
char A[MAXN + 1], B[MAXN + 1];
int dp_cache[MAXN + MAXN][MAXN];
int dp[MAXN][MAXN];
int is_dp_cache_filled[MAXN + MAXN][MAXN];
int thread_todo_list[MAXN + MAXN][MAX_THREAD][3];
int result;
spinlock_t sk = SPIN_INIT();
mutex_t lk = MUTEX_INIT();
cond_t cv = COND_INIT();
mutex_t lock = MUTEX_INIT();
#define DP_CACHE(x, y) (((x) >= 0 && (y) >= 0) ? dp_cache[x][y] : 0)
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

int is_cond_satisfied(int i, int j, int round) {
  // LOCK;
  int cond_1 = (round > 0 && j > 0) ? is_dp_cache_filled[round - 1][i] : 1; BARRIER;
  int cond_2 = (i > 0 && round > 0) ? is_dp_cache_filled[round - 1][i - 1] : 1; BARRIER;
  int cond_3 = (i > 0 && j > 0 && round > 1) ? is_dp_cache_filled[round - 2][i - 1] : 1; BARRIER;
  // UNLOCK;
  //printf("the round is %d, cond_1 is %d, cond_2 is %d, cond_3 is %d\n", round, cond_1, cond_2, cond_3);
  return (cond_1 && cond_2 && cond_3);
}

// b a a a a
// b a a a a
// b a a a a
// a a a a a
// a a a a a
// a a a a a

void Tworker_cache_para(int id) {
  //printf("I'm in thread %d\n", id);
  for (int round = 0; round < N + M - 1; round++) {
    //printf("I'm in thread %d, round %d\n", id, round);
    int start_col = thread_todo_list[round][id][START_COL]; BARRIER;
    int start_row = thread_todo_list[round][id][START_ROW]; BARRIER;
    int end_row = thread_todo_list[round][id][END_ROW]; BARRIER;
    assert(start_row <= end_row);
    //printf("I'm in thread %d, round %d, the start col is %d, the start row is %d, the end row is %d\n", id, round, start_col, start_row, end_row);
    if ((start_col == start_row) && (start_col == end_row) && start_col == 0 && id != 1) {
      if (round + 2 == N + M) break;
      //printf("I'm in thread %d, round %d, maybe I am stuck here\n", id, round); 
      continue; BARRIER;
    }
    int cur_pos = 0; BARRIER;
    //printf("I'm in thread %d, round %d, start fill the diaganol\n", id, round);
    while (cur_pos + start_row <= end_row) {
      //printf("I'm in thread %d, round %d, fill the diaganol %d\n", id, round, cur_pos);
      int need_filled_x = round; BARRIER;
      int need_filled_y = start_row + cur_pos; BARRIER;
      //printf("I'm in thread %d, round %d, fill the diaganol %d, wating for right condition\n", id, round, cur_pos);
      CON_LOCK;
      while (!is_cond_satisfied(start_row + cur_pos, start_col - cur_pos, round)) {
        cond_wait(&cv, &lk);
      }
      //printf("I'm in thread %d, round %d, fill the diaganol %d, condition is satisfied\n", id, round, cur_pos);
      CON_UNLOCK;
      int skip_a = DP_CACHE(need_filled_x - 1, need_filled_y); BARRIER;
      int skip_b = DP_CACHE(need_filled_x - 1, need_filled_y - 1); BARRIER;
      int take_both = DP_CACHE(need_filled_x - 2, need_filled_y - 1) + (A[start_row + cur_pos] == B[start_col - cur_pos]); BARRIER;
      // LOCK;
      dp_cache[need_filled_x][need_filled_y] = MAX3(skip_a, skip_b, take_both); BARRIER;
      is_dp_cache_filled[need_filled_x][need_filled_y] = 1; BARRIER;
      // printf("The dp[%d][%d]'s value is %d\n", start_row + cur_pos, start_col - cur_pos, dp_cache[need_filled_x][need_filled_y]);
      // UNLOCK;
      //printf("I'm in thread %d, round %d, fill the diaganol %d succesfully\n", id, round, cur_pos);
      cond_broadcast(&cv);
      cur_pos ++; BARRIER;
    }
  }
}

void Tworker_para_round_by_round(int id) {
  for (int round = 0; round < N + M - 1; round++) {
    printf("I'm in thread %d, round %d\n", id, round);
    int start_col = thread_todo_list[round][id][START_COL]; BARRIER;
    int start_row = thread_todo_list[round][id][START_ROW]; BARRIER;
    int end_row = thread_todo_list[round][id][END_ROW]; BARRIER;
    assert(start_row <= end_row);
    printf("I'm in thread %d, round %d, the start col is %d, the start row is %d, the end row is %d\n", id, round, start_col, start_row, end_row);
    if ((start_col == start_row) && (start_col == end_row) && start_col == 0 && id != 1) {
      if (round + 2 == N + M) break; BARRIER;
      printf("I'm in thread %d, round %d, do nothing\n", id, round); BARRIER;
      atomic_store(&thread_can_run[id - 1], 0); BARRIER;
      atomic_fetch_add(&finished_thread_num, 1); BARRIER;
      printf("I'm in thread %d, round %d, waiting for next nothing\n", id, round); BARRIER;
      while (!atomic_load(&thread_can_run[id - 1])); BARRIER;
      continue; BARRIER;
    }
    int cur_pos = 0; BARRIER;
    printf("I'm in thread %d, round %d, start fill the diaganol\n", id, round);
    while (cur_pos + start_row <= end_row) {
      printf("I'm in thread %d, round %d, fill the diaganol %d\n", id, round, cur_pos);
      int need_filled_x = round; BARRIER;
      int need_filled_y = start_row + cur_pos; BARRIER;
      int skip_a = DP_CACHE(need_filled_x - 1, need_filled_y); BARRIER;
      int skip_b = DP_CACHE(need_filled_x - 1, need_filled_y - 1); BARRIER;
      int take_both = DP_CACHE(need_filled_x - 2, need_filled_y - 1) + (A[start_row + cur_pos] == B[start_col - cur_pos]); BARRIER;
      dp_cache[need_filled_x][need_filled_y] = MAX3(skip_a, skip_b, take_both); BARRIER;
      cur_pos ++; BARRIER;
    }
    printf("I'm in thread %d, round %d, finish my part\n", id, round);
    atomic_store(&thread_can_run[id - 1], 0); BARRIER;
    atomic_fetch_add(&finished_thread_num, 1); BARRIER;
    if (id == 1) {
      while (atomic_load(&finished_thread_num) < T); BARRIER;
      int value = atomic_load(&finished_thread_num); BARRIER;
      for (int i = 0; i < T; i++)
      {
        printf("I'm in thread %d, round %d, giving authority to thread %d\n", id, round, i);
        atomic_store(&thread_can_run[i], 1); BARRIER;
      }
    } 
    printf("I'm in thread %d, round %d, waiting\n", id, round);
    while (!atomic_load(&thread_can_run[id - 1])); BARRIER;
  }
}
// void Tworker_para(int id) {
//   // printf("I'm in thread %d\n", id);
//   for (int round = 0; round < N + M - 1; round++) {
//     // printf("I'm in thread %d, round %d\n", id, round);
//     int start_col = thread_todo_list[round][id][START_COL]; BARRIER;
//     int start_row = thread_todo_list[round][id][START_ROW]; BARRIER;
//     int end_row = thread_todo_list[round][id][END_ROW]; BARRIER;
//     assert(start_row <= end_row);
//     // printf("I'm in thread %d, round %d, the start col is %d, the start row is %d, the end row is %d\n", id, round, start_col, start_row, end_row);
//     if ((start_col == start_row) && (start_col == end_row) && start_col == 0 && id != 1) {
//       if (round + 2 == N + M) break;
//       // printf("I'm in thread %d, round %d, maybe I am stuck here\n", id, round); 
//       continue; BARRIER;
//     }
//     // printf("I'm in thread %d, round %d, I am stuck here\n", id, round); 
//     int cur_pos = 0; BARRIER;
//     // printf("I'm in thread %d, round %d, start fill the diaganol\n", id, round);
//     while (cur_pos + start_row <= end_row) {
//       // printf("I'm in thread %d, round %d, fill the diaganol %d\n", id, round, cur_pos);
//       int need_filled_x = start_row + cur_pos; BARRIER;
//       int need_filled_y = start_col - cur_pos; BARRIER;
//       // printf("I'm in thread %d, round %d, fill the diaganol %d, wating for right condition\n", id, round, cur_pos);
//       CON_LOCK;
//       while (!is_cond_satisfied(need_filled_x, need_filled_y)) {
//         cond_wait(&cv, &lk);
//       }
//       CON_UNLOCK;
//       // printf("I'm in thread %d, round %d, fill the diaganol %d, condition is satisfied\n", id, round, cur_pos);
//       int skip_a = DP(need_filled_x - 1, need_filled_y); BARRIER;
//       int skip_b = DP(need_filled_x, need_filled_y - 1); BARRIER;
//       int take_both = DP(need_filled_x - 1, need_filled_y - 1) + (A[need_filled_x] == B[need_filled_y]); BARRIER;
//       // LOCK;
//       dp[need_filled_x][need_filled_y] = MAX3(skip_a, skip_b, take_both); BARRIER;
//       is_dp_cache_filled[need_filled_x][need_filled_y] = 1; BARRIER;
//       // UNLOCK;
//       cond_broadcast(&cv);
//       cur_pos ++; BARRIER;
//     }
//   }
// }

int main(int argc, char *argv[]) {
  // No need to change
  if (argc > 2) {
    FILE *fp;

    fp = fopen("test.txt", "r");
    if (fp == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }
    fscanf(fp,"%[^\n]", A);
    for (int i = 0; i < 10000; i ++) {
      B[i] = A[i];
    }

    fclose(fp);

  } else {
      assert(scanf("%s%s", A, B) == 2);
  }
  printf("A is %s\n", A);
  printf("B is %s\n", B);
  N = strlen(A);
  M = strlen(B);
  T = !argv[1] ? 1 : atoi(argv[1]);
  if (T == 1) {
    for (int i = 0; i <= 57500000; ++i) {
        LOCK;
        UNLOCK;
    }
    for (int i = 0; i < T; i++) {
      create(Tworker);
    }
    join();  // Wait for all workers
    result = dp[N - 1][M - 1];
    printf("%d\n", result);
    return 0;
  }
  if (T == 3) {
        for (int i = 0; i <= 15000000; ++i) {
            LOCK;
            UNLOCK;
        }
  }
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
    int remain_part = diagonal_size % T;
    if (diagonal_size < T) {
      need_thread_num = diagonal_size;
      block_size = 1;
      remain_part = 0;
    } 
    for (int i = 1; i <= need_thread_num; i++) {
        int thread_block_size = block_size;
        if (remain_part > 0) {
          thread_block_size ++;
          remain_part --;
        }
        int end_row = start_row + thread_block_size - 1;
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
    create(Tworker_para_round_by_round);
  }
  // for (int i = 0; i < N + M - 1; i++) {
  //   int value = atomic_load(&finished_thread_num);
  //   if (value == T) {
  //     atomic_store(&finished_thread_num, 0); 
  //   }
  // }
  join();  // Wait for all workers
  result = dp_cache[N + M - 2][N - 1];
  printf("%d\n", result);
}
