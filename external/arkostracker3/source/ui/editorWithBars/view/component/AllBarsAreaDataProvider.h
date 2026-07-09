#pragma once

namespace arkostracker 
{

/** Provides the data the AllBarsArea class needs. */
class AllBarsAreaDataProvider
{
public:
    /** Destructor. */
    virtual ~AllBarsAreaDataProvider() = default;

    /** @return all the AreaTypes to display, in order. */
    virtual const std::vector<AreaType>& getAllAreaTypes() const = 0;

    /** @return the heights for the given AreaType, caption included. Must be heightCountPerArea-sized! */
    virtual std::vector<int> getBarHeights(AreaType areaType) const = 0;

    /** @return the displayed name for the given area. */
    virtual juce::String getDisplayedName(AreaType areaType) const = 0;

    /** @return the possible sections to display (as the AreaType where the sections must appear, and the label to display). */
    virtual std::vector<std::pair<AreaType, juce::String>> getSections() const = 0;
};

}   // namespace arkostracker
