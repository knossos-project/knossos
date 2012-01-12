// The following are functions that do not need to be accessed openly.
//
// See comments in hash.c for some background on ht_hash.

static uint32_t ht_hash(Hashtable *hashtable, Coordinate key);

// These functions are used internally to deal with the doubly linked list in
// which the data is stored.
static uint32_t ht_ll_put(uint32_t place, C2D_Element *destElement, C2D_Element *newElement, C2D_Element *ht_next);
static uint32_t ht_ll_del(C2D_Element *delElement);
static uint32_t ht_ll_rmlist(C2D_Element *delElement);
