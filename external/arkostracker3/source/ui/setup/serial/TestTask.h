#pragma once

#include "../../../controllers/serial/model/SerialData.h"
#include "../../../utils/task/Task.h"

namespace arkostracker
{

/** Task to test the serial connectivity. */
class TestTask : public Task<std::unique_ptr<bool>>
{
public:
    /**
     * Constructor.
     * @param serialData The data to use to test the port access.
     */
    explicit TestTask(SerialData serialData) noexcept;

    // Task method implementations.
    // ===================================================
    std::pair<bool, std::unique_ptr<bool>> performTask() noexcept override;

private:
    SerialData serialData;
};

}   // namespace arkostracker
