#pragma once

namespace arkostracker
{

class MetersListener
{
public:
    /** Destructor. */
    virtual ~MetersListener() = default;

    /** Called once the meter switch is performed. */
    virtual void onMeterSwitchPerformed() noexcept = 0;
};

}   // namespace arkostracker
