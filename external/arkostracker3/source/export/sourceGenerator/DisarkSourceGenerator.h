#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

class SourceGenerator;

/**
 * Class to generate labels understood by Disark. It only holds complementary methods to the usual SourceGenerator.
 */
class DisarkSourceGenerator
{
public:
    /**
     * Constructor.
     * @param sourceGenerator the Source Generator to rely on generating the labels.
     */
    explicit DisarkSourceGenerator(SourceGenerator& sourceGenerator) noexcept;

    /**
     * Sets the prefix before each Disark label.
     * @param prefix the prefix.
     */
    void setPrefix(juce::String prefix) noexcept;

    /** Declares a label for the start of a Byte region. */
    void declareByteRegionStart();
    /** Declares a label for the end of a Byte region. */
    void declareByteRegionEnd();

    /** Declares a label for the start of a Word region. */
    void declareWordRegionStart();
    /** Declares a label for the end of a Word region. */
    void declareWordRegionEnd();

    /**
     * Declares a label and a Word with a "force reference".
     * @param expression the expression to write.
     * @param comment a possible comment.
     */
    void declareWordForceReference(const juce::String& expression, const juce::String& comment = juce::String());
    /**
     * Declares a label and a Word with a "force reference".
     * @param value the value.
     */
    void declareWordForceReference(int value);

    /** Declares a label for a Word with a "force reference". */
    void declareLabelForWordForceReference();

    /**
     * Declares a label and a Word with a "force non-reference".
     * @param expression the expression to write.
     */
    void declareWordForceNonReference(const juce::String& expression);
    /**
     * Declares a label and a Word with a "force non-reference".
     * @param value the value.
     * @param comment a possible comment.
     */
    void declareWordForceNonReference(int value, const juce::String& comment = juce::String());

    /** Declares a label for a Word with a "force non-reference". */
    void declareLabelForWordForceNonReference();

    /** Declares a label for the start of a Pointer region. */
    void declarePointerRegionStart();
    /** Declares a label for the end of a Pointer region. */
    void declarePointerRegionEnd();

    /** Declares a label for the start of a "force non-reference" area. */
    void declareForceNonReferenceAreaStart();
    /** Declares a label for the end of a "force non-reference" area. */
    void declareForceNonReferenceAreaEnd();
    /** Declares a label for a "force non-reference" for X bytes (1-9). */
    void declareForceNonReferenceAreaDuring(int durationInBytes);

    /** Declares a label for the start of a "force reference" area. */
    void declareForceReferenceAreaStart();
    /** Declares a label for the end of a "force reference" area. */
    void declareForceReferenceAreaEnd();

    /**
     * Declares an external label.
     * @param label the label ("Mylabel"), without any prefix or suffix.
     */
    void declareExternalLabel(const juce::String& label);

private:
    /**
     * Encodes a label using the prefix and the counter.
     * @param tagLabel the tag label.
     * @param id the id to put as a postfix.
     * @return the label.
     */
    void encodeAreaLabelAndEndOfLine(const juce::String& tagLabel, int id);

    /**
     * Opens a Region. The ID is increased first.
     * @param tag the tag.
     */
    void openRegion(const juce::String& tag);

    /**
     * Closes a Region. If no region was opened before, an exception is thrown.
     * @param tag the tag.
     */
    void closeRegion(const juce::String& tag);

    /**
     * Inserts a Region only for one instruction, so it is opened and closed in one label. Useful for tags such as "declare word with Force Reference".
     * @param tag the tag to write.
     */
    void insertInlineRegion(const juce::String& tag);

    SourceGenerator& sourceGenerator;                   // The Source Generator to rely on generating the labels.
    int nextId;                                         // The next structure ID for the postfix. Increases whenever a structure is opened.
    juce::String prefix;                                // The prefix to put before each label.

    std::vector<int> openedStructureId;                 // Stores the ids of the already opened structures.
};


}   // namespace arkostracker

