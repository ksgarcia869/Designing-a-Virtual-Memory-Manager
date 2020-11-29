//
//  memmgr.c
//  memmgr
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define FRAME_SIZE  256
#define TLB_SIZE 16
#define PHYSICAL_MEMORY_SIZE BUFLEN * FRAME_SIZE


//-------------------------------------------------------------------

int Hit, TLB_index, TLB_size, TLB_hit_count = 0;
int address_count, page_fault_count = 0;
float TLB_hit_rate, page_fault_rate = 0;


struct TLBTable{
  unsigned int page_num;
  unsigned int frame_num;
};

unsigned getpage(unsigned x) { return (0xff00 & x) >> 8; }

unsigned getoffset(unsigned x) { return (0xff & x); }

void getpage_offset(unsigned x) {
  unsigned  page   = getpage(x);
  unsigned  offset = getoffset(x);
  printf("x is: %u, page: %u, offset: %u, address: %u, paddress: %u\n", x, page, offset,
         (page << 8) | getoffset(x), page * 256 + offset);
}

int main(int argc, const char* argv[]) {
  FILE* fadd = fopen("addresses.txt", "r");    // open file addresses.txt  (contains the logical addresses)
  if (fadd == NULL) { fprintf(stderr, "Could not open file: 'addresses.txt'\n");  exit(FILE_ERROR);  }

  FILE* fcorr = fopen("correct.txt", "r");     // contains the logical and physical address, and its value
  if (fcorr == NULL) { fprintf(stderr, "Could not open file: 'correct.txt'\n");  exit(FILE_ERROR);  }

  FILE* fback = fopen("BACKING_STORE.bin", "rb");     // contains the logical and physical address, and its value
  if (fback == NULL) { fprintf(stderr, "Could not open file: 'BACKING_STORE.bin'\n");  exit(FILE_ERROR);  }

  char buf[BUFLEN];
  unsigned   page, offset, physical_add, frame = 0;
  unsigned   logic_add;                  // read from file address.txt
  unsigned   virt_add, phys_add, value;  // read from file correct.txt

  int physical_mem[PHYSICAL_MEMORY_SIZE];
  int index;

  int page_table[BUFLEN];
  memset(page_table,-1, 256*sizeof(int));

  struct TLBTable TLB[TLB_SIZE];
  memset(page_table,-1,16*sizeof(char));

  printf("ONLY READ FIRST 20 entries -- TODO: change to read all entries\n\n");

  while (frame < 244) {

    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add,
           buf, buf, &physical_add, buf, &value);  // read from file correct.txt

    fscanf(fadd, "%d", &logic_add);  // read from file address.txt
    page   = getpage(  logic_add);
    offset = getoffset(logic_add);

    Hit = -1;

    address_count++;

    for(index = 0; index < TLB_SIZE; index++){
      if(TLB[index].page_num == page){
        Hit = TLB[index].frame_num;
        phys_add = Hit*FRAME_SIZE + offset;
      }
    }

    if(!(Hit == -1)){
      TLB_hit_count++;
    }

    else if(page_table[page]==-1){
      fseek(fback, page*FRAME_SIZE, SEEK_SET);
      fread(buf, sizeof(char),FRAME_SIZE,fback);
      page_table[page] = frame;

      for(index = 0; index < FRAME_SIZE; index++){
        physical_mem[frame*FRAME_SIZE + index] = buf[index];
      }
      page_fault_count++;
      physical_add = frame * FRAME_SIZE + offset;
      frame++;

      if(TLB_size == 16)
        TLB_size--;

      for(TLB_index = TLB_size; TLB_index > 0; TLB_index--){
        TLB[TLB_index].page_num = TLB[TLB_index-1].page_num;
        TLB[TLB_index].frame_num = TLB[TLB_index-1].frame_num;
      }

      if(TLB_size <= 15)
        TLB_size++;

      TLB[0].page_num = page;
      TLB[0].frame_num = page_table[page];
      phys_add = page_table[page]*FRAME_SIZE + offset;
    }

    else{

      phys_add = page_table[page]*FRAME_SIZE + offset;
    }

    value = physical_mem[phys_add];


    assert(physical_add == phys_add);

  //printf("\n Physical address: %d\n",physical_add);


    printf("logical: %5u (page: %3u, offset: %3u) ---> physical: %5u -- passed\n", logic_add, page, offset, phys_add);
    if (frame%5 == 0) { printf("\n");}
  }

  page_fault_rate = page_fault_count*1.0f/address_count;
  TLB_hit_rate = TLB_hit_count*1.0f / address_count;

  fclose(fcorr);
  fclose(fadd);
  fclose(fback);

  printf("\nPage Fault Rate: %f \n", page_fault_rate);
  printf("\nTLB Hit Rate: %f \n", TLB_hit_rate);

  printf("ONLY READ FIRST 20 entries -- TODO: change to read all entries\n\n");

  printf("ALL logical ---> physical assertions PASSED!\n");
  printf("!!! This doesn't work passed entry 24 in correct.txt, because of a duplicate page table entry\n");
  printf("--- you have to implement the PTE and TLB part of this code\n");

//  printf("NOT CORRECT -- ONLY READ FIRST 20 ENTRIES... TODO: MAKE IT READ ALL ENTRIES\n");

  printf("\n\t\t...done.\n");
  return 0;
}
