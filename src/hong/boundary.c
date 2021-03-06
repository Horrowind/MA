#include <stdio.h>

#include "datasizes.h"
#include "boundary.h"
#include "params.h"

#define bitsof(type) (sizeof(type) * 8)

static inline
boundary_t boundary_rotl(boundary_t boundary, int shift) {
    boundary.bits =
        (((boundary.bits << (shift * BITS)) & ((((boundary_bits_t)1) << (boundary.size * BITS)) - ((boundary_bits_t)1))) |
         (boundary.bits >> ((boundary.size - shift)  * BITS)));
    return boundary;
}

static inline
boundary_t boundary_rotr(boundary_t boundary, int shift) {
    boundary.bits = (((boundary.bits >> (shift * BITS)) |
                      (boundary.bits << ((boundary.size - shift) * BITS))) & ((((boundary_bits_t)1) << (boundary.size * BITS)) - 1));
    return boundary;
}


#if 0
#include <stdlib.h>
static
void boundary_check(boundary_t boundary) {
    if(boundary.bits != 0 && bitsof(boundary_bits_t) - __builtin_clzll(boundary.bits) > boundary.size * BITS) {
        printf("Boundary ");
        boundary_write(boundary);
        printf(" is larger then size %i\n", boundary.size);
        exit(0);
    }
}
#else
static
void boundary_check(boundary_t boundary) { }
#endif

static inline
boundary_t boundary_normalize(boundary_t boundary) {
    boundary_t max = boundary;
    for(int i = 0; i < boundary.size; i++) {
        boundary = boundary_rotl(boundary, 1);
        if(max.bits < boundary.bits) max = boundary;
    }
    return max;
}


static
void boundary_write(boundary_t boundary) {
    printf("[%02i] ", boundary.size);
    for(int i = bitsof(boundary_bits_t) / BITS - 1; i >= 0; i--) {
        if(i < boundary.size) {
            printf("%i", (int)((boundary.bits >> (i * BITS)) & LAST_NODE_MASK));
        } else {
            printf(" ");
        }
    }
}

static
boundary_t boundary_unfold(boundary_t boundary, int n) {
    boundary_t result = boundary;
    result.size = boundary.size * n;
    for(int i = 1; i < n; i++) {
        result.bits <<= boundary.size * BITS;
        result.bits  |= boundary.bits;
    }
    return result;
}

static inline
boundary_t boundary_insert(boundary_t boundary, int ngon, int allow_overlap) {
    boundary_t result = {.bits = 0, .size = 0};
    if((boundary.bits >> (BITS * (boundary.size - 1))) < VALENCE - 2) {
        u32 s = boundary_bits_get_trailing_max_nodes(bits);
        if(s != boundary.size - 1) {
            if(s <= ngon - 2) {
		result = boundary;
                result.bits  += (((boundary_bits_t)1) << ((boundary.size - 1) * BITS));
                result.bits >>= s * BITS;
                result.bits  += ((boundary_bits_t)1);
                result.bits <<= (ngon - 2 - s) * BITS;
                result.size   = boundary.size + ngon - 2 - 2 * s;
            }
        } else if(allow_overlap) {
            if((boundary.bits >> (BITS * (boundary.size - 1))) < VALENCE - 3) {
                if(boundary.size + 1 <= ngon) {
                    result.bits   = (boundary.bits >> (BITS * (boundary.size - 1))) + 2;
                    result.bits <<= (ngon - boundary.size - 1) * BITS;
                    result.size   = ngon - boundary.size;
                }
            } else {
                if(boundary.size + 3 <= ngon) {
                    result.bits   = ((boundary_bits_t)1) << (ngon - boundary.size - 3) * BITS;
                    result.size   = ngon - boundary.size - 2;
                }
            }
        }        
    }
    return result;
}



static inline
boundary_t boundary_remove(boundary_t boundary, int ngon, int allow_overlap) {
    boundary_t result = {.bits = 0, .size = 0};
    if((boundary.bits >> (BITS * (boundary.size - 1))) != 0) {
        u32 s = boundary_bits_ctz(boundary.bits) / BITS;
        if(s != boundary.size - 1) {
            if(s <= ngon - 2) {
		result = boundary;
                result.bits  -= (((boundary_bits_t)1) << ((boundary.size - 1) * BITS));
                result.bits >>= s * BITS;
                result.bits  -= ((boundary_bits_t)1);
                result.bits <<= (ngon - 2 - s) * BITS;
                result.bits  |= (((((boundary_bits_t)1) << (ngon - 2 - s) * BITS) - 1) / LAST_NODE_MASK) * (VALENCE - 2);
                result.size   = boundary.size + ngon - 2 - 2 * s;
            }
        } else if(allow_overlap) { // Overlapping case
            if((boundary.bits >> (BITS * (boundary.size - 1))) < VALENCE - 3) { // Overlapping with a single node
                if(boundary.size + 1 <= ngon) {
                    result.bits   = (boundary.bits >> (BITS * (boundary.size - 1))) - 2;
                    result.bits <<= (ngon - boundary.size - 1) * BITS;
                    result.bits  |= ((((boundary_bits_t)1) << ((ngon - boundary.size - 1) * BITS)) - 1) / LAST_NODE_MASK * (VALENCE - 2);
                    result.size   = ngon - boundary.size;
                }
            } else { // Overlapping with a single edge
                if(boundary.size + 3 <= ngon) { 
                    result.bits   = (VALENCE - 3) << ((ngon - boundary.size - 3) * BITS);
                    result.bits  |= (((((boundary_bits_t)1) << ((ngon - boundary.size - 3) * BITS)) - 1) / LAST_NODE_MASK) * (VALENCE - 2);
                    result.size       = ngon - boundary.size - 2;
                }
            }
        }        
    }
    return result;   
}

static
char boundary_is_mouse(boundary_t boundary) {
    // A mouse boundary complex has odd length and the head has valence 1
    if(boundary.size % 2 == 1 && (boundary.bits & LAST_NODE_MASK) == 0) {
	boundary_bits_t v = boundary.bits >> BITS;
	int s = boundary.size - 1;
	for(int i = 0; i < s / 2; i++) {
	    if(((v >> (i * BITS)) & LAST_NODE_MASK) + ((v >> ((s - 1 - i) * BITS)) & LAST_NODE_MASK) != VALENCE - 2) {
		return 0;
	    }
	}
	return 1;
    }
    return 0;
}
