#pragma once

namespace arkostracker
{

class TotalAndWeight
{
public:
    /**
     * Constructor.
     * @param the score. Probably an average already.
     * @param weight the weight (how many items there are).
     */
    explicit TotalAndWeight(float score = 0.0F, int weight = 0) noexcept;

    /**
     * Builds from cumulated scores.
     * @param cumulatedScore the cumulated scores (example: I had a 10, 13, 15 at school).
     * @param itemCount how many item there are (3 for the example above) (>0).
     */
    static TotalAndWeight buildFromCumulatedScores(float cumulatedScore, int itemCount) noexcept;

    /* @return the average. */
    float getAverage() const noexcept;
    /** @return the current score. */
    float getScore() const noexcept;
    /** @return how many results are within the result. */
    int getWeight() const noexcept;

private:
    float score;
    int weight;
};

TotalAndWeight operator+(const TotalAndWeight& left, const TotalAndWeight& right);

}   // namespace arkostracker
