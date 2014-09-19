#ifndef _STRING_H_
#define _STRING_H_

#include <cstddef>
#include <string.h>
#include <assert.h>
#include <vector>
#include <string>

namespace sails {
namespace common {

// cat n byte of src to dst, len stand for the maxlen of dst
size_t strlncat(char *dst, size_t len, const char *src, size_t n);

size_t strlcat(char *dst, const char *src, size_t len);

size_t strlncpy(char *dst, size_t len, const char *src, size_t n);

size_t strlcpy(char *dst, const char *src, size_t len);


int first_index_of(const char* src, char c);
int first_index_of_substr(const char* src, const char* substr);
int last_index_of(const char* src, char c);

char* url_encode(const char *source_str, char *encode_str);

std::vector<std::string> split(std::string& str,const char* c);

} // namespace common  
} // namespace sails

#endif /* _STRING_H_ */
