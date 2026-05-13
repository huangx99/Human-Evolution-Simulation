#pragma once

#include "sim/field/field_module.h"

// FieldRef: alias into a FieldModule-owned field.
// All read/write operations delegate to the owning FieldModule.
// Used by EnvironmentModule/InformationModule facades so that
//   world.Env().env2.WriteNext(x,y) = v
// writes the same buffer as
//   ctx.Fields().WriteNext(fire_, x, y, v)
//
// Swap is a no-op — FieldModule.SwapAll() handles double-buffer rotation.
// Invalid FieldRef (default-constructed or from missing FieldKey) returns
// 0 for reads and no-ops for writes.

class FieldRef
{
public:
    FieldRef() = default;
    FieldRef(FieldModule* fm, FieldIndex idx) : fm_(fm), idx_(idx) {}

    bool Valid() const { return fm_ && idx_.index != 0; }

    f32 At(i32 x, i32 y) const { return Valid() ? fm_->Read(idx_, x, y) : 0.0f; }
    f32& WriteNext(i32 x, i32 y)
    {
        if (!Valid()) { static f32 dummy = 0.0f; return dummy; }
        return fm_->WriteNextRef(idx_, x, y);
    }
    bool InBounds(i32 x, i32 y) const { return Valid() ? fm_->InBounds(idx_, x, y) : false; }
    void CopyCurrentToNext() { if (Valid()) fm_->CopyCurrentToNext(idx_); }
    void FillBoth(f32 value) { if (Valid()) fm_->FillBoth(idx_, value); }
    void Swap() { if (Valid()) fm_->SwapField(idx_); }

private:
    FieldModule* fm_ = nullptr;
    FieldIndex idx_;
};
