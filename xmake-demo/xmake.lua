add_rules("mode.debug", "mode.release")

if is_mode("release") then
    add_cxflags("-O3 -Og -std=c++14")
end

if is_mode("debug") then
    add_defines("DEBUG")
end

--add_requires("protobuf-cpp")
add_requires("apt::libevent-dev", {alias = 'libevent'})
add_requires("zlib")
add_requires("apt::libtbb-dev", {alias='tbb'})
add_requires("apt::librocksdb-dev", {alias='rocksdb'})
add_requires("apt::libleveldb-dev", {alias='leveldb'})
add_requires("apt::libhiredis-dev", {alias='hiredis'})

target("liblz4")
    set_kind("static")
    add_files("lz4/lz4.c")

target("lz4test")
    set_kind("binary")
    add_files("lz4/lz4_test.c")
    add_deps("liblz4")

-- target("server")
--     set_kind("binary")
--     add_includedirs("build/.gens")
--     add_files("src/*.cc")  
--     add_files("src/*.proto", {rules = "protobuf.cpp"})
--     add_packages("protobuf-cpp")
--     add_packages("libevent")

target("client")
    set_kind("binary")
    add_files("client/*.cc")
    add_packages("libevent")

target("zlibtest")
    set_kind("binary")
    add_files("zlib/*.c")
    add_packages("zlib")

    
target("tbb")
    set_kind("binary")
    add_files("tbb/*.cc")
    add_packages("tbb")

target("rocksdb")
    set_kind("binary")
    add_files("rocksdb/*.cc")
    add_packages("rocksdb")
    add_packages("leveldb")
    add_includedirs("cpp")

-- target("rdma")
--     set_kind("binary")
--     add_files("rdma/*.cc")
--     add_includedirs("cpp")

target("redis-async")
    set_kind("binary")
    add_files("redis/libevent-hiredis-async.cc")
    add_packages("hiredis")
    add_packages("libevent")

target("redis-pipeline")
    set_kind("binary")
    add_files("redis/hiredis-pipeline.cc")
    add_packages("hiredis")

--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
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

