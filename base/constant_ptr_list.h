// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: constant_ptr_list.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 09:20:17



#ifndef SAILS_BASE_CONSTANT_PTR_LIST_H_
#define SAILS_BASE_CONSTANT_PTR_LIST_H_

#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <list>
#include <memory>
#include <mutex>
#include "sails/base/uncopyable.h"

// 注意,这里保存的是T类型的指针,ConstantPtrList<Person>保存的是
namespace sails {
namespace base {

template<typename T>
class ConstantPtrList : public Uncopyable {
 public:
  ConstantPtrList();

  ~ConstantPtrList();

  // 初始化大小
  void init(uint32_t size);

  /**
   * 获取惟一ID
   *
   * @return unsigned int
   */
  uint32_t getUniqId();

  // 添加连接
  void add(T *t, uint32_t uid);

  // 获取某一个连接
  T* get(uint32_t uid);

  // 删除连接
  void del(uint32_t uid);

  // T类型必须实现了static destroy(T *t)的方法
  void empty();

  // 大小
  size_t size();

 protected:
  // 内部删除, 不加锁
  void _del(uint32_t uid);

 protected:
  // 总计连接数
  uint32_t total;

  // 空闲链表
  std::list<uint32_t> free;

  // 空闲链元素个数
  size_t free_size;

  // 链接列表
  T** vList;
  // 链接ID的魔数
  uint32_t magic_num;

  std::mutex list_mutex;
};















template<typename T>
ConstantPtrList<T>::ConstantPtrList()
    :total(0),
     free_size(0),
     vList(NULL),
     magic_num(0) {
}


template<typename T>
ConstantPtrList<T>::~ConstantPtrList() {
  for (int i = 1; i <= total; i++) {
    vList[i] = NULL;
  }

  delete[] vList;
}


template<typename T>
void ConstantPtrList<T>::init(uint32_t size) {
  total = size;

  free_size  = 0;

  //初始化链接链表
  if (vList) delete[] vList;

  // 分配total+1个空间(多分配一个空间, 第一个空间其实无效)
  vList = new T*[total+1];

  magic_num = time(NULL);

  magic_num = (((uint32_t)magic_num) << 24) & (0xFFFFFFFF << 24);

  vList[0] = NULL;
  // free从1开始分配, 这个值为uid
  for (uint32_t i = 1; i <= total; i++) {
    vList[i] = NULL;
    free.push_back(i);

    ++free_size;
  }
}


template<typename T>
uint32_t ConstantPtrList<T>::getUniqId() {
  std::unique_lock<std::mutex> locker(list_mutex);

  uint32_t uid = free.front();
  assert(uid > 0 && uid <= total);
  free.pop_front();
  --free_size;

  return magic_num | uid;
}


template<typename T>
T* ConstantPtrList<T>::get(uint32_t uid) {
  uint32_t magi = uid & (0xFFFFFFFF << 24);
  uid           = uid & (0xFFFFFFFF >> 8);

  if (magi != magic_num) return NULL;

  return vList[uid];
}


template<typename T>
void ConstantPtrList<T>::add(T* t,  uint32_t uid) {
  std::unique_lock<std::mutex> locker(list_mutex);

  uint32_t magi = uid & (0xFFFFFFFF << 24);
  uid  = uid & (0xFFFFFFFF >> 8);

  assert(magi ==  magic_num && uid > 0 && uid <= total && vList[uid] == NULL);

  vList[uid] = t;
}


template<typename T>
void ConstantPtrList<T>::del(uint32_t uid) {
  std::unique_lock<std::mutex> locker(list_mutex);

  uint32_t magi = uid & (0xFFFFFFFF << 24);
  uid           = uid & (0xFFFFFFFF >> 8);

  assert(magi == magic_num && uid > 0 && uid <= total && vList[uid] != NULL);

  _del(uid);
}


template<typename T>
void ConstantPtrList<T>::_del(uint32_t uid) {
  assert(uid > 0 && uid <= total && vList[uid] != NULL);

  vList[uid] = NULL;

  free.push_back(uid);

  ++free_size;
}


template<typename T>
size_t ConstantPtrList<T>::size() {
  std::unique_lock<std::mutex> locker(list_mutex);

  return total - free_size;
}

template<typename T>
void ConstantPtrList<T>::empty() {
  for (int i = 1; i <= total; i++) {
    if (vList[i] != NULL) {
      T::destroy(vList[i]);
      vList[i] = NULL;
    }
  }
}


}  // namespace base
}  // namespace sails


#endif  // SAILS_BASE_CONSTANT_PTR_LIST_H_
