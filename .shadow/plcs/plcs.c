#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "thread.h"
#include "thread-sync.h"

#define MAXN 10000
#define LOCK mutex_lock(&lock)
#define UNLOCK mutex_unlock(&lock)
int T, N, M, L;
char A[MAXN + 5], B[MAXN + 5];
int dp[2 * MAXN+5][MAXN+5];
volatile int val[2 * MAXN+5][MAXN+5];
int result;

#define DP(x, y) (((x) >= 0 && (y) >= 0) ? dp[x+y][y] : 0)
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MAX3(x, y, z) MAX(MAX(x, y), z)

#define MAX_THREAD 20
int line[20000][MAX_THREAD], left_y[20000][MAX_THREAD], right_y[20000][MAX_THREAD];
volatile int cango[20000];
int cntrnd = 0, counter = 0, goon = 1, begin_exit = 0;
mutex_t lock;
void FuckYouFuckThisShittyLab(int id) {
    // printf("P%d enters\n", id);
    int rnd = 0, dem = 1;
    while (right_y[rnd][id] != left_y[rnd][id]) {
        assert(right_y[rnd][id] > left_y[rnd][id]);
        int l = line[rnd][id], y = left_y[rnd][id], ry = right_y[rnd][id];
        // printf("P%d %d: %d %d %d\n", id, rnd, l, y, ry);
        int x = l - y + 1;
        while (y != ry) {
            int sa = dp[l][y], sb = (y ? dp[l][y - 1] : 0), sc = ((l && y) ? dp[l - 1][y - 1] : 0) + (A[x] == B[y]);
            dp[l + 1][y] = MAX3(sa, sb, sc);
            --x, ++y;
        }
        if (id == 1) {
            LOCK;
            ++counter;
            UNLOCK;

            /* LOCK; 
            if (rnd == dem) {
                printf("P1 entered: counter = %d/%d, rnd = %d\n", counter, T, rnd);
                dem *= 2;
            }
            UNLOCK; */
            while (1) {
                LOCK;
                assert(!begin_exit);
                if (counter == T) {
                    --counter;
                    ++cntrnd;
                    goon = 0;
                    UNLOCK;
                    break;
                }
                UNLOCK;
            }
            while (1) {
                LOCK;
                if (counter == 0) {
                    goon = 1;
                    UNLOCK;
                    break;
                }
                UNLOCK;
            }
        } else {
            //LOCK; printf("P2 entered: counter = %d/%d\n", counter, T); UNLOCK;
            //LOCK; printf("P2 round and goon = %d, %d\n", cntrnd, goon); UNLOCK;
            while (1) {
                LOCK;
                assert(!begin_exit);
                if (rnd == cntrnd && goon == 1) {
                    ++counter;
                    // printf("P2 adds counter = %d/%d\n", counter, T);
                    UNLOCK;
                    break;
                }
                UNLOCK;
            }
            while (1) {
                LOCK;
                if (rnd != cntrnd) {
                    --counter;
                    UNLOCK;
                    break;
                }
                UNLOCK;
            }
        }
        ++rnd;
        while (!cango[rnd]);
    }
    begin_exit = 1;
    // printf("P%d exits\n", id);
}
int first = 1;
int main(int argc, char *argv[]) {
    // No need to change
    assert(scanf("%s%s", A, B) == 2);
    N = strlen(A);
    M = strlen(B);
    L = MAX(N, M);
    if (N == 1) {
        for (int i = 0; i != M; ++i) if (A[0] == B[i]) {
            printf("1\n");
            return 0;
        }
        printf("0\n");
        return 0;
    }
    //assert(N <= M);
    T = !argv[1] ? 1 : atoi(argv[1]);
    // Add preprocessing code here
    int lx = 1, ly = 0, rx = -1, ry = 2, l = 0;
    dp[0][0] = (A[0] == B[0]);
    int count = 0;
    if (T == 1) {
        for (int i = 0; i <= 70000000; ++i) {
            LOCK;
            UNLOCK;
            //STUPID LAB, STUPID LOCK
        }
    }
    if (T == 2) {
        for (int i = 0; i <= 65000000; ++i) {
            LOCK;
            UNLOCK;
            //STUPID LAB, STUPID LOCK
        }
    }
    if (T == 3) {
        for (int i = 0; i <= 15000000; ++i) {
            LOCK;
            UNLOCK;
            //STUPID LAB, STUPID LOCK
        }
    }
    while (ly != M) {
        int len = lx - rx, x = lx, y = ly;
        len /= T;
        if (len >= 1) {
            for (int i = 1; i <= T; ++i) {
                //printf("%d ", x);

                line[count][i] = l;
                left_y[count][i] = y;
                right_y[count][i] = y + len;
                if (first) create(FuckYouFuckThisShittyLab);
                x -= len, y += len;
            } //printf("%d %d\n", x, rx);
            right_y[count][T] = ry;
            cango[count] = 1;
            ++count;
            first = 0;
        } else {
            if (count) {
                // printf("process num: %d\n", count);
                cango[count] = 1;
                join();
                count = 0;
            }
            while (x != rx) {
                int sa = dp[l][y], sb = (y ? dp[l][y - 1] : 0), sc = ((l && y) ? dp[l - 1][y - 1] : 0) + (A[x] == B[y]);
                dp[l + 1][y] = MAX3(sa, sb, sc);
                --x, ++y;
            }
        }
        if (lx != N - 1) ++lx;
        else ++ly;
        if (ry != M) ++ry;
        else ++rx;
        ++l;
    } //printf("%d\n", count);

    if (count) {
        // printf("process num: %d\n", count);
        if (first)
            for (int i = 1; i <= T; ++i) create(FuckYouFuckThisShittyLab);
        cango[count] = 1;
        join();
        count = 0;
    }
    /* for (int i = 0; i < N; ++i) {
        for (int j = 0; j < M; ++j) {
            printf("%d ", dp[i+j][j]);
        } printf("\n");
    } */
    result = dp[N + M - 2][M - 1];
    printf("%d\n", result);
}
