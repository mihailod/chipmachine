#include "ViewArranger.h"

namespace arkostracker 
{

ViewArranger::ViewArranger(const bool pHorizontal, const int pTopOffset) noexcept :
        horizontal(pHorizontal),
        topOffset(pTopOffset)
{
}

void ViewArranger::onResized(const juce::Component& parentComponent, const std::vector<ArrangerView*>& views) noexcept
{
    const auto totalSize = horizontal ?
            parentComponent.getWidth() :
            parentComponent.getHeight() - topOffset;
    // If the parent doesn't have any size yet (has just been created), don't bother do anything.
    if (totalSize <= 0) {
        return;
    }

    // What algorithm to choose?
    const auto sizes = calculateSizes(totalSize, views);

    // Tries to change even if there is nothing, because the "unwanted" size may have changed, we must adapt to it too.
    setSizesToViews(parentComponent, sizes, totalSize, views);
}

std::vector<int> ViewArranger::calculateSizes(const int availableSize, const std::vector<ArrangerView*>& views) const noexcept
{
    std::vector<int> sizes;
    sizes.reserve(views.size());

    // A first pass of normalization. Takes care of the Views that have no size, uses their desired/minimum size.
    // Also calculates how much room is left, and put the Views that are worth resizing into a Set.
    // ----------------------------------------------
    size_t viewIndex = 0U;
    auto remainingSize = availableSize;
    std::set<size_t> viewIndexesToResize;
    for (const auto& view : views) {
        const auto dimension = horizontal ? view->getArrangerViewWidth() : view->getArrangerViewHeight();
        auto viewSize = view->getCurrentSize(horizontal);
        if (viewSize <= 0) {
            // The view has no size yet. Uses their desired, or a default.
            const auto& desiredSize = dimension.getDesiredSize();
            viewSize = desiredSize.isPresent() ? desiredSize.getValue() : dimension.getMinimum();
        }

        // Marks the view as "to be resized", but only if not locked.
        if (!dimension.isLocked()) {
            viewIndexesToResize.insert(viewIndex);
        }

        remainingSize -= viewSize;

        sizes.emplace_back(viewSize);
        ++viewIndex;
    }

    // Is there no space to share? Then everything is fine, stops here.
    if (remainingSize == 0) {
        return sizes;
    }

    // Space to fill but no view can be resized, abnormal.
    if (viewIndexesToResize.empty()) {
        jassertfalse;
        return sizes;
    }

    // Now shrinks or grows the Views according to the remaining size.
    // ----------------------------------------------
    if (remainingSize < 0) {
        manageShrink(views, -remainingSize, sizes, viewIndexesToResize);
    } else {
        manageGrow(views, remainingSize, sizes, viewIndexesToResize);
    }

    return sizes;
}

void ViewArranger::manageGrow(const std::vector<ArrangerView*>& views, int remainingSize, std::vector<int>& sizes,
                              const std::set<size_t>& viewIndexesToResize) const noexcept
{
    // This is useful to skip Views that have reached their desired size after one pass.
    std::set<size_t> viewIndexesToSkip;

    // There is some space to share: grows the views.
    // Makes several passes: due to rounding, can be some pixels "lost", which must be shared again.
    auto passCounter = 3;
    while ((passCounter > 0) && (remainingSize > 0)) {

        auto sizeToShare = 0;            // Set below.
        auto mustRefreshSizeToShare = true;

        for (const auto viewIndexToResize : viewIndexesToResize) {
            // Must be skipped?
            if (viewIndexesToSkip.find(viewIndexToResize) != viewIndexesToSkip.cend()) {
                continue;
            }

            // How many views to resize? What size to share? Don't calculate that if not needed.
            if (mustRefreshSizeToShare) {
                const auto viewToResizeCount = static_cast<int>(viewIndexesToResize.size() - viewIndexesToSkip.size());
                if (viewToResizeCount <= 0) {
                    passCounter = 0;            // We can all stop!
                    break;
                }

                sizeToShare = remainingSize / viewToResizeCount;
                if (sizeToShare == 0) {         // We know it is not 0, but rounding will make it 0.
                    sizeToShare = 1;
                }

                mustRefreshSizeToShare = false;
            }

            const auto currentSize = sizes.at(viewIndexToResize);
            auto newSize = currentSize;
            const auto& view = views.at(viewIndexToResize);

            // If the view is too small compared to his desired size, we can make it grow (up to the desired size).
            // Else, make it grow up.
            auto managed = false;
            const auto desiredSizeOptional = (horizontal ? view->getArrangerViewWidth() : view->getArrangerViewHeight()).getDesiredSize();
            if (desiredSizeOptional.isPresent()) {
                const auto desiredSize = desiredSizeOptional.getValue();
                // Views with the right size shouldn't be here, but they CAN in some corner case: only Views with desires are here,
                // and we have choice but to resize them.
                // Also, if a View was smaller than its desired size on first pass, then was stretched.
                if (currentSize == desiredSize) {
                    managed = true;         // Nothing to do with such Views, as long as we can.
                } else if (currentSize < desiredSize) {
                    // Makes it grow, but don't go over the desired size.
                    const auto specificSizeToShare = std::min(desiredSize - currentSize, sizeToShare);
                    newSize += specificSizeToShare;

                    managed = true;
                    remainingSize -= specificSizeToShare;

                    // Skips this View for now on.
                    viewIndexesToSkip.insert(viewIndexToResize);
                    mustRefreshSizeToShare = true;      // Forces recalculation, as one view must be skipped.
                }
            }

            if (!managed) {
                newSize += sizeToShare;
                remainingSize -= sizeToShare;
            }

            sizes.at(viewIndexToResize) = newSize;

            if (remainingSize <= 0) {
                break;
            }
        }

        --passCounter;
    }

    // Corner case: there can be some space remaining if all the views have reached their min/desired (implies they have all desired! Which is normally not the case).
    // In that case, forces a resize again on all the views.
    // Use several passes to take care of the rounding.
    auto viewIndexesRemaining = viewIndexesToResize;
    viewIndexesToSkip.clear();
    for (passCounter = 3; (passCounter > 0) && (remainingSize != 0) && !viewIndexesRemaining.empty(); --passCounter) {
        auto sizeToShare = remainingSize / static_cast<int>(viewIndexesRemaining.size());
        if (sizeToShare == 0) {         // We know it is not 0, but rounding will make it 0.
            sizeToShare = 1;
        }
        for (const auto viewIndexToResize : viewIndexesRemaining) {
            // Makes sure we cannot exceed the maximum.
            const auto currentSize = sizes.at(viewIndexToResize);
            const auto maximumSize = views.at(viewIndexToResize)->getMaximumSize(horizontal);

            const auto newSize = std::min(currentSize + sizeToShare, maximumSize);
            const auto spentSize = newSize - currentSize;

            sizes.at(viewIndexToResize) = newSize;

            remainingSize -= spentSize;
            if (remainingSize <= 0) {
                break;
            }

            // If a View has reached its max, don't include it for the next passes.
            // For now, simply marks, to be removed from the collection out of loop.
            if (spentSize == 0) {
                viewIndexesToSkip.insert(viewIndexToResize);
            }
        }

        // Removes the views that have their maximum size from the collection before the next pass.
        for (const auto viewIndex : viewIndexesToSkip) {
            viewIndexesRemaining.erase(viewIndex);
        }
    }

    jassert(remainingSize == 0);
}

void ViewArranger::manageShrink(const std::vector<ArrangerView*>& views, int totalSizeToSave, std::vector<int>& sizes,
                                            const std::set<size_t>& viewIndexesToResize) const noexcept
{
    // Not exactly the same code as the Growth, so not put in common.

    // This is useful to skip Views that have reached their desired size after one pass.
    std::set<size_t> viewIndexesToSkip;

    // There is some space to share: grows the views.
    // Makes several passes: due to rounding, there can be some pixels "lost", which must be shared again.
    auto passCounter = 3;
    while ((passCounter > 0) && (totalSizeToSave > 0)) {

        auto sizeToShare = 0;            // Set below.
        auto mustRefreshSizeToShare = true;

        for (const auto viewIndexToResize : viewIndexesToResize) {
            // Must be skipped?
            if (viewIndexesToSkip.find(viewIndexToResize) != viewIndexesToSkip.cend()) {
                continue;
            }

            // How many views to resize? What size to share? Don't calculate that if not needed.
            if (mustRefreshSizeToShare) {
                const auto viewToResizeCount = static_cast<int>(viewIndexesToResize.size() - viewIndexesToSkip.size());
                if (viewToResizeCount <= 0) {
                    passCounter = 0;            // We can all stop!
                    break;
                }

                sizeToShare = totalSizeToSave / viewToResizeCount;
                if (sizeToShare == 0) {         // We know it is not 0, but rounding will make it 0.
                    sizeToShare = 1;
                }

                mustRefreshSizeToShare = false;
            }

            const auto currentSize = sizes.at(viewIndexToResize);
            auto newSize = currentSize;
            const auto& view = views.at(viewIndexToResize);

            // If the view is too big compared to his desired size, we can make it shrink (up to the desired size).
            // Else, shrinks up to the minimum at worst.
            auto managed = false;
            auto markThisViewAsSkipped = false;
            const auto& desiredSizeOptional = (horizontal ? view->getArrangerViewWidth() : view->getArrangerViewHeight()).getDesiredSize();
            if (desiredSizeOptional.isPresent()) {
                const auto desiredSize = desiredSizeOptional.getValue();

                // Views with the right size shouldn't be here, but they CAN in some corner case: only Views with desires are here,
                // and we have choice but to resize them.
                // Also, if a View was bigger than its desired size on first pass, then was shrunk.
                if (currentSize == desiredSize) {
                    managed = true;         // Nothing to do with such Views, as long as we can.
                } else if (currentSize > desiredSize) {
                    // Makes it shrink, but don't go over the desired size.
                    const auto specificSizeToShare = std::min(currentSize - desiredSize, sizeToShare);
                    newSize -= specificSizeToShare;

                    managed = true;
                    totalSizeToSave -= specificSizeToShare;

                    // Skips this View for now on.
                    markThisViewAsSkipped = true;
                }
            }

            if (!managed) {
                // Shrinks, but watch out for minimum.
                const auto minimumSize = (horizontal ? view->getArrangerViewWidth() : view->getArrangerViewHeight()).getMinimum();

                const auto specificSizeToShare = std::min(currentSize - minimumSize, sizeToShare);
                newSize -= specificSizeToShare;

                // If the minimum has been reached, this view can be skipped.
                if (newSize == minimumSize) {
                    markThisViewAsSkipped = true;
                }

                totalSizeToSave -= specificSizeToShare;
            }

            // Skips this view from now on?
            if (markThisViewAsSkipped) {
                viewIndexesToSkip.insert(viewIndexToResize);
            }

            sizes.at(viewIndexToResize) = newSize;

            if (totalSizeToSave <= 0) {
                break;
            }
        }

        --passCounter;
    }

    // Corner case: there can be some space remaining if all the views have reached their desired, but not minimum size yet.
    // In that case, forces a resize again on all the views.
    // Use several passes to take care of the rounding.
    auto viewIndexesRemaining = viewIndexesToResize;
    viewIndexesToSkip.clear();
    for (passCounter = 3; (passCounter > 0) && (totalSizeToSave != 0) && !viewIndexesRemaining.empty(); --passCounter) {
        auto sizeToShare = totalSizeToSave / static_cast<int>(viewIndexesRemaining.size());
        if (sizeToShare == 0) {         // We know it is not 0, but rounding will make it 0.
            sizeToShare = 1;
        }
        for (const auto viewIndexToResize : viewIndexesRemaining) {
            const auto currentSize = sizes.at(viewIndexToResize);
            const auto minimumSize = views.at(viewIndexToResize)->getMinimumSize(horizontal);

            // Makes it shrink, but don't go over the minimum size.
            const auto specificSizeToShare = std::min(currentSize - minimumSize, sizeToShare);
            const auto newSize = currentSize - specificSizeToShare;

            sizes.at(viewIndexToResize) = newSize;

            totalSizeToSave -= specificSizeToShare;
            if (totalSizeToSave <= 0) {
                break;
            }

            // If a View has reached its min, don't include it for the next passes.
            // For now, simply marks, to be removed from the collection out of loop.
            if (specificSizeToShare == 0) {
                viewIndexesToSkip.insert(viewIndexToResize);
            }
        }

        // Removes the views that have their maximum size from the collection before the next pass.
        for (const auto viewIndex : viewIndexesToSkip) {
            viewIndexesRemaining.erase(viewIndex);
        }
    }

    jassert(totalSizeToSave == 0);  // This may show if the screen is too small for the Views, as they reached their minimum.
}

// --------------------------------

void ViewArranger::setSizesToViews(const juce::Component& parentComponent, const std::vector<int>& sizes, const int availableSize,
                                   const std::vector<ArrangerView*>& views) const noexcept
{
    // Either all the sizes are here, either none (if only the undesired size has changed).
    jassert((sizes.size() == views.size()) || (sizes.empty()));

    // Gets the size that doesn't change (the one we're not interested in).
    const auto totalImmutableSize = horizontal ? parentComponent.getHeight() : parentComponent.getWidth();

    // Special case if there are no sizes.
    if (sizes.empty()) {
        // Simply applies the current size and the undesired size.
        for (const auto& view : views) {
            if (horizontal) {
                view->setBounds(view->getX(), view->getY(), view->getWidth(), totalImmutableSize);
            } else {
                view->setBounds(view->getX(), view->getY(), totalImmutableSize, view->getHeight());
            }
        }
        return;
    }

    // Normal case: one new size for each view.
    auto accumulatedSize = 0;
    size_t viewIndex = 0U;
    for (const auto& view : views) {
        const auto size = sizes.at(viewIndex);
        if (horizontal) {
            view->setBounds(accumulatedSize, 0, size, totalImmutableSize);
        } else {
            view->setBounds(0, accumulatedSize + topOffset, totalImmutableSize, size);
        }

        accumulatedSize += size;
        ++viewIndex;
    }

    jassert(accumulatedSize == availableSize);        // All space should be occupied. This may show when the screen is too reduced though.
    (void)availableSize;
}

void ViewArranger::onHandleDragged(const int handleId, const int distancePixels, const std::vector<ArrangerView*>& views) const noexcept
{
    // Finds the Handle. It must exist! However, if first or last, don't do anything.
    size_t index = 0U;
    const auto count = views.size();
    ArrangerView* handle = nullptr;

    for (const auto& arrangerView : views) {
        if (arrangerView->getArrangerId() == handleId) {
            // If the found Handle is the first or last, don't do anything! We don't want to move these!
            if ((index == 0U) || (index == (count - 1U))) {
                return;
            }

            // Found the Handle!
            handle = arrangerView;
            break;
        }

        ++index;
    }
    // Only a security.
    if (handle == nullptr) {
        jassertfalse;
        return;
    }
    jassert(index > 1U);

    // Can previous and next views can be resized?
    auto& previousView = *views.at(index - 1U);
    auto& nextView = *views.at(index + 1U);

    auto positiveSizeToApply = abs(distancePixels);
    const auto previousViewSize = previousView.getCurrentSize(horizontal);
    const auto nextViewSize = nextView.getCurrentSize(horizontal);

    int valueToApplyPreviousView;   // NOLINT(*-init-variables)
    int valueToApplyNextView;       // NOLINT(*-init-variables)

    // Grow or shrink?
    // I'm sure this can be optimized and all, but it is worth it...
    if (distancePixels < 0) {
        // Shrinks the previous, grows the next.
        const auto previousViewMinimumSize = (horizontal ? previousView.getArrangerViewWidth() : previousView.getArrangerViewHeight()).getMinimum();

        // Is it allowed (limits reached?)?
        // --> TO IMPROVE? No need to test the maximum size? (like below?)
        if ((previousViewSize - positiveSizeToApply) < previousViewMinimumSize) {
            // One of the View cannot bear the motion. Takes the minimum possible difference.
            positiveSizeToApply = previousViewSize - previousViewMinimumSize;
        }

        valueToApplyPreviousView = -positiveSizeToApply;
        valueToApplyNextView = positiveSizeToApply;
    } else {
        // Grows the previous, shrinks the next.
        const auto nextViewMinimumSize = (horizontal ? nextView.getArrangerViewWidth() : nextView.getArrangerViewHeight()).getMinimum();
        const auto previousViewMaximumSize = (horizontal ? previousView.getArrangerViewWidth() : previousView.getArrangerViewHeight()).getMaximum();

        // One of the View may not bear the motion. Takes the minimum possible difference.
        if ((previousViewSize + positiveSizeToApply) > previousViewMaximumSize) {
            positiveSizeToApply = previousViewMaximumSize - previousViewSize;
        } else if ((nextViewSize - positiveSizeToApply) < nextViewMinimumSize) {
            positiveSizeToApply = nextViewSize - nextViewMinimumSize;
        }

        valueToApplyPreviousView = positiveSizeToApply;
        valueToApplyNextView = -positiveSizeToApply;
    }

    // The previous view has only its size changed.
    const auto toWidth = horizontal;
    previousView.addToSize(toWidth, valueToApplyPreviousView);
    // The handle only has its position changed.
    handle->addToPosition(toWidth, valueToApplyPreviousView);
    // The next view has its position and size changed. The X/Y changes according to the stretch/shrink of the PreviousView.
    nextView.addToPosition(toWidth, valueToApplyPreviousView);
    nextView.addToSize(toWidth, valueToApplyNextView);
}


}   // namespace arkostracker

