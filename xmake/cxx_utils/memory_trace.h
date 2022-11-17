#pragma once

#include <source_location>
#include <unordered_map>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#ifdef __cpp_lib_source_location
class memory_trace
{
    std::unordered_map<uintptr_t, std::source_location> data;

public:
    ~memory_trace()
    {
        for (auto &&[addr, sl] : data)
            printf("\n=====> memory leak at %p  %s:%s:%d\n", (void *)addr, sl.file_name(), sl.function_name(), sl.line());
    }
    void trace_alloc(void *addr, const std::source_location sl = std::source_location::current())
    {
        auto [_, res] = data.try_emplace((uintptr_t)addr, sl);
        if (!res)
        {
            printf("\n=====> remalloc at %p  %s:%s:%d\n", addr, sl.file_name(), sl.function_name(), sl.line());
            std::abort();
        }
    }
    void trace_free(void *addr, const std::source_location sl = std::source_location::current())
    {
        if (!data.erase((uintptr_t)addr))
        {
            printf("\n=====> double free at %p  %s:%s:%d\n", addr, sl.file_name(), sl.function_name(), sl.line());
            std::abort();
        }
    }
};
#endif