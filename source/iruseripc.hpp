#pragma once
#include <3ds.h>
#include "iruser.hpp"

class irUSERIPC{
    public:
        void HandleCommands(IrUser *ir);
};