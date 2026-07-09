#pragma once

namespace arkostracker 
{

/** Abstract class about objects that can indicate whether an operation can be performed. */
class OperationValidityProvider
{
public:
    /** Destructor. */
    virtual ~OperationValidityProvider() = default;

    /**
     * @return true if the operation can be performed.
     * @param commandId the Command ID (can be a JUCE one, or a one from our own Constants).
     */
    virtual bool canOperationBePerformed(int commandId) const noexcept = 0;
};

/** An implementation of OperationValidityProvider that always returns false. */
class OperationValidityProviderNoOp final : public OperationValidityProvider
{
public:
    bool canOperationBePerformed(int /*commandId*/) const noexcept override
    {
        return false;
    }
};

}   // namespace arkostracker

