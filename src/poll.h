#ifndef _POLL_H_
#define _POLL_H_

namespace sails {

class Poll {
private:
    Poll(const Poll&);
    Poll& operator=(const Poll&);
public:
    void init();
    void set();
    void start();
    void stop();

private:
    int poll_fd;
};

} // namespace sails

#endif /* _POLL_H_ */











