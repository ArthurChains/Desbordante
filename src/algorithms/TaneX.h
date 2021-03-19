
#pragma once

#include <string>

#include "CSVParser.h"
#include "PositionListIndex.h"
#include "RelationData.h"


class Tane{
private:
    CSVParser inputGenerator;
public:
    constexpr static char INPUT_FILE_CONFIG_KEY[] = "inputFile";

    const double maxFdError = 0.01;
    const double maxUccError = 0.01;
    const unsigned int maxArity = -1;

    int countOfFD = 0;
    int countOfUCC = 0;
    long aprioriMillis = 0;
    explicit Tane(std::filesystem::path const& path, char separator = ',', bool hasHeader = true)
            : inputGenerator(path, separator, hasHeader) {}
    long execute();

    static double calculateZeroAryFdError(ColumnData const* rhs, ColumnLayoutRelationData const* relationData);
    static double calculateFdError(PositionListIndex const* lhsPli, PositionListIndex const* jointPli,
                                   ColumnLayoutRelationData const* relationData);
    static double calculateUccError(PositionListIndex const* pli, ColumnLayoutRelationData const* relationData);

    //static double round(double error) { return ((int)(error * 32768) + 1)/ 32768.0; }

    void registerFD(Vertical const& lhs, Column const* rhs, double error, RelationalSchema const* schema);
    // void registerFD(Vertical const* lhs, Column const* rhs, double error, RelationalSchema const* schema);
    void registerUCC(Vertical const& key, double error, RelationalSchema const* schema);

};
