#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <string>
#include "../index/BPlusTree.h"

namespace mislib
{
    class UserInterface {
    private:
        BPlusTree tree;
        size_t lastId;

    public:
        UserInterface();
        ~UserInterface();

        // Starts the application by triggering autoLoad and executing the UI loop
        void run();
    };
} // namespace mislib

#endif // USER_INTERFACE_H