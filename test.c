#include <stdlib.h>
#include <time.h>
#include "my_malloc.h"

typedef void* (*malloc_func_type)(size_t);
typedef void (*free_func_type)(void *);

void test(malloc_func_type my_malloc, free_func_type my_free) {
	int test = 0;

	/* Tests for guarenteeing SIZE ordering, will not work with ADDRESS */
	printf("\n");
	void *s1000 = my_malloc(1000);
	void *sx1000 = my_malloc(1);
	void *s3 = my_malloc(30);
	void *sx3 = my_malloc(1);
	void *s2 = my_malloc(20);
	void *sx2 = my_malloc(1);
	void *s1 = my_malloc(10);
	void *sx1 = my_malloc(1);
	my_free(s1);
	my_free(s2);
	my_free(s3);
	my_free(s1000);
	printf("\n%d. gdb here to test that freelist is in size order. Sizes should be 34, 44, 54, 792, 1024.", ++test);
	my_free(sx1000);
	my_free(sx3);
	my_free(sx1);
	my_free(sx2);
	printf("\n%d. gdb here to test that freelist is a single block.", ++test);



	/* Tests for guarenteeing ADDRESS ordering, will not work with SIZE*/
	printf("\n");
	void *a1 = my_malloc(104);
	void *ax1 = my_malloc(1);
	void *a2 = my_malloc(103);
	void *ax2 = my_malloc(1);
	void *a3 = my_malloc(102);
	void *ax3 = my_malloc(1);
	void *a4 = my_malloc(101);
	void *ax4 = my_malloc(1);
	my_free(a4);
	my_free(a3);
	my_free(a2);
	my_free(a1);
	printf("\n%d. gdb here to test that freelist is in address order. Sizes should be 128, 127, 126, 125, 1542 (or 7586)", ++test);
	my_free(ax1);
	my_free(ax2);
	my_free(ax3);
	my_free(ax4);
	printf("\n%d. gdb here to test that freelist is a single block.", ++test);



	/* Tests for SINGLE_REQUEST_TOO_LARGE */
	printf("\n");
	void* thisIsNull = my_malloc(2049);
	printf("\n%d. Asking for too much data should set error code to SINGLE_REQUEST_TOO_LARGE: %d", ++test, ERRNO == SINGLE_REQUEST_TOO_LARGE ? 1 : 0);
	printf("\n%d. The returned pointer should also be null: %d", ++test, thisIsNull == NULL ? 1 : 0);



	/* Tests for modifying data and saving state correctly */
	printf("\n");
	int* num = (int*) my_malloc(sizeof(int));
	num[0] = 5;
	printf("\n%d. Storing int 5 in allocated memory should save 5: %d", ++test, num[0]);

	int sum = num[0] * 2;
	printf("\n%d. Modifying int in allocated memory should have desired effect. 5 * 2 = %d", ++test, sum);

	my_free(num);
	printf("\n%d. After freeing memory, freelist size should be a multiple of 2048: %d", ++test, getFreelistSize());



	/* Tests for mallocing entire 2048b blocks and OUT_OF_MEMORY */
	printf("\n");
	void* large = my_malloc(2016);
	printf("\n%d. Freelist should now be empty? GDB break to here.", ++test);
	void* forceSbrk = my_malloc(2016);
	printf("\n%d. Freelist should now be empty? GDB break to here.", ++test);
	void* forceSbrk2 = my_malloc(2016);
	printf("\n%d. Freelist should now be empty? GDB break to here.", ++test);
	void* forceSbr3 = my_malloc(2016);
	printf("\n%d. Freelist should now be empty? GDB break to here.", ++test);
	void* outOfMemory = my_malloc(2016);
	printf("\n%d. Asking for too much data should set error code to OUT_OF_MEMORY: %d", ++test, ERRNO == OUT_OF_MEMORY ? 1 : 0);
	printf("\n%d. outOfMemory should be null: %d", ++test, outOfMemory == NULL ? 1 : 0);
	my_free(large);
	my_free(forceSbrk);
	my_free(forceSbrk2);
	my_free(forceSbr3);



	/* Tests for making sure bulk mallocs are all in use and correctly freed. */
	printf("\n");
	void* manyMalloc[16];
	for (int i = 0; i < 16; i++) {
		manyMalloc[i] = my_malloc(488);
	}
	printf("\n%d. All user allocated memory should be in use: ", ++test);
	int all1 = 1;
	for (int i = 0; i < 16; i++) {
		metadata_t* pt = (metadata_t*) (((char*) manyMalloc[i]) - sizeof(metadata_t));
		// if (pt->in_use == 0) {
		// 	all1 = 0;
		// }
		printf("%d",  pt->in_use);
	}

	/* Free odd-indexed memory */
	for (int i = 15; i >= 1; i -= 2) {
		my_free(manyMalloc[i]);
	}
	/* Free even-indexed memory */
	for (int i = 14; i >= 0; i -= 2) {
		my_free(manyMalloc[i]);
	}



	/* Tests freeing bulk memory in various orders and that integrity of data is maintained */
	printf("\n");
	int* f1 = (int*) my_malloc(sizeof(int) * 70);
	int* f2 = (int*) my_malloc(sizeof(int) * 70);
	int* f3 = (int*) my_malloc(sizeof(int) * 70);
	int* f4 = (int*) my_malloc(sizeof(int) * 70);
	for (int i = 0; i < 70; i++) {
		f1[i] = 1;
		f2[i] = 2;
		f3[i] = 3;
		f4[i] = 4;
	}
	my_free(f2);
	my_free(f4);
	all1 = 1;
	int all3 = 1;
	for (int i = 0; i < 70; i++) {
		if (f1[i] != 1) {
			all1 = 0;
		}
		if (f3[i] != 3) {
			all3 = 0;
		}
	}
	printf("\n%d. After f2 and f4 freed, f1 should still have all 1s: %d", ++test, all1);
	printf("\n%d. f3 should still have all 3s: %d", ++test, all3);
	my_free(f3);
	all1 = 1;
	for (int i = 0; i < 70; i++) {
		if (f1[i] != 1) {
			all1 = 0;
		}
	}
	printf("\n%d. After f3 freed, f1 should still have all 1s: %d", ++test, all1);

	printf("\n");
	// int* f1 = (int*) my_malloc(sizeof(int) * 70);
	f2 = (int*) my_malloc(sizeof(int) * 70);
	f3 = (int*) my_malloc(sizeof(int) * 70);
	f4 = (int*) my_malloc(sizeof(int) * 70);
	for (int i = 0; i < 70; i++) {
		// f1[i] = 1;
		f2[i] = 2;
		f3[i] = 3;
		f4[i] = 4;
	}
	my_free(f3);
	my_free(f1);
	int all2 = 1;
	int all4 = 1;
	for (int i = 0; i < 70; i++) {
		if (f2[i] != 2) {
			all2 = 0;
		}
		if (f4[i] != 4) {
			all4 = 0;
		}
	}
	printf("\n%d. After f1 and f3 freed, f2 should still have all 2s: %d", ++test, all2);
	printf("\n%d. f4 should still have all 4s: %d", ++test, all4);
	my_free(f2);
	all4 = 1;
	for (int i = 0; i < 70; i++) {
		if (f4[i] != 4) {
			all4 = 0;
		}
	}
	printf("\n%d. After f2 freed, f4 should still have all 1s: %d", ++test, all4);

	printf("\n");
	f1 = (int*) my_malloc(sizeof(int) * 70);
	f2 = (int*) my_malloc(sizeof(int) * 70);
	f3 = (int*) my_malloc(sizeof(int) * 70);
	// f4 = (int*) my_malloc(sizeof(int) * 70);
	for (int i = 0; i < 70; i++) {
		f1[i] = 1;
		f2[i] = 2;
		f3[i] = 3;
		// f4[i] = 4;
	}
	my_free(f3);
	my_free(f1);
	all2 = 1;
	all4 = 1;
	for (int i = 0; i < 70; i++) {
		if (f2[i] != 2) {
			all2 = 0;
		}
		if (f4[i] != 4) {
			all4 = 0;
		}
	}
	printf("\n%d. After f1 and f3 freed, f2 should still have all 2s: %d", ++test, all2);
	printf("\n%d. f4 should still have all 4s: %d", ++test, all4);
	my_free(f4);
	all2 = 1;
	for (int i = 0; i < 70; i++) {
		if (f2[i] != 2) {
			all2 = 0;
		}
	}
	printf("\n%d. After f4 freed, f2 should still have all 1s: %d", ++test, all4);
	my_free(f2);



	/* Tests mallocing, freeing, and then mallocing more. */
	printf("\n");
	int *m1 = (int*) my_malloc(sizeof(int) * 68);
	m1[0] = 1;
	int *m2 = (int*) my_malloc(sizeof(int) * 68);
	m2[0] = 2;
	int *m3 = (int*) my_malloc(sizeof(int) * 68);
	m3[0] = 3;
	my_free(m2);
	printf("\n%d. m1 and m3 should be 1 and 3: %d and %d", ++test, m1[0], m3[0]);
	int *m4 = (int*) my_malloc(sizeof(int) * 68);
	m4[0] = 4;
	my_free(m1);
	printf("\n%d. m3 and m4 should be 3 and 4: %d and %d", ++test, m3[0], m4[0]);
	int *m5 = (int*) my_malloc(sizeof(int) * 68);
	m5[0] = 5;
	my_free(m4);
	printf("\n%d. m3 and m5 should be 3 and 5: %d and %d", ++test, m3[0], m5[0]);
	int *m6 = (int*) my_malloc(sizeof(int) * 68);
	m6[0] = 6;
	int *m7 = (int*) my_malloc(sizeof(int) * 68);
	m7[0] = 7;
	my_free(m3);
	my_free(m7);
	printf("\n%d. m5 and m7 should be 5 and 6: %d and %d", ++test, m5[0], m6[0]);
	my_free(m5);
	my_free(m6);
	printf("\n%d. After all freed, gdb here to make sure freelist is fully coalesced.", ++test);



	/* THIS TEST WILL WORK IN CONTEXT. HOWEVER, IT WILL NOT EFFECTIVELY TEST UNLESS RUN ALONE.
	Tests for correct coalescing of my_sbrk in my_malloc_size_order.
	NOT RELEVANT FOR ADDRESS ORDER. */
	void *n1 = my_malloc(1500);
	void *n1x = my_malloc(1);
	void *n2 = my_malloc(475);
	my_free(n2);
	my_free(n1);
	void *nCritical = my_malloc(1600); // This will force a my_sbrk call.
	printf("\n%d. gdb to the nCritical malloc call and step through until after you make the my_sbrk call to make sure first element in freelist size is 1524, NOT 499.", ++test);
	my_free(n1x);
	my_free(nCritical);

}

int main() {
    /* you can change this number to modify how many times the function will be run */
    const long unsigned int NUM_RUNS = 1;
    clock_t start_time = clock();
    for (long unsigned int i = 0; i < NUM_RUNS; i++)
        test(my_malloc_size_order, my_free_size_order);

    clock_t post_size_time = clock();
    for (long unsigned int i = 0; i < NUM_RUNS; i++)
        test(my_malloc_addr_order, my_free_addr_order);

    clock_t post_addr_time = clock();
    long unsigned int milli_seconds_size = (post_size_time - start_time) * 1000 / CLOCKS_PER_SEC;
    long unsigned int milli_seconds_addr = (post_addr_time - post_size_time) * 1000 / CLOCKS_PER_SEC;

    printf("Time to run %lu iterations in milliseconds:\n", NUM_RUNS);
    printf("sorted by size test: %lu\n", milli_seconds_size);
    printf("sorted by addr test: %lu\n", milli_seconds_addr);

	return 0;
}
