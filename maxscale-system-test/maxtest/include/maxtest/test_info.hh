#pragma once
#include "test_dir.hh"

struct TestDefinition
{
    const char* name;
    const char* config_template;
    const char* labels;
};

extern const TestDefinition* test_definitions;

/** The default template to use */
extern const char* default_template;
