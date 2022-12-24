add_requires("apt::libevent-dev", {alias = 'libevent'})
add_requires("protobuf-cpp")
target("server")
    set_kind("binary")
    add_files("*.cc")  
    add_files("*.proto", {rules = "protobuf.cpp"})
    add_packages("protobuf-cpp")
    add_packages("libevent")