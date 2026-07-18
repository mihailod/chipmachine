#include "TestTask.h"

#include "../../../controllers/serial/SerialAccess.h"

namespace arkostracker
{

TestTask::TestTask(SerialData pSerialData) noexcept :
        serialData(std::move(pSerialData))
{
}


// Task method implementations.
// ===================================================

std::pair<bool, std::unique_ptr<bool>> TestTask::performTask() noexcept
{
    const auto success = SerialAccess::testPortSync(serialData);

    return { true, std::make_unique<bool>(success) };
}

}   // namespace arkostracker
