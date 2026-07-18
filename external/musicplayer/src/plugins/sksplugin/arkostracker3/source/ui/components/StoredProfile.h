#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** A base class for data of a Profile. */
class StoredProfile
{
public:
    /** Destructor. */
    virtual ~StoredProfile() = default;

    /** @return the ID of the Profile. */
    virtual int getId() const = 0;
    /**
     * Sets the ID.
     * @param id the ID. Must be >0 (as a convenience, Combobox in JUCE starts at 1).
     */
    virtual void setId(int id) = 0;

    /** @return the name to be displayed. */
    virtual juce::String getDisplayedName() const = 0;
    /**
     * Set the displayed name.
     * @param name the name.
     */
    virtual void setDisplayedName(const juce::String& name) = 0;


    /** @return true if the Profile is read-only. */
    virtual bool isReadOnly() const = 0;
    /**
     * Sets the "read only" flag.
     * @param readOnly true if read only.
     */
    virtual void setReadOnly(bool readOnly) = 0;
};

}   // namespace arkostracker
