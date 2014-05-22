#ifndef _MODULE_LOAD_H_
#define _MODULE_LOAD_H_

#include <string>

namespace sails {

class ModuleLoad {
public:
    static void load(std::string modulepath);
    static void unload();
};

} //namespace sails

#endif /* _MODULE_LOAD_H_ */
