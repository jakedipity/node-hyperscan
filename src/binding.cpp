#include <nan.h>

#include "chimera_database.hpp"

NAN_MODULE_INIT(InitModule)
{
    ChimeraDatabase::Init(target);
}

NODE_MODULE(node_hyperscan, InitModule);
