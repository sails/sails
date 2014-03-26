#ifndef _UNCOPYABLE_H_
#define _UNCOPYABLE_H_

namespace sails {
namespace common {

class Uncopyable {
protected:
    Uncopyable(){}
    ~Uncopyable(){}
private:
    Uncopyable(const Uncopyable&);
    const Uncopyable& operator=(const Uncopyable&);
};

} // namespace common
} // namespace sails

#endif /* _UNCOPYABLE_H_ */
