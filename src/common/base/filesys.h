#ifndef _FILESYS_H_
#define _FILESYS_H_


namespace sails {
namespace common {

char* get_dir_separator(char *separator);

bool make_directory(const char* path);

} // namespace sails
} // namespace common

#endif /* _FILESYS_H_ */
