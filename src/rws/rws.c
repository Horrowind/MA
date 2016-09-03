#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int b32;
typedef int i32;
typedef unsigned int u32;
typedef long i64;
typedef unsigned long u64;
typedef float f32;
typedef double f64;

typedef unsigned int uint;

#define MAX_WORD_LENGTH 16
#define MAX_LIST_LENGTH 4096

#define min(x, y) ((x) < (y) ? (x) : (y))

typedef struct {
    char data[MAX_WORD_LENGTH];
    short length;
} word_t;

typedef struct {
    char data[MAX_WORD_LENGTH];
    short length;
} cyc_word_t;

typedef struct {
    word_t l;
    word_t r;
    u8 marker;
} identity_t;

typedef struct {
    cyc_word_t l;
    cyc_word_t r;
    u8 marker;
} cyc_identity_t;


word_t word_from_c_string(char* str) {
    word_t result = { 0 };
    while(*str != '\0') {
        assert(result.length <= MAX_WORD_LENGTH);
        result.data[result.length] = *str;
        result.length++;
        str++;
    }
    return result;
}

cyc_word_t cyc_word_rotate(cyc_word_t cyc_word) {
    if(cyc_word.length > 0) {
        char head = cyc_word.data[0];
        for(int i = 1; i < cyc_word.length; i++) {
            cyc_word.data[i - 1] = cyc_word.data[i];
        }
        cyc_word.data[cyc_word.length - 1] = head;
        return cyc_word;
    }
}

int word_eq(word_t w1, word_t w2) {
    int result = 1;
    if(w1.length != w2.length) {
        result = 0;
    } else {
        for(int i = 0; i < w1.length; i++) {
            if(w1.data[i] != w2.data[i]) result = 0;
        }
    }
    return result;
}

int identity_eq(identity_t id1, identity_t id2) {
    return word_eq(id1.l, id2.l) && word_eq(id1.r, id2.r);
}

int cyc_word_eq(cyc_word_t w1, cyc_word_t w2) {
    int result = 1;
    if(w1.length != w2.length) {
        result = 0;
    } else {
        for(int i = 0; i < w1.length; i++) {
            int result = 1;
            for(int j = 0; j < w1.length; j++) {
                if(w1.data[(i + j) % w1.length] != w2.data[j]) {
                    result = 0;
                    break;
                }
            }
            if(result) {
                return result;
            }
        }
    }
    return result;
}

int cyc_identity_eq(cyc_identity_t id1, cyc_identity_t id2) {
    return cyc_word_eq(id1.l, id2.l) && cyc_word_eq(id1.r, id2.r);
}

identity_t identity_canonical(identity_t id) {
    if(id.l.length != id.r.length) {
        if(id.l.length < id.r.length) {
            return (identity_t){ .l = id.r, .r = id.l };
        } else {
            return id;
        }
    } else {
        int length = id.l.length;
        for(int i = 0; i < length; i++) {
            if(id.l.data[i] < id.r.data[i]) {
                return (identity_t){ .l = id.r, .r = id.l };
            }
            if(id.l.data[i] > id.r.data[i]) {
                return id;
            }
        }
        return id;
    }
}

typedef struct {
    identity_t* data;
    uint size;
    uint allocated_size;
} identity_set_t;

void identity_set_init(identity_set_t* set, uint allocated_size) {
    set->data = malloc(sizeof(identity_t) * allocated_size);
    set->size = 0;
    set->allocated_size = allocated_size;
}

void identity_set_deinit(identity_set_t* set) {
    free(set->data);
}

#define set_foreach(set, it)                 \
    for(typeof((set).data) it = (set).data;  \
        it < &(set).data[(set).size];        \
        it++) 

void identity_set_insert(identity_set_t* set, identity_t id) {
    b32 is_found = 0;
    set_foreach(*set, id_ptr) {
        if(identity_eq(id, *id_ptr)) {
            is_found = 1;
            break;
        }
    }
    if(!is_found) {
        if(set->size == set->allocated_size) {
            set->allocated_size *= 2;
            set->data = realloc(set->data, sizeof(identity_t) * set->allocated_size);
            assert(set->data);
        }
        set->data[set->size] = id;
        set->size++;
    }
}

void identity_set_remove(identity_set_t* set, identity_t id) {
    identity_t* found_ptr = NULL;
    set_foreach(*set, id_ptr) {
        if(identity_eq(id, *id_ptr)) {
            found_ptr = id_ptr;
            break;
        }
    }
    if(found_ptr) {
        *found_ptr = set->data[set->size - 1];
        set->size--;
    }
}

typedef struct {
    cyc_identity_t* data;
    uint size;
    uint allocated_size;
} cyc_identity_set_t;

void cyc_identity_set_init(cyc_identity_set_t* set, uint allocated_size) {
    set->data = malloc(sizeof(cyc_identity_t) * allocated_size);
    set->size = 0;
    set->allocated_size = allocated_size;
}

void cyc_identity_set_deinit(cyc_identity_set_t* set) {
    free(set->data);
}

#define cyc_set_foreach(set, it)             \
    for(typeof((set).data) it = (set).data;  \
        it < &(set).data[(set).size];        \
        it++) 

void cyc_identity_set_insert(cyc_identity_set_t* set, cyc_identity_t id) {
    b32 is_found = 0;
    cyc_set_foreach(*set, id_ptr) {
        if(cyc_identity_eq(id, *id_ptr)) {
            is_found = 1;
            break;
        }
    }
    if(!is_found) {
        if(set->size == set->allocated_size) {
            set->allocated_size *= 2;
            set->data = realloc(set->data, sizeof(cyc_identity_t) * set->allocated_size);
            assert(set->data);
        }
        set->data[set->size] = id;
        set->size++;
    }
}

void cyc_identity_set_remove(cyc_identity_set_t* set, cyc_identity_t id) {
    cyc_identity_t* found_ptr = NULL;
    set_foreach(*set, id_ptr) {
        if(cyc_identity_eq(id, *id_ptr)) {
            found_ptr = id_ptr;
            break;
        }
    }
    if(found_ptr) {
        *found_ptr = set->data[set->size - 1];
        set->size--;
    }
}



typedef struct {
    identity_set_t rules;
    identity_set_t eqs;
    identity_set_t ids;

    cyc_identity_set_t cyc_rules;
    cyc_identity_set_t cyc_eqs;
    cyc_identity_set_t cyc_ids;
} rewriting_system_t;

void rewriting_system_init(rewriting_system_t* rws) {
    identity_set_init(&rws->rules, 64);
    identity_set_init(&rws->eqs, 64);
    identity_set_init(&rws->ids, 64);

    cyc_identity_set_init(&rws->cyc_rules, 64);
    cyc_identity_set_init(&rws->cyc_eqs, 64);
    cyc_identity_set_init(&rws->cyc_ids, 64);
}

b32 word_reduce(word_t* word, identity_t rule) {
    b32 is_equal = 0;
    for(int i = 0; i < word->length; i++) {
        if(i + rule.l.length <= word->length) {
            is_equal = 1;
            for(int j = 0; j < rule.l.length; j++) {
                if(word->data[i + j] != rule.l.data[j]) {
                    is_equal = 0;
                    break;
                }
            }
            if(is_equal) {
                assert(word->length + rule.r.length - rule.l.length < MAX_WORD_LENGTH);
                int diff = rule.l.length - rule.r.length;
                if(diff < 0) {
                    for(int k = word->length - 1;
                        k >= rule.r.length + i;
                        k--) {
                        word->data[k] = word->data[k + diff];
                    }
                } else if(diff > 0) {
                    for(int k = rule.r.length + i; 
                        k < word->length + rule.r.length - rule.l.length;
                        k++) {
                        word->data[k] = word->data[k + diff];
                    }
                }
                for(int k = 0; k < rule.r.length; k++) {
                    word->data[i + k] = rule.r.data[k];
                }
                word->length = word->length + rule.r.length - rule.l.length;
                break;
            }
        }
    }
    return is_equal;
}

b32 cyc_word_reduce(cyc_word_t* cyc_word, identity_t rule) {
    for(int i = 0; i < cyc_word->length; i++) {
        if(word_reduce((word_t*)cyc_word, rule)) {
            return 1;
        }
        *cyc_word = cyc_word_rotate(*cyc_word);
    }
    return 0;
}

b32 cyc_word_reduce2(cyc_word_t* cyc_word, cyc_identity_t cyc_rule) {
    if(cyc_word_eq(*cyc_word, cyc_rule.l)) {
        *cyc_word = cyc_rule.r;
        return 1;
    }
    return 0;
}

word_t word_normal_form(word_t word, identity_set_t rules) {
    b32 could_be_reduced = 1;
    while(could_be_reduced == 1) {
        could_be_reduced = 0;
        set_foreach(rules, rule) {
            if(word_reduce(&word, *rule)) {
                could_be_reduced = 1;
                break;
            }
        }
        
    }
    return word;
}

cyc_word_t cyc_word_normal_form(cyc_word_t cyc_word, identity_set_t rules, cyc_identity_set_t cyc_rules) {
    b32 could_be_reduced = 1;
    while(could_be_reduced == 1) {
        could_be_reduced = 0;
        set_foreach(rules, rule) {
            if(cyc_word_reduce(&cyc_word, *rule)) {
                could_be_reduced = 1;
                break;
            }
        }
        cyc_set_foreach(cyc_rules, cyc_rule) {
            if(cyc_word_reduce2(&cyc_word, *cyc_rule)) {
                could_be_reduced = 1;
                break;
            }
        }
        
    }
    return cyc_word;
}

identity_set_t tmp_word_list;

b32 word_equivalence(word_t w1, word_t w2, identity_set_t eqs) {
    struct {
        word_t words[4096];
        int length;
    } tmp_word_list;
    
    tmp_word_list.words[0] = w1;
    tmp_word_list.length = 1;
    if(word_eq(w1, w2)) return 1;
    for(int i = 0; i < tmp_word_list.length; i++) {
        set_foreach(eqs, eq) {
            assert(eq->l.length == eq->r.length);
            identity_t eq_as_rule = *eq;
            for(int do_twice = 0; do_twice < 2; do_twice++) {
                for(int j = 0; j < tmp_word_list.words[i].length - eq_as_rule.l.length + 1; j++) {
                    word_t reduced_word = tmp_word_list.words[i];
                    int can_be_reduced = 1;
                    for(int k = 0; k < eq_as_rule.l.length; k++) {
                        if(reduced_word.data[j + k] != eq_as_rule.l.data[k]) {
                            can_be_reduced = 0;
                            break;
                        }
                    }
                    if(can_be_reduced) {
                        for(int k = 0; k < eq_as_rule.r.length; k++) {
                            reduced_word.data[j + k] = eq_as_rule.r.data[k];
                        }
                        b32 is_already_found = 0;
                        for(int k = 0; k < tmp_word_list.length; k++) {
                            if(word_eq(reduced_word, tmp_word_list.words[k])) {
                                is_already_found = 1;
                                break;
                            }
                        }
                        if(!is_already_found) {
                            if(word_eq(reduced_word, w2)) return 1;
                            assert(tmp_word_list.length <= sizeof(tmp_word_list.words) / sizeof(word_t));
                            tmp_word_list.words[tmp_word_list.length] = reduced_word;
                            tmp_word_list.length++;
                        }
                    }
                }
                eq_as_rule = (identity_t){ .l = eq->r, .r = eq->l };
            }
        }
    }
    return 0;
}

b32 cyc_word_equivalence(cyc_word_t w1, cyc_word_t w2, identity_set_t eqs, cyc_identity_set_t cyc_eqs) {
    struct {
        cyc_word_t cyc_words[4096];
        int length;
    } tmp_cyc_word_list;
    
    tmp_cyc_word_list.cyc_words[0] = w1;
    tmp_cyc_word_list.length = 1;
    if(cyc_word_eq(w1, w2)) return 1;
    for(int i = 0; i < tmp_cyc_word_list.length; i++) {
        cyc_set_foreach(eqs, eq) {
            assert(eq->l.length == eq->r.length);
            cyc_identity_t eq_as_rule = *eq;
            for(int do_twice = 0; do_twice < 2; do_twice++) {
                for(int j = 0; j < tmp_cyc_word_list.cyc_words[i].length - eq_as_rule.l.length + 1; j++) {
                    cyc_word_t reduced_word = tmp_cyc_word_list.cyc_words[i];
                    int can_be_reduced = 1;
                    for(int k = 0; k < eq_as_rule.l.length; k++) {
                        if(reduced_word.data[j + k] != eq_as_rule.l.data[k]) {
                            can_be_reduced = 0;
                            break;
                        }
                    }
                    if(can_be_reduced) {
                        for(int k = 0; k < eq_as_rule.r.length; k++) {
                            reduced_word.data[j + k] = eq_as_rule.r.data[k];
                        }
                        b32 is_already_found = 0;
                        for(int k = 0; k < tmp_cyc_word_list.length; k++) {
                            if(cyc_word_eq(reduced_word, tmp_cyc_word_list.cyc_words[k])) {
                                is_already_found = 1;
                                break;
                            }
                        }
                        if(!is_already_found) {
                            if(cyc_word_eq(reduced_word, w2)) return 1;
                            assert(tmp_cyc_word_list.length <= sizeof(tmp_cyc_word_list.cyc_words) / sizeof(cyc_word_t));
                            tmp_cyc_word_list.cyc_words[tmp_cyc_word_list.length] = reduced_word;
                            tmp_cyc_word_list.length++;
                        }
                    }
                }
                eq_as_rule = (cyc_identity_t){ .l = eq->r, .r = eq->l };
            }
        }
        cyc_word_t cyc_word_rotation = tmp_cyc_word_list.cyc_words[i];
        for(int j = 1; j < cyc_word_rotation.length; j++) {
            cyc_word_rotation = cyc_word_rotate(cyc_word_rotation);
            b32 is_already_found = 0;
            for(int k = 0; k < tmp_cyc_word_list.length; k++) {
                if(cyc_word_eq(cyc_word_rotation, tmp_cyc_word_list.cyc_words[k])) {
                    is_already_found = 1;
                    break;
                }
            }
            if(!is_already_found) {
                assert(tmp_cyc_word_list.length <= sizeof(tmp_cyc_word_list.cyc_words) / sizeof(cyc_word_t));
                tmp_cyc_word_list.cyc_words[tmp_cyc_word_list.length] = cyc_word_rotation;
                tmp_cyc_word_list.length++;
            }
        }

    }
    return 0;
}


void rewriting_system_print(rewriting_system_t rws) {
    printf("Rewrite system:\n");
    set_foreach(rws.rules, rule) {
        printf("%.*s -> %.*s\n", rule->l.length, rule->l.data, rule->r.length, rule->r.data);
    }
    set_foreach(rws.eqs, eq) {
        printf("%.*s = %.*s\n", eq->l.length, eq->l.data, eq->r.length, eq->r.data);
    }
    set_foreach(rws.ids, id) {
        printf("%.*s ? %.*s\n", id->l.length, id->l.data, id->r.length, id->r.data);
    }
    cyc_set_foreach(rws.cyc_rules, rule) {
        printf("(%.*s) -> (%.*s)\n", rule->l.length, rule->l.data, rule->r.length, rule->r.data);
    }
    cyc_set_foreach(rws.cyc_eqs, eq) {
        printf("(%.*s) = (%.*s)\n", eq->l.length, eq->l.data, eq->r.length, eq->r.data);
    }
    cyc_set_foreach(rws.cyc_ids, id) {
        printf("(%.*s) ? (%.*s)\n", id->l.length, id->l.data, id->r.length, id->r.data);
    }
}

void identity_print(identity_t id) {
    printf(" \"%.*s\" ? \"%.*s\"\n", id.l.length, id.l.data, id.r.length, id.r.data);
}

identity_t identity_gen(char* l, char* r) {
    identity_t result = { 0 };
    result.l = word_from_c_string(l);
    result.r = word_from_c_string(r);
    return identity_canonical(result);
}

void insert_all_critical_pairs_into_map(identity_t rule1, identity_t rule2, identity_set_t* map) {
    // Case rule1 = ABC -> D, rule2 = B -> E => D = AEC
    {
        for(int i = 0; i < rule1.l.length - rule2.l.length + 1; i++) {
            b32 is_equal = 1;
            for(int j = 0; j < rule2.l.length; j++) {
                if(rule1.l.data[i + j] != rule2.l.data[j]) {
                    is_equal = 0;
                    break;
                }
            }
            if(is_equal) {
                identity_t new_id = { .l = rule1.l, .r = rule1.r };
                new_id.l.length = rule1.l.length + rule2.r.length - rule2.l.length;
                assert(new_id.l.length < MAX_WORD_LENGTH);
                int diff = rule2.l.length - rule2.r.length;
                if(diff < 0) {
                    for(int k = rule1.l.length - 1; k >= rule2.r.length + i; k--) {
                        new_id.l.data[k] = rule1.l.data[k + diff];
                    }
                } else if(diff > 0) {
                    for(int k = rule2.r.length + i; k < rule1.l.length + rule2.r.length - rule2.l.length; k++) {
                        new_id.l.data[k] = rule1.l.data[k + diff];
                    }
                }
                for(int k = 0; k < rule2.r.length; k++) {
                    new_id.l.data[i + k] = rule2.r.data[k];
                }
                new_id.l.length = rule1.l.length + rule2.r.length - rule2.l.length;
                new_id = identity_canonical(new_id);
                identity_set_insert(map, new_id);
            }
        }
    }
    // Case rule1 = AB -> D, rule2 = BC -> E => DC = AE
    {
        int start = rule1.l.length - rule2.l.length < 0 ? 0 : rule1.l.length - rule2.l.length;
        for(int i = start; i < rule1.l.length; i++) {
            b32 is_equal = 1;
            for(int j = i; j < rule1.l.length; j++) {
                if(rule1.l.data[j] != rule2.l.data[j - i]) {
                    is_equal = 0;
                    break;
                }
            }
            if(is_equal) {
                int length_a = i;
                int length_b = rule1.l.length - length_a;
                identity_t new_id;
                new_id.l = rule1.r;
                new_id.r = rule1.l;
                new_id.l.length = rule1.r.length + rule2.l.length - length_b;
                assert(new_id.l.length < MAX_WORD_LENGTH);
                new_id.r.length = rule1.l.length + rule2.r.length - length_b;
                assert(new_id.r.length < MAX_WORD_LENGTH);
                for(int j = 0; j < rule2.l.length - length_b; j++) {
                    new_id.l.data[rule1.r.length + j] = rule2.l.data[length_b + j];
                }
                for(int j = 0; j < rule2.r.length; j++) {
                    new_id.r.data[length_a + j] = rule2.r.data[j];
                }
                new_id = identity_canonical(new_id);
                identity_set_insert(map, new_id);
            }
        }
    }
}

void insert_all_cyc_critical_pairs_into_map1(identity_t rule1, identity_t rule2, cyc_identity_set_t* map) {
    // rule1 = ABC -> E, rule2 = CDA -> F => (ED) = (FB)
    int min_length = rule1.l.length < rule2.l.length ? rule1.l.length : rule2.l.length;
    for(int i = 1; i < min_length; i++) {
        for(int j = 1; i + j < min_length + 1; j++) {
            b32 is_equal = 1;
            for(int k = 0; k < i; k++) {
                if(rule1.l.data[k] != rule2.l.data[rule2.l.length - i + k]) {
                    is_equal = 0;
                    break;
                }
            }
            for(int k = 0; k < j; k++) {
                if(rule1.l.data[rule1.l.length - j + k] != rule2.l.data[k]) {
                    is_equal = 0;
                    break;
                }
            }
            if(is_equal) {
                cyc_identity_t new_identity;
                for(int k = 0; k < rule1.r.length; k++) {
                    new_identity.l.data[k] = rule1.r.data[k]; // E
                }
                for(int k = 0; k < rule2.l.length - i - j; k++) {
                    new_identity.l.data[k + rule1.r.length] = rule2.l.data[k + j]; // D
                }
                new_identity.l.length = rule1.r.length + rule2.l.length - i - j;
                
                for(int k = 0; k < rule2.r.length; k++) {
                    new_identity.r.data[k] = rule2.r.data[k]; // F
                }
                for(int k = 0; k < rule1.l.length - i - j; k++) {
                    new_identity.r.data[k + rule2.r.length] = rule1.l.data[k + i]; // B
                }
                new_identity.r.length = rule2.r.length + rule1.l.length - i - j;
                printf("%2i %2i ", i, j);
                printf("%.*s ? %.*s + ", rule1.l.length, rule1.l.data, rule1.r.length, rule1.r.data);
                printf("%.*s ? %.*s = ", rule2.l.length, rule2.l.data, rule2.r.length, rule2.r.data);
                printf("(%.*s) ? (%.*s)\n", new_identity.l.length, new_identity.l.data, new_identity.r.length, new_identity.r.data);
                cyc_identity_set_insert(map, new_identity);
            }
        }
    }
}

void insert_all_cyc_critical_pairs_into_map2(cyc_identity_t rule1, identity_t rule2, cyc_identity_set_t* map) {
    if(rule1.l.length < rule2.l.length) return;
    for(int i = 0; i < rule1.l.length; i++) {
        b32 is_equal = 1;
        for(int j = 0; j < rule2.l.length; j++) {
            if(rule1.l.data[j] != rule2.l.data[j]) {
                is_equal = 0;
                break;
            }
        }
        if(is_equal) {
            cyc_identity_t new_rule = {
                .l = rule1.r,
                .r.length = rule2.r.length
            };
            for(int j = 0; j < rule2.r.length; j++) {
                new_rule.r.data[j] = rule2.r.data[j];
            }
            for(int j = 0; j < rule1.l.length - rule2.l.length; j++) {
                new_rule.r.data[j + rule2.r.length] = rule1.l.data[j + rule2.l.length];
            }
            new_rule.r.length += rule1.l.length - rule2.l.length;
            cyc_identity_set_insert(map, new_rule);
        }
        rule1.l = cyc_word_rotate(rule1.l);
    }
}

void insert_all_cyc_critical_pairs_into_map3(cyc_identity_t rule1, cyc_identity_t rule2, cyc_identity_set_t* map) {
    if(cyc_word_eq(rule1.l, rule2.l)) {
        cyc_identity_t new_rule = { .l = rule1.r, .r = rule2.r };
        cyc_identity_set_insert(map, new_rule);
    }
}

void generate_ngon_identities(identity_set_t* map, cyc_identity_set_t* cyc_map, int n, int valence) {
    identity_t id;
    id.l.data[0] = 'a';
    id.r.data[0] = 'b';
    
    for(int i = 0; i < n - 1; i++) {
        for(int j = 0; j < n - 1; j++) {
            if(j < i) {
                id.l.data[j + 1] = 'b';
            } else {
                id.r.data[j - i + 1] = 'a';
            }
        }
        id.l.data[i + 1] = 'a';
        id.r.data[n - i - 1] = 'b';
        id.l.length = i + 2;
        id.r.length = n - i;
        /* printf("%.*s = %.*s\n", id.l.length, id.l.data, id.r.length, id.r.data); */
        identity_set_insert(map, id);
    }
}


int is_confluent(rewriting_system_t rws) {
    identity_set_t ids;
    identity_set_init(&ids, 64);
    cyc_identity_set_t cyc_ids;
    cyc_identity_set_init(&cyc_ids, 64);

    set_foreach(rws.rules, rule1) {
        set_foreach(rws.rules, rule2) {
            insert_all_critical_pairs_into_map(*rule1, *rule2, &ids);
            insert_all_critical_pairs_into_map(*rule2, *rule1, &ids);
            insert_all_cyc_critical_pairs_into_map1(*rule2, *rule1, &cyc_ids);
            insert_all_cyc_critical_pairs_into_map1(*rule2, *rule1, &cyc_ids);
        }
        
        set_foreach(rws.cyc_rules, rule2) {
            insert_all_cyc_critical_pairs_into_map2(*rule2, *rule1, &cyc_ids);
        }

        set_foreach(rws.eqs, rule2) {
            insert_all_critical_pairs_into_map(*rule1, *rule2, &ids);
            insert_all_critical_pairs_into_map(*rule2, *rule1, &ids);
            insert_all_cyc_critical_pairs_into_map1(*rule2, *rule1, &cyc_ids);
            insert_all_cyc_critical_pairs_into_map1(*rule2, *rule1, &cyc_ids);
        }
        
    }

    set_foreach(rws.cyc_rules, rule1) {
        set_foreach(rws.eqs, rule2) {
            insert_all_cyc_critical_pairs_into_map2(*rule1, *rule2, &cyc_ids);
        }
        set_foreach(rws.cyc_rules, rule2) {
            insert_all_cyc_critical_pairs_into_map3(*rule1, *rule2, &cyc_ids);
        }
    }

    b32 is_confluent = 1;
    
    set_foreach(ids, id) {
        word_t w1 = word_normal_form(id->l, rws.rules);
        word_t w2 = word_normal_form(id->r, rws.rules);
        if(!word_equivalence(w1, w2, rws.eqs)) {
            is_confluent = 0;
            printf("%.*s ? %.*s\n", id->l.length, id->l.data, id->r.length, id->r.data);
        }
    }
    set_foreach(cyc_ids, cyc_id) {
        cyc_word_t w1 = cyc_word_normal_form(cyc_id->l, rws.rules, rws.cyc_rules);
        cyc_word_t w2 = cyc_word_normal_form(cyc_id->r, rws.rules, rws.cyc_rules);
        if(!cyc_word_equivalence(w1, w2, rws.eqs, rws.cyc_eqs)) {
            is_confluent = 0;
            printf("(%.*s) ? (%.*s)\n", cyc_id->l.length, cyc_id->l.data, cyc_id->r.length, cyc_id->r.data);
            printf("> (%.*s) ? (%.*s)\n", w1.length, w1.data, w2.length, w2.data);
        }
    }
    return is_confluent;
}


int main() {
    rewriting_system_t rws;
    rewriting_system_init(&rws);
    
    generate_ngon_identities(&rws.ids, &rws.cyc_ids, 3, 3);
    generate_ngon_identities(&rws.ids, &rws.cyc_ids, 9, 3);

    int num_symbols = 2;
    while(1) {
        /* system("clear"); */
        /* rewriting_system_print(rws); */
        //getchar();
        {
            int length = 2 * MAX_WORD_LENGTH;
            identity_t* id = NULL;
            set_foreach(rws.ids, new_id) {
                if(new_id->l.length + new_id->r.length < length) {
                    id = new_id;
                    length = new_id->l.length + new_id->r.length;
                }
            }
            if(id) {
                word_t l = word_normal_form(id->l, rws.rules);
                word_t r = word_normal_form(id->r, rws.rules);
                identity_t old_id = *id;
                if(!word_equivalence(l, r, rws.eqs)) {
                    if(l.length == r.length) {
                        identity_t new_id = { .l = l, .r = r, .marker = 0 };
                        identity_t dual_id = { .l = r, .r = l, .marker = 0 };
                        set_foreach(rws.rules, rule) {
                            if(rule->marker) {
                                insert_all_critical_pairs_into_map(*rule, new_id, &rws.ids);
                                insert_all_critical_pairs_into_map(*rule, dual_id, &rws.ids);
                            }
                        }
                        new_id = identity_canonical(new_id);
                        identity_set_insert(&rws.eqs, new_id);
                
                    } else {
                        if(l.length < r.length) {
                            word_t tmp = r;
                            r = l;
                            l = tmp;
                        }
                        identity_t new_rule = { .l = l, .r = r, .marker = 0 };
                        identity_set_insert(&rws.rules, new_rule);
                        identity_set_t reduced_rules = { 0 };
                        identity_set_init(&reduced_rules, 64);
                        set_foreach(rws.rules, rule) {
                            if(identity_eq(*rule, new_rule)) {
                                identity_set_insert(&reduced_rules, new_rule);
                                continue;
                            }
                            word_t reduced_r = word_normal_form(rule->r, rws.rules);
                            word_t reduced_l = rule->l;
                            if(word_reduce(&reduced_l, new_rule)) {
                                identity_t new_id = identity_canonical((identity_t){ .l = reduced_l, .r = rule->r });
                                identity_set_insert(&rws.ids, new_id);
                            } else {
                                identity_t reduced_rule = { .l = reduced_l, .r = reduced_r, .marker = rule->marker};
                                identity_set_insert(&reduced_rules, reduced_rule);
                            }
                        }
                
                        identity_set_deinit(&rws.rules);
                        rws.rules = reduced_rules;
                    }
                }
                identity_set_remove(&rws.ids, old_id);
        
                continue;
            }
        }

        {
            int length = 2 * MAX_WORD_LENGTH;
            cyc_identity_t* cyc_id = NULL;
            cyc_set_foreach(rws.cyc_ids, new_cyc_id) {
                if(new_cyc_id->l.length + new_cyc_id->r.length < length) {
                    cyc_id = new_cyc_id;
                    length = new_cyc_id->l.length + new_cyc_id->r.length;
                }
            }
            
            if(cyc_id) {
                cyc_word_t l = cyc_word_normal_form(cyc_id->l, rws.rules, rws.cyc_rules);
                cyc_word_t r = cyc_word_normal_form(cyc_id->r, rws.rules, rws.cyc_rules);
                cyc_identity_t old_cyc_id = *cyc_id;
                if(!cyc_word_equivalence(l, r, rws.eqs, rws.cyc_eqs)) {
                    if(l.length == r.length) {
                        cyc_identity_t new_cyc_id = { .l = l, .r = r, .marker = 0 };
                        cyc_identity_t dual_cyc_id = { .l = r, .r = l, .marker = 0 };
                        set_foreach(rws.rules, rule) {
                            if(rule->marker) {
                                insert_all_cyc_critical_pairs_into_map2(new_cyc_id, *rule, &rws.cyc_ids);
                            }
                        }
                        cyc_identity_set_insert(&rws.cyc_eqs, new_cyc_id);
                
                    } else {
                        if(l.length < r.length) {
                            cyc_word_t tmp = r;
                            r = l;
                            l = tmp;
                        }
                        cyc_identity_t new_cyc_rule = { .l = l, .r = r, .marker = 0 };
                        cyc_identity_set_insert(&rws.rules, new_rule);
                        cyc_identity_set_t reduced_rules = { 0 };
                        cyc_identity_set_init(&reduced_rules, 64);
                        set_foreach(rws.rules, rule) {
                            if(identity_eq(*rule, new_rule)) {
                                identity_set_insert(&reduced_rules, new_rule);
                                continue;
                            }
                            word_t reduced_r = word_normal_form(rule->r, rws.rules);
                            word_t reduced_l = rule->l;
                            if(word_reduce(&reduced_l, new_rule)) {
                                identity_t new_id = identity_canonical((identity_t){ .l = reduced_l, .r = rule->r });
                                identity_set_insert(&rws.ids, new_id);
                            } else {
                                identity_t reduced_rule = { .l = reduced_l, .r = reduced_r, .marker = rule->marker};
                                identity_set_insert(&reduced_rules, reduced_rule);
                            }
                        }
                
                        identity_set_deinit(&rws.rules);
                        rws.rules = reduced_rules;
                    }
                }
                identity_set_remove(&rws.ids, old_id);
        
                continue;
            }
        }

        
        identity_t* rule = NULL;
        int l_length = MAX_WORD_LENGTH;
        set_foreach(rws.rules, marked_rule) {
            if(!marked_rule->marker) {
                if(marked_rule->l.length < l_length) {
                    rule = marked_rule;
                    l_length = rule->l.length;
                }
            }
        }
        if(rule) {
            insert_all_critical_pairs_into_map(*rule, *rule, &rws.ids);
            set_foreach(rws.rules, other_rule) {
                if(other_rule->marker) {
                    insert_all_critical_pairs_into_map(*rule, *other_rule, &rws.ids);
                    insert_all_critical_pairs_into_map(*other_rule, *rule, &rws.ids);
                }
            }
            set_foreach(rws.eqs, eq) {
                insert_all_critical_pairs_into_map(*rule, *eq, &rws.ids);
                insert_all_critical_pairs_into_map(*eq, *rule, &rws.ids);
                identity_t dual = { .l = eq->r, .r = eq->l };
                insert_all_critical_pairs_into_map(*rule, dual, &rws.ids);
                insert_all_critical_pairs_into_map(dual, *rule, &rws.ids);
            }
            rule->marker = 1;
            continue;
        }

        identity_t* eq = NULL;
        l_length = MAX_WORD_LENGTH;
        set_foreach(rws.eqs, marked_eq) {
            if(!marked_eq->marker) {
                if(marked_eq->l.length < l_length) {
                    eq = marked_eq;
                    l_length = eq->l.length;
                }
            }
        }
        if(eq) {
            identity_t new_eq = {
                .l = word_normal_form(eq->l, rws.rules),
                .r = word_normal_form(eq->r, rws.rules),
                .marker = 1
            };

            identity_set_remove(&rws.eqs, *eq);
            if(!word_equivalence(new_eq.l, new_eq.r, rws.eqs)) {
                identity_set_insert(&rws.eqs, new_eq);
            }
            continue;
        }
        break;
    }

    rewriting_system_print(rws);

    is_confluent(rws);
    
    word_t normalized_hexagon = word_normal_form(word_from_c_string("aaaaaa"), rws.rules);
    word_t normalized_triangle_hole = word_normal_form(word_from_c_string("bbb"), rws.rules);
    printf(word_equivalence(normalized_hexagon, normalized_triangle_hole, rws.eqs) ?
           "\n%.*s == %.*s\n" :
           "\n%.*s != %.*s\n", normalized_hexagon.length, normalized_hexagon.data,
           normalized_triangle_hole.length, normalized_triangle_hole.data);
    
    int max_length = 0;
    set_foreach(rws.rules, rule) {
        if(rule->l.length > max_length) {
            max_length = rule->l.length;
        }
    }
    set_foreach(rws.eqs, eq) {
        if(eq->l.length > max_length) {
            max_length = eq->l.length;
        }
    }
    
    return 0;
}
