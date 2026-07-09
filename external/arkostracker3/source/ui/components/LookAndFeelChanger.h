#pragma once

namespace arkostracker 
{

/** Class for objects that know how to change the look'n'feel. */
class LookAndFeelChanger
{
public:
    /** Destructor. */
    virtual ~LookAndFeelChanger() = default;

    /**
     * Sets and apply the Theme which ID is given.
     * @param themeId the ID of the theme (native or custom).
     */
    virtual void setLookAndFeel(int themeId) = 0;

    /** Notifies the look and feel has changed. */
    virtual void notifyLookAndFeelChange() = 0;
};

}   // namespace arkostracker

