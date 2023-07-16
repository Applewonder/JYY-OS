#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef struct fat32hdr Fat32Hdr;
typedef struct BITMAPFILEHEADER BMFileHdr;
typedef struct BITMAPINFOHEADER BMInfoHdr;
typedef struct fat32dent Fat32Dent;
typedef struct longentry LFN_ENTRY;

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
} __attribute__((packed));;

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

Fat32Hdr *hdr;

void *map_disk(const char *fname);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s fs-image\n", argv[0]);
    exit(1);
  }

  setbuf(stdout, NULL);

  assert(sizeof(struct fat32hdr) == 512); // defensive

  // map disk image to memory
  hdr = map_disk(argv[1]);

  u32 DataSec = hdr->BPB_RsvdSecCnt + hdr->BPB_NumFATs * hdr->BPB_FATSz32;
  u32 Clu_cnt = hdr->BPB_TotSec32 / hdr->BPB_SecPerClus;
  for (u32 cluster = DataSec; i < ; i++)
  {
    /* code */
  }
  

  // file system traversal
  munmap(hdr, hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec);
}

void* Cluster_to_addr(u32 n) {
  u32 data_
}

bool judge_if_dir(void* cluster) {
  size_t cluster_size = hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus;
  size_t bmp_cnt = 0;
  for (int i = 0; i < cluster_size - 2; i ++) {
        char* aim_str = cluster + i;
        if (aim_str[0] != 'B') {
          continue;
        }
        if (aim_str[1] == 'M' && aim_str[2] == 'P') {
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
  if (cluster + (cluster_size >> 1)) {

  }
  return true;
}

bool judge_if_bmp_hdr(void* cluster) {
  BMFileHdr* file_hdr = cluster;
  if (file_hdr->type != ('B' << 8 | 'M')) {
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
