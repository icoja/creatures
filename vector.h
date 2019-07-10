#ifndef VECTOR_MACRO_H
#define VECTOR_MACRO_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define VECTOR_MIN_CAPACITY 1
#define DEFINE_VECTOR_TYPE(type) \
	typedef struct{\
		type *data;\
		size_t capacity;\
		size_t size;\
	} vector_##type;\
	\
	static inline void vector_##type##_construct(vector_##type* vec){\
		vec->data = malloc(sizeof(type)*VECTOR_MIN_CAPACITY);\
		vec->capacity = VECTOR_MIN_CAPACITY;\
		vec->size = 0;\
	}\
	\
	static inline type vector_##type##_get(const vector_##type *vec, size_t index){\
		return vec->data[index];\
	}\
	static inline type *vector_##type##_get_ptr(const vector_##type *vec, size_t index){\
		return &vec->data[index];\
	}\
	\
	static inline void vector_##type##_push_back(vector_##type *vec, type elem){\
		if (vec->size >= vec->capacity){\
			vec->data = realloc(vec->data, sizeof(type)*vec->capacity*2);\
			vec->capacity *= 2;\
		}\
		vec->data[vec->size] = elem;\
		vec->size ++;\
	}\
	\
	static inline void vector_##type##_free(vector_##type *vec){\
		free(vec->data);\
	}\
	static inline type vector_##type##_pop_out(vector_##type *vec){\
		vec->size--;\
		return vec->data[vec->size];\
	}\
	static inline void vector_##type##_reserve(vector_##type *vec, size_t new_elements){\
		if(vec->size + new_elements > vec->capacity){\
			vec->data = realloc(vec->data, sizeof(type) * (vec->size + new_elements));\
		}\
	}\
	static inline void vector_##type##_fpush_back(vector_##type *vec, type elem){\
		vec->data[vec->size++] = elem;\
	}\
	\
	static inline void vector_##type##_shrink(vector_##type *vec){\
		while(vec->capacity >= 2*vec->size) vec->capacity = vec->capacity / 2 ? vec->capacity / 2 : 1;\
		vec->data = realloc(vec->data, sizeof(type) * vec->capacity);\
	}\
	static inline void vector_##type##_freeze(vector_##type *vec){\
		vec->capacity = vec->size;\
		if((vec->data = realloc(vec->data, vec->size * sizeof(type))) == NULL){\
			exit(1);\
		}\
	}\
	static inline void vector_##type##_clear(vector_##type *vec){\
		vec->size = 0;\
	}\
	static inline vector_##type vector_##type##_copy(const vector_##type *vec){\
		vector_##type nv;\
		nv.size = vec->size;\
		nv.capacity = vec->capacity;\
		nv.data = malloc(sizeof(type) * nv.capacity);\
		memcpy(nv.data, vec->data, sizeof(type)*nv.capacity);\
		return nv;\
	}\
	\
	static inline void vector_##type##_reverse(vector_##type *vec){\
		for(size_t k = 0; k < vec->size / 2; k++){\
			type tmp;\
			tmp = vec->data[k];\
			vec->data[k] = vec->data[vec->size-k-1];\
			vec->data[vec->size-k-1] = tmp;\
		}\
	}\
	static inline void vector_##type##_insert(vector_##type *vec, size_t offset, const type t){\
		if(++(vec->size) > vec->capacity){\
		vec->data = realloc(vec->data, sizeof(type) * vec->capacity * 2);\
		vec->capacity *= 2;\
		}\
		for (size_t i = vec->size - 1; i > offset; i--){\
			vec->data[i] = vec->data[i - 1];\
		}\
		vec->data[offset] = t;\
	}\
	static inline type vector_##type##_pop(vector_##type *vec, size_t offset){\
		type t = vec->data[offset];\
		vec->size--;\
		for (size_t i = offset; i < vec->size - 1; i++){\
			vec->data[i] = vec->data[i + 1];\
		}\
		return t;\
	}\

#endif

//DEFINE_VECTOR_TYPE(int)
/*
int vector_demo(){
	vector_int v;
	vector_int_construct(&v);
	for(int i = 0; i < 20; i++){
		vector_int_push_back(&v, i);
	}
	vector_int_insert(&v, 3, 15);
	vector_int_insert(&v, 3, 99);
	for(int i = 0; i < 22; i++){
		printf("%d\n", v.data[i]);
	}
}*/
