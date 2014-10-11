#ifndef _HASH_H_
#define _HASH_H_

namespace sails {
namespace crypto {

unsigned int SDBMHash(char *str);

unsigned int RSHash(char *str);

unsigned int JSHash(char *str);

unsigned int PJWHash(char *str);

unsigned int ELFHash(char *str);

unsigned int BKDRHash(char *str);

unsigned int DJBHash(char *str);

unsigned int APHash(char *str);


} // namespace crypto
} // namespace sails


#endif /* _HASH_H_ */
