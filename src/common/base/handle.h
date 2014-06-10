#ifndef _HANDLE_H_
#define _HANDLE_H_

#include <stdio.h>
#include <vector>

namespace sails {
namespace common {

template <typename T, typename U> class HandleChain;

template <typename T, typename U>
class Handle
{
public:
    virtual void do_handle(T t, U u, HandleChain<T, U> *chain) = 0;
};



template <typename T, typename U>
class HandleChain
{
public:
    HandleChain();
	
    void do_handle(T t, U u);

    void add_handle(Handle<T, U> *handle);
private:
    std::vector<Handle<T,U>*> chain;
    int index;
};



/**
 * implements template in head file
 */


template <typename T, typename U>
HandleChain<T, U>::HandleChain() {
    this->index = 0;
}

template <typename T, typename U>
void HandleChain<T, U>::do_handle(T t, U u) {
    if(this->chain.size() > index) {
	Handle<T, U> *handle = this->chain.at(index++);
	if(handle != NULL) {
	    handle->do_handle(t, u, this);
	}
    }else {
//		printf("end handle chain\n");
    }
}

template <typename T, typename U>
void HandleChain<T, U>::add_handle(Handle<T, U> *handle) {
    this->chain.push_back(handle);
}



} // namespace common
} // namespace sails

#endif /* _HANDLE_H_ */
