#include "common.h"
#include "am.h"
#include "testcase.h"
#include "string.h"

#define MAX_LINE_LENGTH 40
#define MAX_LENGTH 20
#define MAX_ALLOC_SIZE 100

extern FILE *file;
char line[MAX_LINE_LENGTH];
char destinationArray[MAX_LENGTH];

int cur_alloc_index = 0;

typedef struct Task_ TASK;
struct Task_{
  void *alloc;
  unsigned long size;
  int line_num;
  TASK* next;
};

TASK* head = NULL;

void print_chain() {
  printf("Printing chain\n");
  TASK* cur = head;
  while (cur != NULL) {
    printf("alloc: %p, size: %ld\n", cur->alloc, cur->size);
    cur = cur->next;
  }
}

int find_the_target_index(char* line) {
  int index = 0;
  while (line[index] != ',') {
    index++;
  }
  return index;
}

void insert_alloc_chain(void* alloc, unsigned long size, int line_num) {
  // printf("Inserting alloc block\n");
  TASK* new_task = (TASK*)malloc(sizeof(TASK));
  new_task->alloc = alloc;
  new_task->size = size;
  new_task->line_num = line_num;

  new_task->next = head;
  head = new_task;
}

bool delete_alloc_block(void* alloc) {
  // printf("Deleting alloc block\n");
  // print_chain();
  TASK* cur = head;
  TASK* pre = NULL;
  while (cur != NULL) {
    if (cur->alloc == alloc) {
      if (pre == NULL) {
        head = cur->next;
      } else {
        pre->next = cur->next;
      }
      return true;
    }
    pre = cur;
    cur = cur->next;
  }
  return false;
}

bool judge_if_have_duplicate_alloc(unsigned long start_alloc, unsigned long end_alloc) {
  TASK* cur = head;
  while (cur != NULL) {
    if ((unsigned long)cur->alloc >= end_alloc || (unsigned long)(cur->alloc + cur->size) <= start_alloc) {
      cur = cur->next;
      continue;
    } else {
      printf("\033[31m Error: Duplicate alloc: %p, size: %ld\n\033[0m", cur->alloc, cur->size);
      printf("\033[31m Error: Duplicate alloc: %p, size: %ld\n\033[0m", (void*)start_alloc, end_alloc - start_alloc);

      return true;
    }
  }
  return false;
}

bool judge_if_has_bad_free(unsigned long start_free) {
  return !delete_alloc_block((void*)start_free);
}

void judger_for_alloc_and_free(int test_id) {
    int line_num = 0;

    char str[20];
    sprintf(str, "%d", test_id);
    char origin_log[200] = "/home/appletree/JYY-OS/kernel/test/testlog";
    strcat(origin_log, str);
    strcat(origin_log, ".txt");

    file = fopen(origin_log, "r");
    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] == 'A') {
            int start_index = 6;
            int end_index = find_the_target_index(line);
            int length = end_index - start_index;
            memcpy(destinationArray, &line[start_index], length);
            destinationArray[length] = '\0'; 
            unsigned long alloc_address = strtoul(destinationArray, NULL, 16);
            int size_start_index = end_index + 7;
            strcpy(destinationArray, &line[size_start_index]);
            unsigned long alloc_size = strtoul(destinationArray, NULL, 10);
            if (judge_if_have_duplicate_alloc(alloc_address, alloc_address + alloc_size)) {
              printf("\033[31m Error: Duplicate alloc: %p\n\033[0m", (void*)alloc_address);
              exit(1);
            }
            insert_alloc_chain((void*)alloc_address, alloc_size, line_num);
            // print_chain();
        } else if (line[0] == 'F'){
            strcpy(destinationArray, &line[5]);
            unsigned long free_address = strtoul(destinationArray, NULL, 16);
            if (judge_if_has_bad_free(free_address)) {
              printf("\033[31m Error: Bad free in line %d\n\033[0m", line_num);
              // print_chain();
              exit(1);
            }
        }
        line_num++;
    }
    fclose(file);
}

int main(int argc, char *argv[]) {
  srand((unsigned)time(NULL));
  if (argc < 2) exit(1);
  switch(atoi(argv[1])) {
    case 0: {
      do_test_0();
      judger_for_alloc_and_free(0);
      printf("\033[32m Test 0 passed\n\033[0m");
      break;
    }
    case 1: {
      do_test_1();
      judger_for_alloc_and_free(1);
      printf("\033[32m Test 1 passed\n\033[0m");
      break;
    }
    case 2: {
      do_test_2();
      judger_for_alloc_and_free(2);
      printf("\033[32m Test 2 passed\n\033[0m");
      break;
    }
    case 3: {
      do_test_3();
      judger_for_alloc_and_free(3);
      printf("\033[32m Test 3 passed\n\033[0m");
      break;
    }
  }
}