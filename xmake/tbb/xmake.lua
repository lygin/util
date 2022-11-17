add_requires("apt::libtbb-dev", {alias='tbb'})

target("tbb")
    set_kind("binary")
    add_files("*.cc")
    add_packages("tbb")