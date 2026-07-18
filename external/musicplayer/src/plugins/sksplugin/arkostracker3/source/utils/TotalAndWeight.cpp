#include "TotalAndWeight.h"

namespace arkostracker
{

TotalAndWeight::TotalAndWeight(float pScore, int pWeight) noexcept :
    score(pScore),
    weight(pWeight)
{
}

TotalAndWeight TotalAndWeight::buildFromCumulatedScores(float cumulatedScore, int itemCount) noexcept
{
    return TotalAndWeight(cumulatedScore / static_cast<float>(itemCount), itemCount);
}

float TotalAndWeight::getAverage() const noexcept
{
    //return (weight == 0) ? 0 : total / weight;
    return score;
}

float TotalAndWeight::getScore() const noexcept
{
    return score;
}

int TotalAndWeight::getWeight() const noexcept
{
    return weight;
}

TotalAndWeight operator+(const TotalAndWeight& left, const TotalAndWeight& right)
{
    // Coming from here, https://math.stackexchange.com/questions/2091521/how-do-i-calculate-a-weighted-average-from-two-averages
    // Because I'm not clever enough to do it myself!

    const auto leftWeight = static_cast<float>(left.getWeight());
    const auto rightWeight = static_cast<float>(right.getWeight());

    const auto newScore =
        (leftWeight / (leftWeight + rightWeight)) * left.getScore()
        + (rightWeight / (leftWeight + rightWeight)) * right.getScore();

    const auto newWeight = left.getWeight() + right.getWeight();

    return TotalAndWeight(newScore, newWeight);
}

}   // namespace arkostracker
