#include "ProfilingContext.h"

#include <utility>
#include "ListAgreeSetSample.h"
#include "PLICache.h"
#include "VerticalMap.h"

#include "logging/easylogging++.h"

ProfilingContext::ProfilingContext(Configuration configuration, ColumnLayoutRelationData const* relationData,
                                   std::function<void(const PartialKey &)> const& uccConsumer,
                                   std::function<void(const PartialFD &)> const& fdConsumer,
                                   CachingMethod const& cachingMethod, CacheEvictionMethod const& evictionMethod,
                                   double cachingMethodValue) :
configuration_(std::move(configuration)), relationData_(relationData),
random_(configuration_.seed == 0 ? std::mt19937() : std::mt19937(configuration_.seed)),
customRandom_(configuration_.seed == 0 ? CustomRandom() : CustomRandom(configuration_.seed)) {
    uccConsumer_ = uccConsumer;
    fdConsumer_ = fdConsumer;
    pliCache_ = std::make_shared<PLICache>(
            relationData_,
            cachingMethod,
            evictionMethod,
            cachingMethodValue,
            getMinEntropy(relationData_),
            getMeanEntropy(relationData_),
            getMedianEntropy(relationData_),
            setMaximumEntropy(relationData_, cachingMethod),
            getMedianGini(relationData_),
            getMedianInvertedEntropy(relationData_)
            );
    pliCache_->setMaximumEntropy(getMaximumEntropy(relationData_));
    if (configuration_.sampleSize > 0) {
        auto schema = relationData_->getSchema();
        agreeSetSamples_ = std::make_unique<VerticalMap<std::shared_ptr<AgreeSetSample>>>(schema);
        for (auto& column : schema->getColumns()) {
            auto sample = createFocusedSample(std::make_shared<Vertical>(static_cast<Vertical>(*column)), 1);
            agreeSetSamples_->put(static_cast<Vertical>(*column), sample);
        }
    } else {
        agreeSetSamples_ = nullptr;
    }
    // TODO: partialFDScoring - for FD registration
}

double ProfilingContext::getMaximumEntropy(ColumnLayoutRelationData const* relationData) {
    auto columns = relationData->getColumnData();
    auto maxColumn = std::max_element(columns.begin(), columns.end(),
            [](auto& cd1, auto& cd2) {
        return cd1.getPositionListIndex()->getEntropy() < cd2.getPositionListIndex()->getEntropy();
    });
    return maxColumn->getPositionListIndex()->getEntropy();
}

double ProfilingContext::getMinEntropy(ColumnLayoutRelationData const* relationData) {
    auto& columns = relationData->getColumnData();
    auto minColumn = std::min_element(columns.begin(), columns.end(),
                                      [](auto& cd1, auto& cd2) {
                                          return cd1.getPositionListIndex()->getEntropy() < cd2.getPositionListIndex()->getEntropy();
                                      });
    return minColumn->getPositionListIndex()->getEntropy();
}

double ProfilingContext::getMedianEntropy(ColumnLayoutRelationData const* relationData) {
    std::vector<double> vals;

    for (auto& column : relationData->getColumnData()) {
        if (column.getPositionListIndex()->getEntropy() >= 0.001) {
            vals.push_back(column.getPositionListIndex()->getEntropy());
        }
    }

    return getMedianValue(std::move(vals), "MedianEntropy");
}

double ProfilingContext::getMedianInvertedEntropy(ColumnLayoutRelationData const* relationData) {
    std::vector<double> vals;

    for (auto& column : relationData->getColumnData()) {
        if (column.getPositionListIndex()->getInvertedEntropy() >= 0.001) {
            vals.push_back(column.getPositionListIndex()->getInvertedEntropy());
        }
    }

    return getMedianValue(std::move(vals), "MedianInvertedEntropy");
}

double ProfilingContext::getMeanEntropy(ColumnLayoutRelationData const* relationData) {
    double e = 0;

    for (auto& column : relationData->getColumnData()) {
        e += column.getPositionListIndex()->getEntropy();
    }
    return e / relationData->getColumnData().size();
}

double ProfilingContext::getMedianGini(ColumnLayoutRelationData const* relationData) {
    std::vector<double> vals;

    for (auto& column : relationData->getColumnData()) {
        if (column.getPositionListIndex()->getEntropy() >= 0.001) {    // getGini?
            vals.push_back(column.getPositionListIndex()->getGiniImpurity());
        }
    }

    return getMedianValue(std::move(vals), "MedianGini");
}

double ProfilingContext::setMaximumEntropy(ColumnLayoutRelationData const* relationData,
                                           CachingMethod const &cachingMethod) {
    switch (cachingMethod) {
        case CachingMethod::ENTROPY:
        case CachingMethod::COIN:
        case CachingMethod::NOCACHING:
            return relationData->getMaximumEntropy();
        case CachingMethod::TRUEUNIQUENESSENTROPY:
            return getMaximumEntropy(relationData);
        case CachingMethod::MEANENTROPYTHRESHOLD:
            return getMeanEntropy(relationData);
        case CachingMethod::HEURISTICQ2:
            return getMaximumEntropy(relationData);
        case CachingMethod::GINI:
            return getMedianGini(relationData);
        case CachingMethod::INVERTEDENTROPY:
            return getMedianInvertedEntropy(relationData);
        default:
            return 0;
    }
}

std::shared_ptr<AgreeSetSample>
ProfilingContext::createFocusedSample(std::shared_ptr<Vertical> focus, double boostFactor) {
    std::shared_ptr<ListAgreeSetSample> sample = ListAgreeSetSample::createFocusedFor(
            relationData_,
            focus,
            pliCache_->getOrCreateFor(*focus, this),
            configuration_.sampleSize * boostFactor,
            customRandom_
            );
    LOG(TRACE) << boost::format {"Creating sample focused on: %1%"} % focus->toString();
    agreeSetSamples_->put(*focus, sample);
    return sample;
}

// TODO: implement using std::max_element??
std::shared_ptr<AgreeSetSample> ProfilingContext::getAgreeSetSample(std::shared_ptr<Vertical> focus) {
    std::shared_ptr<AgreeSetSample> sample = nullptr;
    for (auto& [key, nextSample] : agreeSetSamples_->getSubsetEntries(*focus)) {
        if (sample == nullptr || nextSample->getSamplingRatio() > sample->getSamplingRatio()) {
            sample = nextSample;
        }
    }
    return sample;
}

double ProfilingContext::getMedianValue(std::vector<double> && values, std::string const& measureName) {
    if (values.size() <= 1) {
        LOG(WARNING) << "Got " << measureName << " == 0\n";
        return 0;
    }

    std::sort(values.begin(), values.end());
    return (values.size() % 2 == 0) ?
           ((values[values.size() / 2] + values[values.size() / 2 - 1]) / 2) :
           (values[values.size() / 2]);
}

