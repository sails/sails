#ifndef _EVENT_LOOP_H_
#define _EVENT_LOOP_H_
#include <common/base/uncopyable.h>

namespace sails {
namespace common {

// event
struct Ev {
    int active;
    int pending;
    void *data;
    void (*cb)(Ev *, int events);
};


class EventLoop : private Uncopyable{
public:
    virtual void set(struct Ev *ev) = 0;
    virtual void run() = 0;
    virtial void stop() = 0;
    
};

} // namespace common
} // namespace sails

#endif /* _EVENT_LOOP_H_ */










