#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
// #include <locale.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef struct fat32hdr Fat32Hdr;
typedef struct BITMAPFILEHEADER BMFileHdr;
typedef struct BITMAPINFOHEADER BMInfoHdr;
typedef struct fat32dent Fat32Dent;
typedef struct longentry LFN_ENTRY;
typedef enum cluster_type Clu_Type;
typedef struct dir_node Dir_Node;

// Copied from the manual
struct fat32hdr {
  u8  BS_jmpBoot[3];
  u8  BS_OEMName[8];
  u16 BPB_BytsPerSec;
  u8  BPB_SecPerClus;
  u16 BPB_RsvdSecCnt;
  u8  BPB_NumFATs;
  u16 BPB_RootEntCnt;
  u16 BPB_TotSec16;
  u8  BPB_Media;
  u16 BPB_FATSz16;
  u16 BPB_SecPerTrk;
  u16 BPB_NumHeads;
  u32 BPB_HiddSec;
  u32 BPB_TotSec32;
  u32 BPB_FATSz32;
  u16 BPB_ExtFlags;
  u16 BPB_FSVer;
  u32 BPB_RootClus;
  u16 BPB_FSInfo;
  u16 BPB_BkBootSec;
  u8  BPB_Reserved[12];
  u8  BS_DrvNum;
  u8  BS_Reserved1;
  u8  BS_BootSig;
  u32 BS_VolID;
  u8  BS_VolLab[11];
  u8  BS_FilSysType[8];
  u8  __padding_1[420];
  u16 Signature_word;
} __attribute__((packed));

struct fat32dent {
  u8  DIR_Name[11];
  u8  DIR_Attr;
  u8  DIR_NTRes;
  u8  DIR_CrtTimeTenth;
  u16 DIR_CrtTime;
  u16 DIR_CrtDate;
  u16 DIR_LastAccDate;
  u16 DIR_FstClusHI;
  u16 DIR_WrtTime;
  u16 DIR_WrtDate;
  u16 DIR_FstClusLO;
  u32 DIR_FileSize;
} __attribute__((packed));

struct longentry{
  u8  ord;             
  u16 name1[5];        
  u8  attr;            
  u8  type;            
  u8  chksum;          
  u16 name2[6];        
  u16 first_cluster;   
  u16 name3[2];        
} __attribute__((packed));

struct dir_node {
  u32 clu_num;
  Dir_Node* nxt;
};

struct BITMAPFILEHEADER {
  u16 type;             // Magic identifier, must be 'BM'
  u32 size;             // File size in bytes
  u16 reserved1;        // Not used
  u16 reserved2;        // Not used
  u32 offset;           // Offset to image data in bytes from beginning of file
} __attribute__((packed));

struct BITMAPINFOHEADER {
  u32 size;             // Header size in bytes
  u32 width;            // Width of the image
  u32 height;           // Height of the image
  u16 planes;           // Number of color planes
  u16 bits;             // Bits per pixel
  u32 compression;      // Compression type
  u32 imagesize;        // Image size in bytes
  u32 xresolution;      // Pixels per meter
  u32 yresolution;      // Pixels per meter
  u32 ncolours;         // Number of colors
  u32 importantcolours; // Important colors
} __attribute__((packed));

enum cluster_type {
  DIR,
  BMP_F,
  BMP_I,
  UNUSED
};

Fat32Hdr *hdr;
Dir_Node* Dir_Begin;

void *map_disk(const char *fname);
void* Cluster_to_Addr(u32 n);
// u32 Addr_to_Cluster(void* addr);
bool judge_if_dir(void* cluster);
bool judge_if_unused(void* cluster);
bool judge_if_bmp_hdr(void* cluster);
Clu_Type decide_clu_type(void* cluster);
void initialize_dir_begin();
void insert_dir_node(u32 clu_num);
void classify_the_cluster(u32 clu_cnt, Clu_Type* clu_table);
void recover_the_dir(void* cluster, Clu_Type* clu_table);
void get_long_fill_name(LFN_ENTRY* entry, u32 entry_cnt, char* file_name);
bool get_pic_sha_num_and_print(u32 clu_num, Clu_Type* clu_table, char* file_name);
bool store_pic_in_tmp(void* start, u32 fsize, char* file_name);
char* build_file_name_with_tmp(char* file_name);
bool calculate_sha1sum(char* file_name);
void get_short_fill_name(Fat32Dent* entry, char* file_name);
int recover_short_name_file(void* short_name_entry, Clu_Type* clu_table);
int recover_long_name_file(void* long_name_entry, Clu_Type* clu_table, u32 remain_dir);
void recovery(Clu_Type* clu_table);


int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s fs-image\n", argv[0]);
    exit(1);
  }

  setbuf(stdout, NULL);

  assert(sizeof(struct fat32hdr) == 512); // defensive

  // map disk image to memory
  hdr = map_disk(argv[1]);
  initialize_dir_begin();

  u32 RootDirSectors = ((hdr->BPB_RootEntCnt * 32) + (hdr->BPB_BytsPerSec - 1)) / hdr->BPB_BytsPerSec;

  u32 DataSec_cnt = hdr->BPB_TotSec32 - (hdr->BPB_RsvdSecCnt + (hdr->BPB_NumFATs * hdr->BPB_FATSz32) + RootDirSectors);
  u32 clu_cnt = DataSec_cnt / hdr->BPB_SecPerClus; 
  Clu_Type* clu_table = malloc(clu_cnt * sizeof(Clu_Type));

  classify_the_cluster(clu_cnt, clu_table);

  recovery(clu_table);
  // file system traversal
  munmap(hdr, hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec);
}

void recover_the_dir(void* cluster, Clu_Type* clu_table) {
  u32 clu_size = hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus;
  for (u32 i = 0; i < clu_size; i += 32) {
    Fat32Dent *entry = (Fat32Dent *)(cluster + i);
    if (entry->DIR_Name[0] == 0x00) {
        break;
    } else if (entry->DIR_Name[0] == 0xE5) {
        continue;
    } else if (entry->DIR_Attr == 0x0F) {
        i += recover_long_name_file(cluster, clu_table, (clu_size - i * 32));
    } else {
        i += recover_short_name_file(cluster, clu_table);
    }
  }
}

void get_long_fill_name(LFN_ENTRY* entry, u32 entry_cnt, char* file_name) {
  LFN_ENTRY* cur_dir = entry + entry_cnt - 1;
  int i = 0;
  int k = 0;
  for (int j = entry_cnt - 1; j >= 0; j--) {
    //TODO: judge if valid
    for (i = 0; i < 5; i++)
        file_name[k++] = (char)cur_dir->name1[i];
    for (i = 0; i < 6; i++)
        file_name[k++] = (char)cur_dir->name2[i];
    for (i = 0; i < 2; i++)
        file_name[k++] = (char)cur_dir->name3[i]; 
    cur_dir = cur_dir - 1;
  }
  file_name[k] = '\0';
}

bool store_pic_in_tmp(void* start, u32 fsize, char* file_name) {
  FILE* file = fopen(file_name, "wb");
  if (file == NULL) {
      perror("Failed to open file");
      return false;
  }

  size_t written = fwrite(start, 1, fsize, file);
  if (written != fsize) {
      perror("Failed to write data");
  }

  fclose(file);
  return true;
}

char* build_file_name_with_tmp(char* file_name) {
  char* file_name_head = malloc(sizeof(char) * 300);
  strcpy(file_name_head, "/tmp/");
  strcat(file_name_head, file_name);
  return file_name_head;
}

bool calculate_sha1sum(char* file_name) {
  char command[256];
  // snprintf(command, sizeof(command), "sha1sum %s", file_name);

  FILE* pipe = popen(command, "r");
  if (pipe == NULL) {
      perror("Failed to run command");
      return false;
  }

  char buffer[128];
  while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
      printf("%s", buffer);
  }

  pclose(pipe);
  return true;
}

bool get_pic_sha_num_and_print(u32 clu_num, Clu_Type* clu_table, char* file_name) {
  if (clu_table[clu_num] != BMP_F) {
    return false;
  }
  void* cluster = Cluster_to_Addr(clu_num);
  BMFileHdr* bfhdr = cluster;
  u32 fsize = bfhdr->size;
  char* file_system_file_name = NULL;
  file_system_file_name = build_file_name_with_tmp(file_name);
  bool is_success = store_pic_in_tmp(cluster, fsize, file_system_file_name);
  if (!is_success) {
    return is_success;
  }
  is_success = calculate_sha1sum(file_system_file_name);
    if (!is_success) {
    return is_success;
  }
  printf(" %s\n", file_name);
  return true;
}

void get_short_fill_name(Fat32Dent* entry, char* file_name) {
  for (int i = 0; i < 8; i ++) {
    file_name[i] = (char)entry->DIR_Name[i];
  }
  file_name[8] = '.';
  for (int i = 8; i < 11; i ++) {
    file_name[i + 1] = (char)entry->DIR_Name[i];
  }
}

int recover_short_name_file(void* short_name_entry, Clu_Type* clu_table) {
  Fat32Dent* entry = short_name_entry;
  char* file_name = malloc(sizeof(char) * 20);
  get_short_fill_name(entry, file_name);
  u32 pic_clu_num = (((u32)entry->DIR_FstClusHI) << 16) | entry->DIR_FstClusLO;
  bool is_success = get_pic_sha_num_and_print(pic_clu_num, clu_table, file_name);
  return 0;
}

int recover_long_name_file(void* long_name_entry, Clu_Type* clu_table, u32 remain_dir) {
  LFN_ENTRY* entry = long_name_entry;
  char* file_name = malloc(sizeof(char) * 256);
  if (entry->ord & 0x40) {
    u32 entry_cnt = entry->ord & 0x0f;
    if (remain_dir < (entry_cnt + 1) * 32) {//TODO: use the checksum to get the SFN
      return remain_dir;
    }
    get_long_fill_name(entry, entry_cnt, file_name);
    Fat32Dent* SFN = long_name_entry + 32 * entry_cnt;
    u32 pic_clu_num = SFN->DIR_FstClusHI << 16 | SFN->DIR_FstClusLO;
    bool is_success = get_pic_sha_num_and_print(pic_clu_num, clu_table, file_name);
    if (is_success) {
      return entry_cnt * 32;
    }
  }
  return 0;
}

void recovery(Clu_Type* clu_table) {
  for (Dir_Node* i = Dir_Begin; i != NULL; i = i->nxt)
  {
    u32 clu_num = i->clu_num;
    void* cluster = Cluster_to_Addr(clu_num);
    recover_the_dir(cluster, clu_table);
  }
}

void classify_the_cluster(u32 clu_cnt, Clu_Type* clu_table) {
  for (u32 clu_num = 2; clu_num < clu_cnt; clu_num++)
  {
    void* cluster = Cluster_to_Addr(clu_num);
    Clu_Type the_type = decide_clu_type(cluster);
    clu_table[clu_num] = the_type;
    if (the_type == DIR) {
      insert_dir_node(clu_num);
    }
  }
}

void initialize_dir_begin() {
  Dir_Begin = NULL;
}

void insert_dir_node(u32 clu_num) {
  Dir_Node* clu_node = malloc(sizeof(Dir_Node));
  clu_node->clu_num = clu_num;
  clu_node->nxt = Dir_Begin;
  Dir_Begin = clu_node;
}

void process_the_cluster(void* cluster, Clu_Type clu_type) {
  switch (clu_type){
    case DIR: {
      
      break;
    }
    case BMP_F: {
      break;
    }
    case BMP_I:
    case UNUSED:
      break;
  }
}

Clu_Type decide_clu_type(void* cluster) {
  if (judge_if_bmp_hdr(cluster)) {
    return BMP_F;
  }
  if (judge_if_unused(cluster)) {
    return UNUSED;
  }
  if (judge_if_dir(cluster)) {
    
    return DIR;
  }
  return BMP_I;
}

void* Cluster_to_Addr(u32 n) {
  u8* first_cluster = (u8 *)hdr
                        + ((hdr->BPB_RsvdSecCnt + (hdr->BPB_SecPerClus - 1)) / hdr->BPB_SecPerClus) * hdr->BPB_SecPerClus * hdr->BPB_BytsPerSec
                        + ((hdr->BPB_NumFATs * hdr->BPB_FATSz32 + (hdr->BPB_SecPerClus - 1)) / hdr->BPB_SecPerClus) * hdr->BPB_SecPerClus * hdr->BPB_BytsPerSec
  ;

  void* data_sec = (void*)(first_cluster + (n - 2) * hdr->BPB_SecPerClus * hdr->BPB_BytsPerSec);
  return data_sec;
}

// u32 Addr_to_Cluster(void* addr) {
//   u32 byte_offset = (u32) (addr - (void*) hdr);
//   u32 data_sec = byte_offset / hdr->BPB_BytsPerSec;
//   u32 n = (data_sec - hdr->BPB_RsvdSecCnt - hdr->BPB_NumFATs * hdr->BPB_FATSz32) / hdr->BPB_SecPerClus + 2;
//   return n;
// }

bool judge_if_dir(void* cluster) {
  size_t cluster_size = hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus;
  size_t bmp_cnt = 0;
  for (int i = 0; i < cluster_size - 2; i ++) {
        char* aim_str = cluster + i;
        if (aim_str[0] != 'B' && aim_str[0] != 'b') {
          continue;
        }
        if ((aim_str[1] == 'M' && aim_str[2] == 'P') || (aim_str[1] == 'm' && aim_str[2] == 'p')) {
          bmp_cnt ++;
        }
  }
  if (bmp_cnt > 2) {
    return true;
  }
  return false;
}

bool judge_if_unused(void* cluster) {
  size_t cluster_size = hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus;
  uint16_t* judger = cluster;
  for (int i = 0; i < 16; i++)
  {
    if (judger[i] != 0) {
      return false;
    }
  }
  if (judger[cluster_size >> 2]) {
    return false;
  }
  return true;
}

bool judge_if_bmp_hdr(void* cluster) {
  BMFileHdr* file_hdr = cluster;
  if (file_hdr->type != ('M' << 8 | 'B')) {
      return false;
  } 
  if (file_hdr->reserved1 != 0 || file_hdr->reserved2 != 0) {
    return false;
  }
  BMInfoHdr* info_hdr = cluster + 14;
  if (info_hdr->size != 40) {
    return false;
  }
  return true;
}

void *map_disk(const char *fname) {
  int fd = open(fname, O_RDWR);

  if (fd < 0) {
    perror(fname);
    goto release;
  }

  off_t size = lseek(fd, 0, SEEK_END);
  if (size == -1) {
    perror(fname);
    goto release;
  }

  struct fat32hdr *hdr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (hdr == (void *)-1) {
    goto release;
  }

  close(fd);

  if (hdr->Signature_word != 0xaa55 ||
      hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec != size) {
    fprintf(stderr, "%s: Not a FAT file image\n", fname);
    goto release;
  }
  return hdr;

release:
  if (fd > 0) {
    close(fd);
  }
  exit(1);
}
