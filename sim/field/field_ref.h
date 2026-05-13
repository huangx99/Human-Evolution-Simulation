#pragma once

#include "sim/field/field_module.h"

// FieldRef: alias into a FieldModule-owned field.
// All read/write operations delegate to the owning FieldModule.
// Used by deprecated EnvironmentModule/InformationModule facades so that
//   world.Env().fire.WriteNext(x,y) = v
// writes the same buffer as
//   ctx.Fields().WriteNext(fire_, x, y, v)
//
// Swap is a no-op — FieldModule.SwapAll() handles double-buffer rotation.

class FieldRef
{
public:
    FieldRef() = default;
    FieldRef(FieldModule* fm, FieldIndex idx) : fm_(fm), idx_(idx) {}

    f32 At(i32 x, i32 y) const { return fm_->Read(idx_, x, y); }
    f32& WriteNext(i32 x, i32 y) { return fm_->WriteNextRef(idx_, x, y); }
    bool InBounds(i32 x, i32 y) const { return fm_->InBounds(idx_, x, y); }
    void CopyCurrentToNext() { fm_->CopyCurrentToNext(idx_); }
    void FillBoth(f32 value) { fm_->FillBoth(idx_, value); }
    void Swap() { fm_->SwapField(idx_); }

private:
    FieldModule* fm_ = nullptr;
    FieldIndex idx_;
};
