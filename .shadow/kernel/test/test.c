#include "common.h"
#include "am.h"
#include "testcase.h"
#include "string.h"

#define MAX_LINE_LENGTH 30
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

void insert_alloc_chain(void* alloc, unsigned long size) {
  printf("Inserting alloc block\n");
  TASK* new_task = (TASK*)malloc(sizeof(TASK));
  new_task->alloc = alloc;
  new_task->size = size;
  new_task->next = NULL;

  new_task->next = head;
  head = new_task;
}

bool delete_alloc_block(void* alloc) {
  printf("Deleting alloc block\n");
  print_chain();
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
      return true;
    }
  }
  return false;
}

bool judge_if_has_bad_free(unsigned long start_free) {
  return !delete_alloc_block((void*)start_free);
}

void judger_for_test_0() {
    int line_num = 0;
    printf("Judger for test_0\n");
    file = fopen("/home/appletree/JYY-OS/kernel/test/testlog.txt", "r");
    while (!feof(file)) {
        fgets(line, MAX_LINE_LENGTH, file);
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
              printf("Error: Duplicate alloc\n");
              exit(1);
            }
            insert_alloc_chain((void*)alloc_address, alloc_size);
            print_chain();
        } else {
            strcpy(destinationArray, &line[5]);
            unsigned long free_address = strtoul(destinationArray, NULL, 16);
            if (judge_if_has_bad_free(free_address)) {
              printf("Error: Bad free in line %d\n", line_num);
              print_chain();
              exit(1);
            }
        }
        fgets(line, MAX_LINE_LENGTH, file);//read "\n"
        line_num++;
    }
    fclose(file);
    printf("Judger for test_0 end\n");
}

int main(int argc, char *argv[]) {
  
  if (argc < 2) exit(1);
  switch(atoi(argv[1])) {
    case 0: {
      do_test_0();
      judger_for_test_0();
      break;
    }
  }
}