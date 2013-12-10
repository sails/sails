#ifndef _FILTER_H_
#define _FILTER_H_

#include <vector>

namespace sails {


template <typename T, typename U> class FilterChain;

template <typename T, typename U>
class Filter
{
public:
	virtual void do_filter(T t, U u, FilterChain<T, U> *chain) = 0;
};



template <typename T, typename U>
class FilterChain
{
public:
	FilterChain();
	
	void do_filter(T t, U u);

	void add_filter(Filter<T, U> *filter);
private:
	std::vector<Filter<T,U>*> chain;
	int index;
};



/**
 * implements template in head file
 */


template <typename T, typename U>
FilterChain<T, U>::FilterChain() {
	this->index = 0;
}

template <typename T, typename U>
void FilterChain<T, U>::do_filter(T t, U u) {
	if(this->chain.size() > index) {
		Filter<T, U> *filter = this->chain.at(index++);
		if(filter != NULL) {
			filter->do_filter(t, u, this);
		}
	}else {
		printf("end filter chain\n");
	}
}

template <typename T, typename U>
void FilterChain<T, U>::add_filter(Filter<T, U> *filter) {
	this->chain.push_back(filter);
}




}

#endif /* _FILTER_H_ */
