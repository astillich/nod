
// ----------------------------------------------------------------------------

#include "nod/nodefactory.h"

// ----------------------------------------------------------------------------

namespace nod {

// ----------------------------------------------------------------------------

NodeFactory::~NodeFactory()
{

}

// ----------------------------------------------------------------------------

void NodeFactory::registerType(const NodeType &type)
{
    if (!mTypes.contains(type))
        mTypes.append(type);
}

// ----------------------------------------------------------------------------

} // namespaces

// ----------------------------------------------------------------------------
