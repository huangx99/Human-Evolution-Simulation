#include "sim/event/event_names.h"
#include "test_framework.h"
#include <string>

TEST(memory_reinforced_event_has_logger_name)
{
    ASSERT_TRUE(std::string(EventTypeName(
        EventType::CognitiveMemoryReinforced)) == "CognitiveMemoryReinforced");
    return true;
}

TEST(memory_stabilized_event_has_logger_name)
{
    ASSERT_TRUE(std::string(EventTypeName(
        EventType::CognitiveMemoryStabilized)) == "CognitiveMemoryStabilized");
    return true;
}
