
#include "DependencyConsumer.h"

PartialFD DependencyConsumer::registerFd(Vertical const& lhs, Column const& rhs, double error, double score) const {
    PartialFD partialFd(lhs, rhs, error, score);
    fdConsumer_(partialFd);
    return partialFd;
}

PartialKey DependencyConsumer::registerUcc(Vertical const& keyVertical, double error, double score) const {
    PartialKey partialKey(keyVertical, error, score);
    uccConsumer_(partialKey);
    return partialKey;
}

std::string DependencyConsumer::fdsToString() const {
    std::string result;
    for (auto const& fd : discoveredFDs_) {
        result += fd.toString() + "\n\r";
    }
    return result;
}

std::string DependencyConsumer::uccsToString() const {
    std::string result;
    for (auto const& ucc : discoveredUCCs_) {
        result += ucc.toString() + "\n\r";
    }
    return result;
}

