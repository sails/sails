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
      number_float,
      map,
      list
        };

  ///////////////////////////
  // value storage //
  ///////////////////////////
  union data_value {
    std::string* str;
    bool boolean;
    int64_t int_val;
    double f_val;

    data_value() {
    }
    data_value(std::string* v) {
      str = v;
    }
    data_value(bool boolean) : boolean(boolean) {}
    data_value(int64_t v) : int_val(v) {}
    data_value(double v) : f_val(v) {}
    data_value(data_type t) {
      switch (t) {
        case data_type::string: {
          str = NULL;
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
      : type(data_type::string) {
    std::string* str = new std::string(v);
    value.str = str;
  }
  auto_data(const char* v)  // NOLINT
      : type(data_type::string) {
    std::string* str = new std::string(v);
    value.str = str;
  }

  auto_data(bool v)  // NOLINT
      : type(data_type::boolean), value(v) {}
  auto_data(int64_t v)  // NOLINT
      : type(data_type::number_integer), value(v) {}
  auto_data(int v)  // NOLINT
      : type(data_type::number_integer), value((int64_t)v) {}
  auto_data(size_t v)  // NOLINT
      : type(data_type::number_integer), value((int64_t)v) {}
  auto_data(double v)  // NOLINT
      : type(data_type::number_float), value(v) {}
  auto_data(const auto_data& data) {
    type = data.type;
    if (data.type == data_type::string) {
      value.str = new std::string(*(data.value.str));
    } else if (data.type == data_type::map) {
      for (auto& item : data.map_data) {
        auto_data d = item.second;
        map_data[item.first] = d;
      }
    } else {
      value = data.value;
    }
  }


  ~auto_data() {
    if (type == data_type::string && value.str != NULL) {
      delete value.str;
      value.str = NULL;
    }
  }

  // map
  bool has(std::string key) const {
    if (type == data_type::map) {
      auto iter = map_data.find(key);
      if (iter != map_data.end()) {
        return true;
      }
    }
    return false;
  }
  // because of [] will insert data for key when not found, so
  // can't defined as auto_data& operator[](const std::stirng& key) const;
  // when param as "const auto_data&", call 'Get' method instead
  // besides can't return const auto_data&, bacause of it will be use
  // data["test"] = "test", this will change the result of reference
  auto_data& operator[](const std::string& key) {
    auto iter = map_data.find(key);
    if (iter == map_data.end()) {  // find
      map_data[key] = auto_data();
    }
    type = data_type::map;
    return map_data[key];
  }
  auto_data& operator[](const char* key) {
    auto iter = map_data.find(key);
    if (iter == map_data.end()) {  // find
      map_data[key] = auto_data();
    }
    type = data_type::map;
    return map_data[key];
  }

  // because of want return auto_data&, so when there is not data
  // for key, can't new one, here throw out_of_range exception
  const auto_data& Get(const std::string& key) const {
    auto iter = map_data.find(key);
    if (iter == map_data.end()) {  // find
      throw new std::out_of_range("out of range is Get method");;
    }
    return map_data.at(key);
  }
  

  // vector
  int size() const {
    return list_data.size();
  }
  auto_data operator[](int index) const {
    return list_data[index];
  }
  void push_back(const auto_data& data) {
    type = data_type::list;
    list_data.push_back(data);
  }

  bool operator ==(const auto_data& data) {
    if (this->type != data.type) {
      return false;
    }
    switch (type) {
      case data_type::string: {
        return *(this->value.str) == *(data.value.str);
      }
      case data_type::boolean: {
        return value.boolean == data.value.boolean;
      }
      case data_type::number_integer: {
        return value.int_val == data.value.int_val;
      }
      case data_type::number_float: {
        return value.f_val == data.value.f_val;
      }
      case data_type::map: {
        return false;
      }
      default:
        return false;
    }
    return false;
  }

  // assignment operator
  void operator =(const auto_data& data) {
    type = data.type;
    if (data.type == data_type::string) {
      value.str = new std::string(data.value.str->c_str());
    } else if (data.type == data_type::map) {
      map_data = data.map_data;
    } else {
      value = data.value;
    }
  }
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

  data_type Type() {
    return type;
  }

  bool empty() {
    return type == data_type::null;
  }

  bool is_true() {
    switch (type) {
      case data_type::null: {
        return false;
      }
      case data_type::boolean: {
        return value.boolean;
      }
      case data_type::number_integer: {
        return value.int_val != 0;
      }
      case data_type::number_float: {
        return value.f_val != 0;
      }
      default:
        return true;
        break;
    }
    return true;
  }

 private:
  data_type type;
  data_value value = data_type::null;
  std::map<std::string, auto_data> map_data;
  std::vector<auto_data> list_data;
};

}  // namespace sails

#endif  // AUTO_DATA_H_
