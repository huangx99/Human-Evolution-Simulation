#pragma once

#include "sim/scheduler/system_descriptor.h"

class SystemContext;

// Architecture boundary:
//
//   Field systems (FireSystem, SmellSystem, ClimateSystem, SmokeSystem)
//     → responsible for PROPAGATION: diffusion, decay, spread, transfer.
//     → operate on numeric fields (temperature, humidity, fire, smell, smoke, danger).
//     → must NOT interpret what entities "are" or what reactions "mean".
//
//   Semantic systems (SemanticReactionSystem)
//     → responsible for MEANING: entity state transitions, capability changes, reactions.
//     → operate on ecology entities via predicates (Capability, State, Material, Affordance).
//     → decide WHEN and WHAT reacts; field systems decide HOW values propagate.
//
//   Rule of thumb:
//     FireSystem spreads fire intensity across cells — it does NOT decide what burns.
//     SmellSystem diffuses smell values across cells — it does NOT decide what stinks.
//     SemanticReactionSystem decides "Flammable+Dry+Heat → Burning" — it does NOT diffuse fields.
//
//   If you need to add logic like "torches emit light" or "corpses produce smell",
//   that belongs in SemanticReactionSystem as a reaction rule, NOT in the field system.

class ISystem
{
public:
    virtual ~ISystem() = default;
    virtual void Update(SystemContext& ctx) = 0;
    virtual SystemDescriptor Descriptor() const = 0;
};
