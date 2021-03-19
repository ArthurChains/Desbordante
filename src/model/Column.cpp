#include "Column.h"
#include "Vertical.h"


using namespace std;

bool Column::operator==(const Column &rhs) const {
    if (this == &rhs) return true;
    return index == rhs.index && schema == rhs.schema;
}

Column::operator Vertical() const {
    return Vertical(*this);
}
