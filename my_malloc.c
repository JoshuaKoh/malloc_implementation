#include "my_malloc.h"

/* You *MUST* use this macro when calling my_sbrk to allocate the
 * appropriate size. Failure to do so may result in an incorrect
 * grading!
 */
#define SBRK_SIZE 2048

/* If you want to use debugging printouts, it is HIGHLY recommended
 * to use this macro or something similar. If you produce output from
 * your code then you may receive a 20 point deduction. You have been
 * warned.
 */
#ifdef DEBUG
#define DEBUG_PRINT(x) printf x
#else
#define DEBUG_PRINT(x)
#endif


/* our freelist structure - this is where the current freelist of
 * blocks will be maintained. failure to maintain the list inside
 * of this structure will result in no credit, as the grader will
 * expect it to be maintained here.
 * Technically this should be declared static for the same reasons
 * as above, but DO NOT CHANGE the way this structure is declared
 * or it will break the autograder.
 */
metadata_t* freelist;

enum ORDER sortBy;

void* my_malloc_size_order(size_t size)
{
    sortBy = SIZE;

    /*
    If user request exceeds 2048 bytes (after metadata), set code
    SINGLE_REQUEST_TOO_LARGE and return null.
    */
    if (size + sizeof(metadata_t) > SBRK_SIZE) {
        ERRNO = SINGLE_REQUEST_TOO_LARGE;
        return NULL;
    }

    /* If free list is empty, get new block from my_sbrk */
    if (freelist == NULL) {
        metadata_t *temp = (metadata_t *) my_sbrk(SBRK_SIZE);
        /*
        If my_sbrk returns null, we are out of heap space. Set code
        OUT_OF_MEMORY and return null.
        */
        if (temp == NULL) {
            ERRNO = OUT_OF_MEMORY;
            return NULL;
        } else {
            freelist = temp;
        }
        freelist->in_use = 0;
        freelist->size = SBRK_SIZE;
        // prev and next pointers are already null
    }
    void *ret = getMemory(size);
    if (ret != NULL) {
        ERRNO = NO_ERROR;
        return ret;
    } else {
        return NULL;
    }
}

void* my_malloc_addr_order(size_t size)
{
    sortBy = ADDRESS;
    /*
    If user request exceeds 2048 bytes (after metadata), set code
    SINGLE_REQUEST_TOO_LARGE and return null.
    */
    if (size + sizeof(metadata_t) > SBRK_SIZE) {
        ERRNO = SINGLE_REQUEST_TOO_LARGE;
        return NULL;
    }

    /* If free list is empty, get new block from my_sbrk */
    if (freelist == NULL) {
        metadata_t *temp = (metadata_t *) my_sbrk(SBRK_SIZE);
        /*
        If my_sbrk returns null, we are out of heap space. Set code
        OUT_OF_MEMORY and return null.
        */
        if (temp == NULL) {
            ERRNO = OUT_OF_MEMORY;
            return NULL;
        } else {
            freelist = temp;
        }
        freelist->in_use = 0;
        freelist->size = SBRK_SIZE;
        // prev and next pointers are already null
    }
    void *ret = getMemory(size);
    if (ret != NULL) {
        ERRNO = NO_ERROR;
        return ret;
    } else {
        return NULL;
    }
}

/*
This large funciton does all of the following:
- Finds smallest available memory block in freelist that fits the size request.
- Removes block from freelist.
- Splits it if necessary and returns leftover block to freelist.
- Updates metadata and returns pointer to start of user's memory
Includes some questionable use of recursion to expand freelist size with my_sbrk in the event that there is not enough memory in freelist to suit user's request.

Preconditions:
- freelist is not NULL.
Postconditions:
- pointer to the start of the user's memory is returned
- metadata for user's memory has been updated, in_use = 1
- freelist has been updated to remove user's memory and sometimes readd leftover memory
*/
void* getMemory(size_t size) {
    metadata_t* index = freelist;
    /* end of list, set if we reach end of free list w/o finding adequate
    memory space */
    int eol = 0;

    /* For SIZE, iterate through freelist until the first block of adequate size is found. */
    if (sortBy == SIZE) {
        do {
            /* Test if block [is available and] of adequate size */
            if (index->in_use == 0 && index->size >= size + sizeof(metadata_t)) {
                short blkSize = index->size;
                /* Handle if leftover is smaller than sizeof(metadata). In
                this case, simply return the whole memory block. */
                if (size + (sizeof(metadata_t) * 2) + 1 >= blkSize) {
                    removeFromFreelist(index);
                    index->in_use = 1;
                    void* ret = ((char*) index) + sizeof(metadata_t);
                    return ret;
                }
                /* Otherwise split into two blocks, "index" which the user will
                use and "leftover" which will return to the freelist. */
                metadata_t* leftover = (metadata_t*) (((char*) index) + size + sizeof(metadata_t));
                leftover->size = blkSize - size - sizeof(metadata_t);
                leftover->in_use = 0;
                leftover->next = NULL;
                leftover->prev = NULL;

                removeFromFreelist(index);
                index->size = size + sizeof(metadata_t);
                index->in_use = 1;

                addToFreeList(leftover);

                void* ret = (void*) (((char*) index) + sizeof(metadata_t));
                return ret;
            }

            /* If next is not null and we have not found an adequately sized
            block, there is more memory to test. Otherwise, stop iterating. */
            if (index->next != NULL) {
                index = index->next;
            } else {
                eol = 1;
            }
        } while (!eol);
    /* For ADDRESS, iterate though the entire freelist first, and decide which block fits best after checking them all. */
    } else if (sortBy == ADDRESS) {
        metadata_t* bestFit;
        short bestSize = 8193;
        do {
            /* Test if block is [available and] of adequate size. In the case of a tie between sizes, the first occurence in memory is used. */
            if (index->in_use == 0 && index->size >= size + sizeof(metadata_t) && index->size < bestSize) {
                bestFit = index;
                bestFit->prev = index->prev;
                bestFit->next = index->next;
                bestFit->size = index->size;
                bestSize = index->size;
            }
            /* If next is not null, there is more memory to test. Otherwise, stop iterating. */
            if (index->next != NULL) {
                index = index->next;
            } else {
                eol = 1;
            }
        } while (!eol);

        /* If bestSize was not changed, no adequate memory was found, and so drop through to my_sbrk call. */
        if (bestSize != 8193) {
            index = bestFit; // for consistency with SIZE
            index->prev = bestFit->prev;
            index->next = bestFit->next;
            index->size = bestFit->size;

            removeFromFreelist(index);
            index->in_use = 1;

            /* Test if leftover is larger than sizeof(metadata). If not,
            simply drop through and return the whole memory block. */
            if ((size + (sizeof(metadata_t) * 2)) + 1 <= index->size) {
                /* Split into two blocks, "index" which the user will
                use and "leftover" which will return to the freelist. */
                index->size = size + sizeof(metadata_t);

                metadata_t* leftover = (metadata_t*) (((char*) index) + size + sizeof(metadata_t));
                leftover->size = bestSize - (size
                    + sizeof(metadata_t));
                leftover->in_use = 0;
                leftover->next = NULL;
                leftover->prev = NULL;

                addToFreeList(leftover);
            }
            void* ret = (void*) (((char*) index) + sizeof(metadata_t));
            return ret;
        }
    }

    /*
    If we reach this point, there was not enough memory in freelist to accomodate request. We need to call my_sbrk. Conveniently, index happens to point to the end of the list, so simply set its next to the new large block.
    */
    metadata_t *temp = (metadata_t *) my_sbrk(SBRK_SIZE);
    /*
    If my_sbrk returns null, we are out of heap space. Set code
    OUT_OF_MEMORY and return null.
    */
    if (temp == NULL) {
        ERRNO = OUT_OF_MEMORY;
        return NULL;
    }
    temp->size = SBRK_SIZE;
    temp->in_use = 0;

    /* Adaptation of coalesceLeftandRight to only coalesce left. */
    metadata_t* left = findLeftBlk(temp);
    if (left != NULL && left->in_use == 0) {
        /* Absorb left's size into the returning size */
        short tempSize = temp->size;
        temp = left;
        temp->size += tempSize;
        /* Since you're absorbing the left, remove it from freelist */
        removeFromFreelist(left);
    }
    temp->prev = NULL;
    temp->next = NULL;
    addToFreeList(temp);
    return getMemory(size); // Could be implemented with a "didSBRKHappen" boolean instead of recursion, since this will only recurse once.
}

/*
Removes index from the free list by updating previous and next references.
Pointers in C are glorious and amazing.

Preconditions:
- index is pointing the the metadata, not the user's address
*/
void removeFromFreelist(metadata_t* index) {
    if (index->prev == NULL && index->next == NULL) {
        /* If index's prev and next are null, we know that we have
        requested the only memory available in the freelist. Thus,
        the freelist is now empty. */
        freelist = NULL;
    } else if (index->prev == NULL) {
        /* Update the alloced memory's next block's prev to null,
        since the alloced memory is being removed.
        Further, if index's prev is null, we know we must be at the
        head of the freelist. Thus, update freelist pointer. */
        metadata_t* temp = index->next;
        temp->prev = NULL;
        freelist = temp;
    } else if (index->next == NULL) {
        /* Update the alloced memory's prev block's next to null,
        since the alloced memory is being removed.
        For the record, since index's next is null, we know we must
        be at the last node of the freelist. */
        metadata_t* temp = index->prev;
        temp->next = NULL;
    } else {
        /* Join the alloced memory's prev and next blocks, since the
        alloced memory is being removed.
        For the record, since index has a prev and next reference,
        we know we are inclusively inside the freelist. */
        metadata_t* Iprev = index->prev;
        metadata_t* Inext = index->next;
        Iprev->next = Inext;
        Inext->prev = Iprev;
    }
}

/*
Iterate through the freelist until we find a memory block whose size/address is larger than our block to add. Our block to add should be inserted TO THE LEFT of this larger block.
This is ONLY for adding to free list and does NOT attempt to coalesce with neighbors before adding. Thus, this should only be called through other methods in my_malloc in context, not accessed directly.

Preconditions:
- freelist is not NULL.
- ptr is pointing to the metadata, not the user's address
- ptr has ALREADY attempted to coalesce with neighbors
Postconditions:
- addThis has been added to the free list in appropriate order (size or address).
*/
void addToFreeList(metadata_t* addThis) {
    addThis->in_use = 0;
    /* Handle if free list is empty. */
    if (freelist == NULL) {
        freelist = addThis;
        return;
    }
    metadata_t *index = freelist;
    int eol = 0;
    if (sortBy == SIZE) {
        while (!eol) {
            if (index->size > addThis->size) {
                /* If index's prev is null, it means we are at the start of
                the free list (and our ptr to add is the smallest in the
                list). Thus, update only next references references, and update freelist pointer. */
                if (index->prev == NULL) {
                    addThis->next = index;
                    index->prev = addThis;
                    freelist = addThis;
                    return;
                } else {
                    /* Otherwise, we are somewhere in the middle of the free list, so update prev and next references. */
                    metadata_t* Iprev = index->prev;
                    addThis->prev = Iprev;
                    Iprev->next = addThis;
                    addThis->next = index;
                    index->prev = addThis;
                    return;
                }
            }
            if (index->next == NULL) {
                eol = 1;
            } else {
                index = index->next;
            }
        }
        /* If the while loop terminates, our ptr to add is the largest in the
        free list, so add it to the end. */
        addThis->prev = index;
        index->next = addThis;
        return;
    }
    if (sortBy == ADDRESS) {
        while (!eol) {
            if (index > addThis) {
                /* If index's prev is null, it means we are at the start of
                the free list (and our ptr to add is the smallest in the
                list). Thus, update only next references, and update freelist pointer. */
                if (index->prev == NULL) {
                    addThis->next = index;
                    index->prev = addThis;
                    freelist = addThis;
                    return;
                } else {
                    /* Otherwise, we are somewhere in the middle of the free list, so update prev and next references. */
                    metadata_t* Iprev = index->prev;
                    addThis->prev = Iprev;
                    Iprev->next = addThis;
                    addThis->next = index;
                    index->prev = addThis;
                    freelist = addThis;
                    return;
                }
            }
            if (index->next == NULL) {
                eol = 1;
            } else {
                index = index->next;
            }
        }
        /* If the while loop terminates, our ptr to add is the largest in the
        free list, so add it to the end. */
        addThis->prev = index;
        index->next = addThis;
        return;
    }
}

void my_free_size_order(void* ptr)
{
    if (ptr == NULL) {
        return;
    }
    /* If ptr is not null, attempt to coalesce with neighbors then add to freelist. */
    sortBy = SIZE;
    metadata_t* addThis = coalesceLeftAndRight(ptr);
    addToFreeList(addThis);
    ERRNO = NO_ERROR;
}

void my_free_addr_order(void* ptr)
{
    if (ptr == NULL) {
        return;
    }
    /* If ptr is not null, attempt to coalesce with neighbors then add to freelist. */
    sortBy = ADDRESS;
    metadata_t* addThis = coalesceLeftAndRight(ptr);
    addToFreeList(addThis);
    ERRNO = NO_ERROR;
}

/*
Iterates through the freelist to find the memory block whose address is
closest to ptr and also to the left. Returns NULL if ptr is the leftmost address, or if the left block is in_use.
This serves as a helper function for coalesceLeftAndRight.
*/
metadata_t* findLeftBlk(metadata_t* ptr) {
    /* Handle the case where freelist is empty */
    if (freelist == NULL) {
        return NULL;
    }

    /* Handle the case where ptr is lower than the freelist pointer. Without this, trying to find the left block will try to grab something very low in memory like x20ff1, which is definitely not desired.*/
    metadata_t* lowestInFL = freelist;
    metadata_t* lowestTemp = freelist;
    while (lowestTemp != NULL) {
        if (lowestTemp < lowestInFL) {
            lowestInFL = lowestTemp;
        }
        lowestTemp = lowestTemp->next;
    }
    if (ptr < lowestInFL) {
        return NULL;
    }

    metadata_t *index = freelist;
    /* Handle the case where freelist is one block long. */
    if (index->next == NULL) {
        if (ptr < index) {
            return NULL;
        } else {
            /* Finally, test that this left block is indeed directly left of ptr. If not, then something else (in use) is between them, so return NULL. */
            if ((metadata_t *) (((char*) index) + index->size) == ptr) {
                return index;
            } else {
                return NULL;
            }
        }
    }

    /* Handle the case where freelist is multiple blocks long. */
    metadata_t *left = lowestInFL;
    int eol = 0;
    int change = 0;
    do {
        /* If the index element in free list is smaller than the pointer and larger than the currently saved best fit for left, update left. */
        if (index < ptr && index >= left) {
            left = index;
            change = 1;
        }
        /* If end of free list reached, terminate looping */
        if (index->next == NULL) {
            eol = 1;
        } else {
            index = index->next;
        }
    } while (!eol);

    /* This test for change was likely depricated by adding the case handling where ptr is lower than free list pointer, but I'm too lazy to re-open my VM to be sure... */
    if (!change) {
        return NULL;
    } else {
        /* Finally, test that this left block is indeed directly left of ptr. If not, then something else (in use) is between them, so return NULL. */
        metadata_t* test = (metadata_t*) (((char*) left) + left->size);
        if (test == ptr) {
            return left;
        } else {
            return NULL;
        }
    }
}


/* Iterate through the freelist to find the memory block whose address is
closestto ptr and also to the right. Returns NULL if ptr is the rightmost address. */
metadata_t* findRightBlk(metadata_t* ptr) {
    /* Handle the case where freelist is empty */
    if (freelist == NULL) {
        return NULL;
    }

    /* Handle the case where ptr is the rightmost block. */
    metadata_t* highestInFL = freelist;
    metadata_t* highestTemp = freelist;
    while (highestTemp != NULL) {
        if (highestTemp > highestInFL) {
            highestInFL = highestTemp;
        }
        highestTemp = highestTemp->next;
    }
    if (ptr > highestInFL) {
        return NULL;
    }
    /* Otherwise, use pointer arithmetic to reach ptr's next block. */
    /* This is primarily to make sure that adding the size does not cause us to "fall off" the far end of the freelist's available space. */
    metadata_t* potentialRight = (metadata_t*) (((char*) ptr) + ptr->size);
    metadata_t* index = freelist;
    while (index != NULL) {
        if (index == potentialRight) {
            return potentialRight;
        }
        index = index->next;
    }
    return NULL;


    // /* Handle the case where freelist is one block long */
    // if (index->next == NULL) {
    //     if (ptr > index) {
    //         return NULL;
    //     } else {
    //         return index;
    //     }
    // }
    //
    // int eol = 0;
    // int change = 0;
    // do {
    //     if (index > ptr && index <= right) {
    //         right = index;
    //         change = 1;
    //     }
    //     /* If end of free list reached, terminate looping */
    //     if (index->next == NULL) {
    //         eol = 1;
    //     } else {
    //         index = index->next;
    //     }
    // } while (!eol);
    //
    // if (!change) {
    //     return NULL;
    // } else {
    //     return right;
    // }
}

/* Iterates through freelist to find block that is left and right of the ptr.
Attempts to combine blocks if they are not in use.
Expected behavior is that the returned memory block will be re-added to the freelist before being acted upon. As a result, the returned block's prev and next references are nulled before returning.

Preconditions:
- Ptr points to the memory used by user, not the metadata.

Postconditions:
- Returned ptr points to the memory that the user uses, NOT the metadata.
 */
metadata_t* coalesceLeftAndRight(void *ptr) {
    metadata_t *head = (metadata_t*) ((char*) ptr - sizeof(metadata_t));
    metadata_t *ret = head;
    metadata_t *left = findLeftBlk(ret);
    /* If there is an available left block sitting in memory, set ret to it and update ret's size to include the left block. Remove the left block from the freelist.
    Note that findLeftBlk returns NULL if left is in use, so there is really no reason to test for it here. Consistency tho. */
    if (left != NULL && left->in_use == 0) {
        /* Absorb left's size into the returning size */
        short retSize = ret->size;
        ret = left;
        ret->size += retSize;
        /* Since you're absorbing the left, remove it from freelist */
        removeFromFreelist(left);
    }

    metadata_t *right = findRightBlk(ret);
    /* If there is an available right block sitting in memory, update ret's size to include it. Remove the right block from the freelist. */
    if (right != NULL && right->in_use == 0) {
        ret->size += right->size;
        removeFromFreelist(right);
    }
    ret->prev = NULL;
    ret->next = NULL;
    return ret;
}

/* Returns size of freelist, used for debugging. */
short getFreelistSize() {
    if (freelist == NULL) {
        return -1;
    } else {
        return freelist->size;
    }
}
