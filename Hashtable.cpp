#include "knossos-global.h"

//Knossos 3.x static functions
static uint32_t ht_ll_rmlist(C2D_Element *delElement) {
    C2D_Element *baseElement, *forwardElement;

    baseElement = delElement;

    do {
#ifdef DEBUG
        printf("Freeing delElement in a sec.\n");
#endif
        forwardElement = delElement->next;
        free(delElement);
        delElement = forwardElement;
    } while(forwardElement != baseElement);

    return LL_SUCCESS;
}

static uint32_t ht_hash(Hashtable *hashtable, Coordinate key) {
    // This is a hash function optimized for keys of multiples of 3 bytes.
    // It is adapted from code by Robert Oestling at <http://www.robos.org>
    // which is adapted from the original (public domain) implementation at
    // <http://www.burtleburtle.net/bob/c/lookup3.c>.


    /* Coordinate is int32 not uint32 not sure whether this is safe here...
     * JK August 2012 */
    uint32_t a, b, c;

    a = b = c = 0xC00FFEEE;

    c += key.z;
    b += key.x;
    a += key.y;

    c ^= b; c -= ROL(b, 14);
    a ^= c; a -= ROL(c, 11);
    b ^= a; b -= ROL(a, 25);
    c ^= b; c -= ROL(b, 16);
    a ^= c; a -= ROL(c, 4);
    b ^= a; b -= ROL(a, 14);
    c ^= b; c -= ROL(b, 24);

    // c = c % hashtable->tablesize if hashtable->tablesize is a power of
    // 2.
    c = MODULO_POW2(c, hashtable->tablesize);

    return c;
}

static uint32_t ht_ll_put(uint32_t place, C2D_Element *destElement, C2D_Element *newElement, C2D_Element *ht_next) {
    if(place == LL_AFTER) {
        destElement->next->previous = newElement;
        newElement->next = destElement->next;
        newElement->previous = destElement;
        destElement->next = newElement;
    }
    else if(place == LL_BEFORE) {
        destElement->previous->next = newElement;
        newElement->previous = destElement->previous;
        destElement->previous = newElement;
        newElement->next = destElement;
    }
    else {
        // Something is wrong. We might want to define other flags in the
        // future, though.

        return LL_FAILURE;
    }

    newElement->ht_next = ht_next;

    return LL_SUCCESS;
}

static uint32_t ht_ll_del(C2D_Element *delElement) {
    delElement->previous->next = delElement->next;
    delElement->next->previous = delElement->previous;
    if(delElement->previous->ht_next) {
        // This is the case when the previous element is part of the
        // same chain. If it isn't, we shouldn't touch the ht_next
        // pointer.

        delElement->previous->ht_next = delElement->ht_next;
    }

    free(delElement);
    return LL_SUCCESS;
}

//new static functions
Hashtable *Hashtable::ht_new(uint32_t tablesize) {
    // A Hashtable is defined by a struct of an array of pointers (Table,
    // the hashtable per se) and a pointer to an element (ListEntry, an
    // element in the linked list)

    // Because of the way the hash function calculates modulo, we can only
    // accept powers of 2 as table size.
    // The following code calculates the next power of two if tablesize != 2^n,
    // and doesn't change anything else.

    if(tablesize > 0x80000000)
        return HT_FAILURE;

    tablesize = nextpow2(tablesize - 1);

    new_ht = malloc(sizeof(Hashtable));
    if(new_ht == NULL) {
        printf("Out of memory\n");
        return HT_FAILURE;
    }
    new_ht->table = malloc(tablesize * sizeof(C2D_Element *));
    if(new_ht->table == NULL) {
        printf("Out of memory\n");
        return HT_FAILURE;
    }
    new_ht->listEntry = malloc(sizeof(C2D_Element));
    if(new_ht->listEntry == NULL) {
        printf("Out of memory\n");
        return HT_FAILURE;
    }

    // Set up the structure.

    memset(new_ht->table, 0, tablesize * sizeof(C2D_Element *));
    new_ht->tablesize = tablesize;
    new_ht->listEntry->previous = new_ht->listEntry;
    new_ht->listEntry->next = new_ht->listEntry;

    // ht_next is used for chaining, i.e. collision resolution in the
    // hashtable.

    new_ht->listEntry->ht_next = NULL;

    // This is a dummy element used only for entry into the linked list.

    new_ht->listEntry->datacube = NULL;
    SET_COORDINATE(new_ht->listEntry->coordinate, -1, -1, -1);

    return new_ht;
}

uint32_t Hashtable::ht_del(Hashtable *hashtable, Coordinate key) {
    uint32_t hashIndex;
    C2D_Element *curElement = NULL;

    hashIndex = ht_hash(hashtable, key);
    curElement = hashtable->table[hashIndex];

    while(curElement) {
        if(COMPARE_COORDINATE(key, curElement->coordinate)) {
            // We have the element that we want! Delete it.
            if(!curElement->previous->ht_next) {
                 // This is the first element in the chain, so
                 // we need to update the hash table entry.
                 hashtable->table[hashIndex] = curElement->ht_next;
            }
            ht_ll_del(curElement);

            return HT_SUCCESS;
        }

        curElement = curElement->ht_next;
    }

    // We didn't find the element in the chain for the hash, so
    // it's not in the structure and we can't delete anything.

    return HT_FAILURE;
}

Byte *Hashtable::ht_get(Hashtable *hashtable, Coordinate key) {
    uint32_t hashIndex;
    C2D_Element *curElement;

    hashIndex = ht_hash(hashtable, key);
    curElement = hashtable->table[hashIndex];

    while(curElement) {
        if(COMPARE_COORDINATE(key, curElement->coordinate)) {
            // This is the element we're looking for!
            return curElement->datacube;
        }
        curElement = curElement->ht_next;
    }

    // We don't have that element.
    return HT_FAILURE;
}

uint32_t Hashtable::ht_put(Hashtable *hashtable, Coordinate key, Byte *value) {
    uint32_t hashIndex;
    C2D_Element *curElement, *chainElement, *putElement;

    putElement = malloc(sizeof(C2D_Element));
    if(putElement == NULL) {
        printf("Out of memory\n");
        return HT_FAILURE;
    }
    putElement->coordinate = key;
    putElement->datacube = value;

    // Compute the hash for the key and retrieve the corresponding entry
    // from the hashtable.

    hashIndex = ht_hash(hashtable, key);
    curElement = hashtable->table[hashIndex];

    if(curElement) {

        // There already is an entry with the same hash.

        chainElement = curElement;

        // Go through the chain to see if the key is the same, too.

        do {
            if(COMPARE_COORDINATE(curElement->coordinate, putElement->coordinate)) {
                // We already have an element for that key.
                // Update the value, only.

                curElement->datacube = putElement->datacube;

                free(putElement);
                return HT_SUCCESS;
            }
        } while((curElement = curElement->ht_next));

        // We don't have an element with that key, yet. All the
        // elements we went through were random collisions.
        // Add the element into the chain, as the first element in the
        // chain.

        hashtable->table[hashIndex] = putElement;
        ht_ll_put(LL_BEFORE, chainElement, putElement, chainElement);

        return HT_SUCCESS;
    }
    else {
        // Nothing with that hash in the table, yet.

        hashtable->table[hashIndex] = putElement;
        ht_ll_put(LL_BEFORE, hashtable->listEntry, putElement, NULL);

        return HT_SUCCESS;
    }
}

uint32_t Hashtable::ht_rmtable(Hashtable *hashtable) {

    // Free the entire linked list,
    // free the hash table itself,
    // free the hash table strucure.

    ht_ll_rmlist(hashtable->listEntry);
    free(hashtable->table);
    free(hashtable);

    return HT_SUCCESS;
}
uint32_t Hashtable::ht_union(Hashtable *target, Hashtable *h1, Hashtable *h2) {
    C2D_Element *cur = NULL, *next = NULL;

    cur = h1->listEntry->next;
    while(cur != h1->listEntry) {
        next = cur->next;
        ht_put(target, cur->coordinate, NULL);
        cur = next;
    }

    cur = h2->listEntry->next;
    while(cur != h2->listEntry) {
        next = cur->next;
        ht_put(target, cur->coordinate, NULL);
        cur = next;
    }

    return HT_SUCCESS;
}

uint32_t Hashtable::nextpow2(uint32_t a) {
    /*
     * Compute the next power of two by copying the highest set bit to all the
     * lower bits until 2^n - 1 is reached, then increment by one.
     * This works FOR UNSIGNED 32-BIT INT ONLY.
     *
     */

    if(a == 0)
        return 0;

    a |= (a >> 1);
    a |= (a >> 2);
    a |= (a >> 4);
    a |= (a >> 8);
    a |= (a >> 16);
    a++;

    return a;
}

uint32_t Hashtable::lastpow2(uint32_t a) {
    /*
     * Works very similarly to nextpow2().
     */

    if(a == 0)
        return 0;

    a |= (a >> 1);
    a |= (a >> 2);
    a |= (a >> 4);
    a |= (a >> 8);
    a |= (a >> 16);
    a &= (a >> 1);
    a++;

    return a;
}
