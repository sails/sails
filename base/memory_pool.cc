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

MemoryPoll::MemoryPoll() {
  start_chunk = NULL;
  end_chunk = NULL;
}

MemoryPoll::~MemoryPoll() {
  release_memory();
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
  if (__bytes > _S_max_bytes) {
    return reinterpret_cast<char*>(::operator new(__bytes));
  }
  size_t new_bytes = _M_round_up(__bytes);
  return _M_allocate_chunk(new_bytes);
}

void MemoryPoll::deallocate(char* pointer, size_t bytes) {
  if (pointer == NULL) {
    return;
  }
  if (bytes > _S_max_bytes) {
    ::operator delete(pointer);
    return;
  }

  std::unique_lock<std::mutex> locker(mutex);
  _Obj* free_list = _M_get_free_list(bytes);
  _Obj* obj = reinterpret_cast<_Obj*>(pointer);
  obj->_M_free_list_link = free_list;
  free_list = obj;
  _S_free_list[_M_round_up(bytes)/_S_align-1] = free_list;
}

char* MemoryPoll::_M_allocate_chunk(size_t __bytes) {
  std::unique_lock<std::mutex> locker(mutex);
  _Obj* free_list = _M_get_free_list(__bytes);
  if (free_list == NULL) {
    _Obj* obj = reinterpret_cast<_Obj*>(_M_refill(__bytes, 20));
     if (obj != NULL) {
       free_list = obj;
     }
  }
  if (free_list != NULL) {
    char *data = free_list->_M_client_data;
    _S_free_list[_M_round_up(__bytes)/_S_align-1] =
        free_list->_M_free_list_link;
    return data;
  }
  return NULL;
}

void* MemoryPoll::_M_refill(size_t __bytes, size_t n) {
  if (__bytes > 0 && n > 0) {
    int total_size = __bytes * n;
    void *p = ::operator new(total_size, std::nothrow);
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
      _Obj* obj_pre = reinterpret_cast<_Obj*>(p);
      _Obj* obj_next = obj_pre;

      obj_pre->_M_free_list_link = NULL;
      for (size_t i = 1; i < n; i++) {
        obj_next = reinterpret_cast<_Obj*>(reinterpret_cast<char*>(p)
                                           + __bytes * i);
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
在下面的实际测试中发现内存池并不能提高分配的效率，但是它在复杂的应用
中可以避免内存的碎片，内存池的优势才能体现出来
#include <vector>
#include <thread>

class Test {
 public:
  int x;
  int y[5];
  Test() {
    x = 1;
    y[0] = 2;
  }
};

sails::base::MemoryPoll pool;

void test() {
  for (int i = 0; i < 1000000; i++) {
    Test *t = (Test*)pool.allocate(sizeof(Test));
    if (t != NULL) {
      new(t) Test();
      // printf("x:%d y:%d\n", t->x, t->y);
      pool.deallocate((char*)t, sizeof(Test));
    }
  }
}

int main(int argc, char *argv[]) {
  std::vector<std::thread> test_list;
  for (int i = 0; i < 1; i++) {
    test_list.push_back(std::thread(test));
  }
  for (int i = 0; i < 1; i++) {
    test_list[i].join();
  }
  return 0;
}
*/
