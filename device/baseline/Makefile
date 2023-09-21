IDIR=../shared
BDIR = build
CXX = g++
CXXFLAGS = -std=c++17 -O3


 output: main.o utils.o
	${CXX}  ${BDIR}/main.o ${BDIR}/utils.o -o ${BDIR}/baseline ${CXXFLAGS}

 main.o: main.cpp
	${CXX} -c main.cpp -o ${BDIR}/$@ ${CXXFLAGS}

 utils.o: ${IDIR}/utils.cpp ${IDIR}/utils.hpp
	${CXX} -c ${IDIR}/utils.cpp -o ${BDIR}/$@ ${CXXFLAGS}

#Just for testing on my local machine
run_baseline:
	./build/baseline ../../host/datasets/cpu-splitted/part_1.csv 1000 1 127.0.0.1 12000

 clean:
	rm -rf ${BDIR}

