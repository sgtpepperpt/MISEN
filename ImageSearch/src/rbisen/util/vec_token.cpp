#include "vec_token.h"

void vt_init(vec_token* v, int max_size) {
    v->counter = 0;
    v->max_size = max_size;

    // allocation
    v->array = (iee_token*) malloc(sizeof(iee_token) * v->max_size);
}

void vt_grow(vec_token* v) {
    // allocate a new array and copy the elements
    iee_token* n = (iee_token*) malloc(sizeof(iee_token) * v->max_size * 2);
    for(int i = 0; i < v->max_size; i++)
        n[i] = v->array[i];

    // free the old array
    free(v->array);

    v->max_size = v->max_size * 2;
    v->array = n;
}

void vt_destroy(vec_token* v) {
    free(v->array);
}

void vt_push_back(vec_token* v, iee_token e) {
    if(v->counter == v->max_size)
        vt_grow(v);

    v->array[v->counter++] = e;
}

void vt_pop_back(vec_token* v) {
    if(v->counter > 0)
        v->counter--;
}

iee_token vt_peek_back(vec_token v) {
    return v.array[v.counter - 1];
}

unsigned vt_size(vec_token v) {
    return v.counter;
}

/*
int main (char* argv, int argc) {
    vec_token va;
    init(&va, 4);

    push_back(&va, 8);
    push_back(&va, 19);
    push_back(&va, 3);
    push_back(&va, 4);

    vec_token vb;
    init(&vb, 4);
    
    push_back(&vb, 19);
    push_back(&vb, 1);
    push_back(&vb, 4);
    push_back(&vb, 3);
    
    for(int i = 0; i < r.counter; i++)
        ocall_printf("%d ", r.array[i]);
    ocall_printf("\n");

    return 0;
}*/
