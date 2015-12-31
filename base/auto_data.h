// Copyright (C) 2015 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: auto_data.h
// Description: 模拟动态数据类型
//               type t = 10; int i = v;
//               type t = "tes", std::string s  = t;
//               type t = true, bool b = t;
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2015-12-31 12:36:44


#include <string>
#include <map>
#include <vector>

#ifndef AUTO_DATA_H_
#define AUTO_DATA_H_

namespace sails {

class auto_data {
 public:
  ///////////////////////////
  // type enumeration //
  ///////////////////////////
  enum class data_type : uint8_t {
    null,
      string,
      boolean,
      number_integer,
      number_float
        };

  ///////////////////////////
  // value storage //
  ///////////////////////////
  union data_value {
    std::string* str;
    bool boolean;
    int64_t int_val;
    double f_val;

    data_value(const std::string& v) {
      str = new std::string(v);
    }
    data_value(bool boolean) : boolean(boolean) {}
    data_value(int64_t v) : int_val(v) {}
    data_value(double v) : f_val(v) {}
    data_value(data_type t) {
      switch (t) {
        case data_type::string: {
          str = new std::string("");
          break;
        }
        case data_type::boolean: {
          boolean = false;
          break;
        }
        case data_type::number_integer: {
          int_val = 0;
          break;
        }
        case data_type::number_float: {
          f_val = 0.0;
        }
        default:
          break;
      }
    }
  };

 public:
  // 从其它类型构造basic_data
  using string_t = std::string;
  auto_data() {
    type = data_type::null;
  }
  auto_data(const string_t& v)  // NOLINT
      : value(v), type(data_type::string) {}
  auto_data(const char* v)  // NOLINT
      : value(std::string(v)), type(data_type::string) {}

  auto_data(bool v)  // NOLINT
      : value(v), type(data_type::boolean) {}
  auto_data(int64_t v)  // NOLINT
      : value(v), type(data_type::number_integer) {}
  auto_data(int v)  // NOLINT
      : value((int64_t)v), type(data_type::number_integer) {}
  auto_data(double v)  // NOLINT
      : value(v), type(data_type::number_float) {}

  // map value
  auto_data& operator [](const std::string& key) {
    if (map_data == NULL) {
      map_data = new std::map<std::string, auto_data>();
    }
    auto iter = map_data->find(key);
    if (iter == map_data->end()) {  // find
      (*map_data)[key] = auto_data();
    }
    return map_data->at(key);
  }
  auto_data& operator [](const char* key) {
    if (map_data == NULL) {
      map_data = new std::map<std::string, auto_data>();
    }
    auto iter = map_data->find(key);
    if (iter == map_data->end()) {  // find
      (*map_data)[key] = auto_data();
    }
    return map_data->at(key);
  }
  // assignment operator
  /*
  void operator =(auto_data& data) {
    value = data_value(data.value);
    type = data.type;
    }
  */
  void operator =(auto_data data) {
    type = data.type;
    if (data.type == data_type::string) {
      value = new std::string(*data.value.str);
    }
    value = data_value(data.value);
    if (data.map_data != NULL) {
      map_data = new std::map<std::string, auto_data>(*data.map_data);
    }
  }
  

  // 重载隐式转换，注意它与T operator()的区分，后者是让它成为一个防函数
  // 通过这个模板，可以直接使用int v = basic_data_object;它会生成一个
  // operator int()的函数（这与我们模板作为参数要显示指定不同），但是对
  // 与cppcheck这样的静态检测工具来说，好像不能正确识别出来，会出现要求
  // 实现一个operator int()这样的提示
  // 为了保证cppcheck之类的不出现提示信息，下面进行偏特化
  /*
  template<typename T>
  operator T() const {
    T convertedValue;
    switch (type) {
      case data_type::boolean: {
        convertedValue = value.boolean;
        printf("is a boolean\n");
        break;
      }
      case data_type::number_integer: {
        convertedValue = value.int_val;
        break;
      }
      case data_type::number_float: {
        convertedValue = value.f_val;
        break;
      }
      default:
        perror("type errr cover from basic_data");
        break;
    }
    return convertedValue;
  }
  */
  operator std::string() const {
    std::string str = "";
    switch (type) {
      case data_type::string: {
        str = std::string((*(value.str)));
        break;
      }
      default:
        break;
    }
    return str;
  }
  operator int() const {
    int64_t v = 0;
    switch (type) {
      case data_type::number_integer: {
        v = value.int_val;
        break;
      }
      default:
        break;
    }
    return v;
  }
  operator int64_t() const {
    int64_t v = 0;
    switch (type) {
      case data_type::number_integer: {
        v = value.int_val;
        break;
      }
      default:
        break;
    }
    return v;
  }
  operator double() const {
    double v = 0.0;
    switch (type) {
      case data_type::number_float: {
        v = value.f_val;
        break;
      }
      default:
        break;
    }
    return v;
  }
  operator bool() const {
    bool v = false;
    switch (type) {
      case data_type::boolean: {
        v = value.boolean;
        break;
      }
      default:
        break;
    }
    return v;
  }


 public:
  /// return the type as string
  std::string type_name() const {
    switch (type) {
      case (data_type::null): {
        return "null";
      }
      case (data_type::string): {
        return "string";
      }
      case (data_type::boolean): {
        return "boolean";
      }
      default: {
        return "number";
      }
    }
  }

 private:
  data_value value = data_type::null;
  data_type type;
  std::map<std::string, auto_data>* map_data = NULL;
};



}  // namespace sails

#endif  // AUTO_DATA_H_
