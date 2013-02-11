;; Simple program to test dcache replacement of dirty blocks
;; and the cache copy-back process.
;;
;; compile with: arc-elf32-gcc -static -mA7 -o dcache_test dcache_test.s
;;
	.file	"dcache_test.s"
	.cpu	ARC700
   .globl main
   .type main, @function
   .section .text
   
main:

   mov      r6, 0
loop:

;; This test writes to two cache blocks that are
;; separated by 8192 bytes, thereby forcing them
;; to be in the same set. Choose set 42 so that 
;; the index is easily identified in wave plots.
;;
;; We start by loading into cache an address at
;; index 7 which will serve as a guaranteed cache
;; hit later in the test. This is in r1.
;;
   mov      r1, 7
   asl      r1, r1, 5
   add      r1, r1, 0x08000000
   ld       r2, [r1]       ;; cache miss, clean.
;;
;; Next compute two addresses in r2 and r3 which
;; have index == 42 but are to different addresses
;;
   mov      r2, 42         ;; cache set number
   asl      r2, r2, 5      ;; cache address 2
   mov      r3, 0x04000000 ;; tag for block 3
   or       r3, r3, r2     ;; cache address 3
;;
;; Now store to addresses 1 and 2 to bring each
;; block into cache and make each one dirty.
;;
   mov      r5, 0xdeadbeef
   st.ab    r5, [r2,4]
   st.ab    r5, [r2,4]
   st.ab    r5, [r2,4]
   st.ab    r5, [r2,4]
;;
   mov      r5, 0xa5a5a5a5
   st.ab    r5, [r3,4]
   st.ab    r5, [r3,4]
   st.ab    r5, [r3,4]
   st.ab    r5, [r3,4]
;;
;; Now instigate a miss to address 4, which has
;; the same index as addresses 2 and 3, and follow
;; that miss down the pipeline with a cache hit
;; to address 1 at index 7.
;;
   mov      r4, 42
   asl      r4, r4, 5
   add      r4, r4, 0x0c000000
   st       r5, [r4]             ;; cache miss
   ld       r2, [r1]             ;; cache hit

;;========= Load back modified blocks to check ====
;;
;; Compute two addresses in r2 and r3 which
;; have index == 42 but are to different addresses
;;
   mov      r2, 42         ;; cache set number
   asl      r2, r2, 5      ;; cache address 2
   mov      r3, 0x04000000 ;; tag for block 3
   or       r3, r3, r2     ;; cache address 3
;;
;; Now store to addresses 1 and 2 to bring each
;; block into cache and make each one dirty.
;;
   ld.ab    r5, [r2,4]
   ld.ab    r5, [r2,4]
   ld.ab    r5, [r2,4]
   ld.ab    r5, [r2,4]
;;
   ld.ab    r5, [r3,4]
   ld.ab    r5, [r3,4]
   ld.ab    r5, [r3,4]
   ld.ab    r5, [r3,4]
;;
   
;; Now repeat the test in case of i-cache misses 
;; first time around.
   
   cmp      r6, 9
   bne.d    loop 
   add      r6, r6, 1

   j     [blink]
