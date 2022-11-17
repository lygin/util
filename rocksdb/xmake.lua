add_requires("apt::librocksdb-dev", {alias='rocksdb'})
add_requires("apt::libleveldb-dev", {alias='leveldb'})
target("rocksdb")
    set_kind("binary")
    add_files("*.cc")
    add_packages("rocksdb")
    add_packages("leveldb")
    add_includedirs("$(projectdir)/cxx_utils")