add_rules("mode.debug", "mode.release")

if is_mode("release") then
    add_cxxflags("-O3 -Og -std=c++14")
    add_cflags("-O2")
end
if is_mode("debug") then
    add_defines("DEBUG")
end

-- includes("c_utils")
-- includes("lz4")
-- includes("zlib")
-- includes("tbb")
--includes("rocksdb")
--includes("redis")
-- includes("libevent-client")
-- includes("libevent-server")
includes("rdma")
-- includes("nvmf")


-- Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro defination
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

-- Generate CMakelists.txt
-- xmake project -k cmakelists
