#pragma once

#include "sim/field/field_module.h"

// ScalarFieldRef: alias into a FieldModule-owned scalar field.
// Unlike FieldRef (which wraps a 2D spatial field), ScalarFieldRef
// wraps a single global value. Read/WriteNext delegate to FieldModule
// with (0, 0) coordinates.

class ScalarFieldRef
{
public:
    ScalarFieldRef() = default;
    ScalarFieldRef(FieldModule* fm, FieldIndex idx) : fm_(fm), idx_(idx) {}

    f32 Read() const { return fm_->Read(idx_, 0, 0); }
    void WriteNext(f32 value) { fm_->WriteNext(idx_, 0, 0, value); }
    f32& WriteNextRef() { return fm_->WriteNextRef(idx_, 0, 0); }
    bool Valid() const { return fm_ && idx_.index != 0; }

private:
    FieldModule* fm_ = nullptr;
    FieldIndex idx_;
};
