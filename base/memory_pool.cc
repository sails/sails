// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: memory_pool.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-12-08 09:52:26

#include "sails/base/memory_pool.h"


namespace sails {
namespace base {
          
         
MemoryPoll::_Obj*  MemoryPoll::_S_free_list[_S_free_list_size] = {NULL};
MemoryPoll::ChunkData* MemoryPoll::start_chunk;
MemoryPoll::ChunkData* MemoryPoll::end_chunk;

MemoryPoll::MemoryPoll() {

}

void MemoryPoll::release_memory() {
  ChunkData* chunk = start_chunk;
  while (chunk != NULL) {
    ChunkData* next = chunk->next_chunk;
    ::operator delete(chunk->data);
    delete(chunk);
    chunk = next;
  }
  start_chunk = NULL;
  end_chunk = NULL;
}

char* MemoryPoll::allocate(size_t __bytes) {
  printf("allocate:%zu\n", __bytes);
  if (__bytes > _S_max_bytes) {
    return (char*)::operator new(__bytes);
  }
  size_t new_bytes = _M_round_up(__bytes);
  return _M_allocate_chunk(new_bytes);
}

void MemoryPoll::deallocate(char* pointer, size_t bytes) {
  if (pointer == NULL) {
    return;
  }
  printf("data:%ld\n", pointer);
  printf("deallocate:%zu\n", bytes);
  if (bytes > _S_max_bytes) {
    ::operator delete(pointer);
    return;
  }

  _Obj* free_list = _M_get_free_list(bytes);
  _Obj* obj = (_Obj*)pointer;
  printf("obj->M_free_list address:%ld\n", &(obj->_M_free_list_link));
  obj->_M_free_list_link = free_list;
  free_list = obj;
  _S_free_list[_M_round_up(bytes)/_S_align-1] = free_list;
}

char* MemoryPoll::_M_allocate_chunk(size_t __bytes) {
  _Obj* free_list = _M_get_free_list(__bytes);
  if (free_list == NULL ) {
    _Obj* obj = (_Obj*)_M_refill(__bytes, 20);
     if (obj != NULL) {
       free_list = obj;
     }
  }
  if (free_list != NULL) {
    char *data = free_list->_M_client_data;
    _S_free_list[_M_round_up(__bytes)/_S_align-1] = free_list->_M_free_list_link;
    return data;
  }
  return NULL;
}

void* MemoryPoll::_M_refill(size_t __bytes, size_t n) {
  if (__bytes > 0 && n > 0) {
    int total_size = __bytes * n;
    void *p = ::operator new(total_size, std::nothrow);
    printf("pool new block p:%ld and size:%zu\n", p, total_size);
    if (p != NULL) {

      ChunkData *chunk = new ChunkData();
      chunk->data = p;
      chunk->next_chunk = NULL;

      if (MemoryPoll::start_chunk == NULL) {
        MemoryPoll::start_chunk = chunk;
        MemoryPoll::end_chunk = chunk;
      } else {
        end_chunk->next_chunk = chunk;
        end_chunk = end_chunk->next_chunk;
      }
    
      _Obj* obj_pre = (_Obj*)p;
      _Obj* obj_next = obj_pre;
      
      obj_pre->_M_free_list_link = NULL;
      for (size_t i = 1; i < n; i++) {
        obj_next = (_Obj*)((char*)p + __bytes * i);
        obj_next->_M_free_list_link = NULL;
        obj_pre->_M_free_list_link = obj_next;
        obj_pre = obj_next;
      }
    }
    return p;
  }
  return NULL;
}


}  // namespace base          
}  // namespace sails

/*
class Test {
 public:
  int x;
  int y;
  Test() {
    x = 1;
    y = 2;
  }
};


int main(int argc, char *argv[])
{
  sails::base::MemoryPoll pool;
  for (int i = 0; i < 20; i++) {
      Test *t = (Test*)pool.allocate(sizeof(Test));
      if (t != NULL) {
        new(t) Test();
        printf("x:%d y:%d\n", t->x, t->y);
        pool.deallocate((char*)t, sizeof(Test));
      }
      
  }
  
  Test *t2 = (Test*)pool.allocate(sizeof(Test));
  if (t2 != NULL) {
    new(t2) Test();
    printf("x:%d y:%d\n", t2->x, t2->y);
    pool.deallocate((char*)t2, sizeof(Test));
  }
  sails::base::MemoryPoll::release_memory();
  return 0;
}
*/
