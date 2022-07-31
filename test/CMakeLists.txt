add_executable(MarkSweepGCTest
	../include/gc/gc.hpp
	gc_test.cpp
)

add_executable(CopyGCTest
	../include/gc/gc.hpp
	copy_gc_test.cpp
)

add_executable(IncrementalGCTest
	../include/gc/gc.hpp
	incremental_gc_test.cpp
)