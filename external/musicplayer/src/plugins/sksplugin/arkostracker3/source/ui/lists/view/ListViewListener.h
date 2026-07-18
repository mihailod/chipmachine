#pragma once



namespace arkostracker 
{

/** Listener to the events of items. This does not hold the generic events of a list (click, drag'n'drop, etc.). */
class ListViewListener
{
public:
    /** Destructor. */
    virtual ~ListViewListener() = default;

    /**
     * The user has chosen a new name.
     * @param itemIndex the index of an item.
     * @param newName the new name.
     */
    virtual void onUserConfirmedRename(int itemIndex, const juce::String& newName) noexcept = 0;

    /**
     * The user had confirmed the add Item, with the name.
     * @param itemIndex the index of the item.
     * @param name the name.
     */
    //virtual void onUserConfirmedAdd(int itemIndex, const juce::String& name) noexcept = 0;

    /** The user wants to add a new Item. It will be put at the end. */
    virtual void onUserWantsToAdd() noexcept = 0;

    /**
     * The user wants to delete Items.
     * @param indexesToDelete the indexes to delete.
     */
    virtual void onUserWantsToDelete(const std::set<int>& indexesToDelete) noexcept = 0;

    /** Called to get all the Commands. */
    virtual void getAllCommands(juce::Array<juce::CommandID>& commands) noexcept = 0;
    /** Called to get all the Commands info. */
    virtual void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) noexcept = 0;
    /** Called to get all the perform a Command. */
    virtual bool perform(const juce::ApplicationCommandTarget::InvocationInfo& info) noexcept = 0;
};

}   // namespace arkostracker

