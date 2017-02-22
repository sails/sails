// Copyright (C) 2017 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails and http://www.sailsxu.com/.
//
// Filename: list.h
// Description: 一个另类的list(更通用)
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2017-01-10 11:37:00

#ifndef SAILS_BASE_LIST_H
#define SAILS_BASE_LIST_H

#include <stdio.h>
#include <stdlib.h>


namespace sails {
namespace base {


typedef struct list_item {
  list_item* pre;
  list_item* next;
  list_item() {
    pre = NULL;
    next = NULL;
  }
} list_item;


typedef void (*for_earch_cb)(const list_item*);

class list {
 public:
  list() {
    // 为了简化后面的操作，在list头部增加一个元素，这样在增加删除时，就不用去考虑空的情况
    list_item* item = (list_item*)malloc(sizeof(list_item));
    item->pre = NULL;
    item->next = NULL;
    head = item;
    tail = item;
    iter = head;
  }

  ~list() {
    free(head);
    head = NULL;
    tail = NULL;
  }

  void add(list_item *item) {
    tail->next = item;
    item->pre = tail;
    tail = item;
  }
  bool del(list_item *item) {
    if (item == head) {
      // 初始化的内部item不能删除
      return false;
    }
    // 已经删除或不在list中的元素的元素
    if (item->next == NULL && item->pre == NULL) {
      return false;
    }
    item->pre->next = item->next;
    if (item->next != NULL) {
      item->next->pre = item->pre;
    } else {
      // 末尾
      tail = item->pre;
    }
    item->next = NULL;
    item->pre = NULL;
    return true;
  }

  void reset() {
    iter = head;
  }
  list_item* next() {
    list_item* result = iter->next;
    if (iter != tail) {
      iter = iter->next;
    }
    return result;
  }

  void for_earch(for_earch_cb cb) {
    list_item* item = head->next;
    while (item != NULL) {
      cb(item);
      item = item->next;
    }
  }

 private:
  list_item* head;
  list_item* tail;

  // 用于遍历
  list_item* iter;
};

// 把0作为一个type的地址，那么element的地址就是它在type内的偏移
#define offset_ele(type, element) \
  (uint64_t)(&(((type*)0)->element))

// 通过一个类型与元素指针，得到一个完整的对象
#define container(type, ptr, element) \
  (type*)( (char*)ptr-(uint64_t)(&(((type*)0)->element)) )



// 测试
void list_test() {
  
  class Test {
   public:
    int x;
    int y;
    list_item item;
    int z;
    
    Test(int x, int y, int z) {
      this->x = x;
      this->y = y;
      this->z = z;
    }
  };


  list* l = new list();
  Test t1(1,10,100), t2(2,20, 200), t3(3, 30, 300), t4(4, 40, 400), t5(5, 50, 500);
  l->add(&(t1.item));
  l->add(&(t2.item));
  l->add(&(t3.item));
  l->add(&(t4.item));
  l->add(&(t5.item));

  uint64_t itemoffset = offset_ele(Test, item);
  printf("itemoffset:%lld\n", itemoffset);

  l->for_earch( [](const list_item * iter) {
    Test* t = container(Test, iter, item);
    printf("t x:%d y:%d z:%d\n", t->x, t->y, t->z);
  } );

  l->del(&(t1.item));

  l->del(&(t5.item));

  l->for_earch( [](const list_item * iter) {
    Test* t = container(Test, iter, item);
    printf("t x:%d y:%d z:%d\n", t->x, t->y, t->z);
  } );
  
  delete l;
}

}  // namespace base
}  // namespace sails

#endif  // SAILS_BASE_LIST_H
