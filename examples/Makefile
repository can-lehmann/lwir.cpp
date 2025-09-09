all: test_arith

run: test_arith
	./$<

test_arith: test_arith.cpp arith.hpp
	clang++ -o $@ $<

arith.hpp: arith.py arith.tmpl.hpp
	python3 $<
