#pragma once

// IRuleContext: empty interface for RulePack-specific context.
// The Engine knows nothing about its contents.
// Each RulePack defines its own concrete context (fields, config, tables, ...).
// Systems receive the context sub-section they need via constructor injection.

class IRuleContext
{
public:
    virtual ~IRuleContext() = default;
};
