#pragma once

#include "TestAreaView.h"

namespace arkostracker 
{

/** An implementation of the Test Area View that does nothing. */
class TestAreaViewNoOp final : public TestAreaView
{
public:
    void setDisplayedData(const DisplayedData& displayedData) noexcept override;
};



}   // namespace arkostracker

