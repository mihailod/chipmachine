#pragma once

#include <memory>

#include "../../import/loader/configuration/ImportConfiguration.h"

namespace arkostracker 
{

/** Pure abstract class for Import Panels (that can provide the configuration they have created). */
class ImportPanel
{
public:
    /** Destructor. */
    virtual ~ImportPanel() = default;

    /** Returns the configuration related to his panel. */
    virtual std::unique_ptr<ImportConfiguration> getConfiguration() = 0;
};

}   // namespace arkostracker

