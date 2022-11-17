add_requires("apt::libhiredis-dev", {alias='hiredis'})

target("redis-async")
    set_kind("binary")
    add_files("libevent-hiredis-async.cc")
    add_packages("hiredis")
    add_packages("libevent")

target("redis-pipeline")
    set_kind("binary")
    add_files("hiredis-pipeline.cc")
    add_packages("hiredis")