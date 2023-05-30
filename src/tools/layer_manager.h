#ifndef TOOLS_LAYER_MANAGER_H
#define TOOLS_LAYER_MANAGER_H

#include "map_tool.h"

namespace tools
{

// Base class for managers of specific layer types.
class LayerManager: public MapTool
{
public:
  LayerManager(ToolsManager* tools_manager, const QString& object_name);

  enum { Type = map::LayerManagerType };

  int type() const override
  {
    // Enable the use of qgraphicsitem_cast with this item.
    return Type;
  }

};

} // namespace tools

#endif
