#pragma once

#include "sim/field/field_module.h"

// FieldWriter: controlled write access to simulation fields.
// Systems read through const FieldModule (ctx.GetFieldModule()).
// Systems write through FieldWriter (ctx.Fields().WriteNext()).
//
// Delegates to FieldModule.

class FieldWriter
{
public:
    explicit FieldWriter(FieldModule& fields) : fields_(fields) {}

    void WriteNext(FieldIndex id, i32 x, i32 y, f32 value)
    {
        fields_.WriteNext(id, x, y, value);
    }

    f32 Read(FieldIndex id, i32 x, i32 y) const
    {
        return fields_.Read(id, x, y);
    }

    bool InBounds(FieldIndex id, i32 x, i32 y) const
    {
        return fields_.InBounds(id, x, y);
    }

    void CopyCurrentToNext(FieldIndex id)
    {
        fields_.CopyCurrentToNext(id);
    }

private:
    FieldModule& fields_;
};
