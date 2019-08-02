//
// Created by kek on 26.07.2019.
//

#pragma once

#include "RelationalSchema.h"
#include "ColumnData.h"
#include <vector>

using std::vector;

class RelationData {
public:
    //static int singletonValueId;
    static const int nullValueId;

    virtual int getNumRows()=0;
    int getNumColumns();
    virtual vector<shared_ptr<ColumnData>> getColumnData()=0;
    virtual shared_ptr<ColumnData> getColumnData(int columnIndex)=0;
    virtual vector<int> getTuple(int tupleIndex)=0;
    virtual void shuffleColumns()=0;

    shared_ptr<RelationalSchema> getSchema();
protected:
    explicit RelationData(shared_ptr<RelationalSchema>& schema);
    shared_ptr<RelationalSchema> schema;
};
