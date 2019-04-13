//
//  virt_mem.c
//  virt_mem
//
//  Created by William McCarthy on 3/23/19.
//  Copyright © 2019 William McCarthy. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <alloca.h>

#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define FRAME_SIZE  256
#define TLB_SIZE 1000

int pageTableNumbers[FRAME_SIZE];  // array to hold the page numbers in the page table
int pageTableFrames[FRAME_SIZE];   // array to hold the frame numbers in the page table

int TLBPageNumber[TLB_SIZE];  // array to hold the page numbers in the TLB
int TLBFrameNumber[TLB_SIZE]; // array to hold the frame numbers in the TLB

int physicalMemory[FRAME_SIZE][FRAME_SIZE]; // physical memory 2D array

int pageFaults = 0;   // counter to track page faults
int TLBHits = 0;      // counter to track TLB hits
int firstAvailableFrame = 0;  // counter to track the first available frame
int firstAvailablePageTableNumber = 0;  // counter to track the first available page table entry
int numberOfTLBEntries = 0;

char buf[BUFLEN];
FILE* backing_store;

//-------------------------------------------------------------------
void pagetable(unsigned int x,unsigned int y);
void readFromStore(unsigned int x);
void insertIntoTLB(unsigned int x,unsigned int y);
void readFromStore(unsigned int x);

unsigned int getpage(size_t x) { return (0xff00 & x) >> 8; }
unsigned int getoffset(unsigned int x) { return (0xff & x); }

void getpage_offset(unsigned int x) {
  unsigned int page = getpage(x);
  unsigned int offset = getoffset(x);
  printf("x is: %u, page: %u, offset: %u, address: %u, paddress: %u\n", x, page, offset,
         (page << 8) | getoffset(x), page * 256 + offset);
}



int main(int argc, const char * argv[]) {
  FILE* fadd = fopen("addresses.txt", "r");
  if (fadd == NULL) { fprintf(stderr, "Could not open file: 'addresses.txt'\n");  exit(FILE_ERROR);  }

   FILE* fcorr = fopen("correct.txt", "r");
  if (fcorr == NULL) { fprintf(stderr, "Could not open file: 'correct.txt'\n");  exit(FILE_ERROR);  }

  backing_store = fopen("BACKING_STORE.bin", "r");
  if (backing_store == NULL) { fprintf(stderr, "Could not open file: 'BACKING_STORE.bin'\n");  exit(FILE_ERROR);  }

  unsigned int page, offset, physical_add, frame = 0;
  unsigned int logic_add;                  // read from file address.txt
  unsigned int virt_add, phys_add, value;  // read from file correct.txt
  unsigned int numberOfTranslatedAddresses = 0;

  

      // not quite correct -- should search page table before creating a new entry
      //   e.g., address # 25 from addresses.txt will fail the assertion
      // TODO:  add page table code
      // TODO:  add TLB code
  while (frame < 20) {
    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add,
           buf, buf, &phys_add, buf, &value);  // read from file correct.txt

    fscanf(fadd, "%d", &logic_add);  // read from file address.txt
    page = getpage(logic_add);
    offset = getoffset(logic_add);

    pagetable(page,frame);
    physical_add = frame++ * FRAME_SIZE + offset;
    
    assert(physical_add == phys_add);
    // todo: read BINARY_STORE and confirm value matches read value from correct.txt
    readFromStore(page);
    printf("logical: %5u (page:%3u, offset:%3u) ---> physical: %5u -- passed\n", logic_add, page, offset, physical_add);
    if (frame % 5 == 0) { printf("\n"); }
   ++numberOfTranslatedAddresses;
  }
  fclose(fcorr);
  fclose(fadd);
  fclose(backing_store);

  printf("Number of translated addresses = %d\n", numberOfTranslatedAddresses);
  double pfRate = pageFaults / (double)numberOfTranslatedAddresses;
  double TLBRate = TLBHits / (double)numberOfTranslatedAddresses;
    
  printf("Page Faults = %d\n", pageFaults);
  printf("Page Fault Rate = %.3f\n",pfRate);
  printf("TLB Hits = %d\n", TLBHits);
  printf("TLB Hit Rate = %.3f\n", TLBRate);
  
  return 0;
}

void pagetable(unsigned int x,unsigned int y){
    
    int i;  // look through TLB for a match
    for(i = 0; i < TLB_SIZE; i++){
        if(TLBPageNumber[i] == x){   
            y = TLBFrameNumber[i];  
                ++TLBHits;               
        }
    }
    
  
    if(y == 0){
        int i;  
        for(i = 0; i < firstAvailablePageTableNumber; i++){
            if(pageTableNumbers[i] == x){        
                y  = pageTableFrames[i];          
            }
        }
        if(y  == 0){                     
            readFromStore(x);             
            pageFaults++;                          
            y = firstAvailableFrame - 1;  
        }
    }
    insertIntoTLB(x,y);
}

void insertIntoTLB(unsigned int x,unsigned int y){
    
    int i;  // if it's already in the TLB, break
    for(i = 0; i < numberOfTLBEntries; i++){
        if(TLBPageNumber[i] == x){
            break;
        }
    }
    
   
    if(i == numberOfTLBEntries){
        if(numberOfTLBEntries < TLB_SIZE){  
            TLBPageNumber[numberOfTLBEntries] = x;   
            TLBFrameNumber[numberOfTLBEntries] = y;
        }
        else{                                           
            for(i = 0; i < TLB_SIZE - 1; i++){
                TLBPageNumber[i] = TLBPageNumber[i + 1];
                TLBFrameNumber[i] = TLBFrameNumber[i + 1];
            }
            TLBPageNumber[numberOfTLBEntries-1] = x;  
            TLBFrameNumber[numberOfTLBEntries-1] = y;
        }        
    }
    else{
        for(i = i; i < numberOfTLBEntries - 1; i++){      
            TLBPageNumber[i] = TLBPageNumber[i + 1];      
            TLBFrameNumber[i] = TLBFrameNumber[i + 1];
        }
        if(numberOfTLBEntries < TLB_SIZE){                
            TLBPageNumber[numberOfTLBEntries] = x;
            TLBFrameNumber[numberOfTLBEntries] = y;
        }
        else{                                             
            TLBPageNumber[numberOfTLBEntries-1] = x;
            TLBFrameNumber[numberOfTLBEntries-1] = y;
        }
    }
    if(numberOfTLBEntries < TLB_SIZE){                   
        numberOfTLBEntries++;
    }    
}

void readFromStore(unsigned int x){
    if (fseek(backing_store, x * BUFLEN, SEEK_SET) != 0) {
        fprintf(stderr, "Error seeking in backing store\n");
    }
    
    if (fread(buf, sizeof(signed char), BUFLEN, backing_store) == 0) {
        fprintf(stderr, "Error reading from backing store\n");        
    }
    
    int i;
    for(i = 0; i < BUFLEN; i++){
        physicalMemory[firstAvailableFrame][i] = buf[i];
    }
   
    pageTableNumbers[firstAvailablePageTableNumber] = x;
    pageTableFrames[firstAvailablePageTableNumber] = firstAvailableFrame;
    
    firstAvailableFrame++;
    firstAvailablePageTableNumber++;
}


