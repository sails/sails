// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: memory_pool.h
// Description: memory_pool
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-12-08 09:23:26

#ifndef BASE_MEMORY_POOL_H_
#define BASE_MEMORY_POOL_H_

#include <stdint.h>
#include <mutex>  // NOLINT


namespace sails {
namespace base {

class MemoryPoll {
 public:
  MemoryPoll();
  ~MemoryPoll();
  char* allocate(size_t __bytes);
  void deallocate(char* pointer, size_t bytes);

  void release_memory();

 private:
  enum { _S_align = 8 };
  enum { _S_max_bytes = 256 };
  enum { _S_free_list_size = (size_t)_S_max_bytes / (size_t)_S_align };

  union _Obj {
    union _Obj* _M_free_list_link;
    char        _M_client_data[1];    // The client sees this.
  };

  _Obj*        _S_free_list[_S_free_list_size];

  size_t
  _M_round_up(size_t __bytes)
  { return ((__bytes + (size_t)_S_align - 1) & ~((size_t)_S_align - 1)); }

  _Obj*
  _M_get_free_list(size_t __bytes) {
    return _S_free_list[_M_round_up(__bytes)/_S_align-1];
  }

  void*
  _M_refill(size_t __bytes, size_t n);

  char*
  _M_allocate_chunk(size_t __bytes);

 private:
  std::mutex mutex;

  typedef struct chunk_data {
    struct chunk_data* next_chunk;
    void* data;
  } ChunkData;
  ChunkData* start_chunk;
  ChunkData* end_chunk;
};

}  // namespace base
}  // namespace sails




#endif  // BASE_MEMORY_POOL_H_
