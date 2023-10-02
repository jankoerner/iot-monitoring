IDIR=../shared
BDIR = build
CXX = g++
CXXFLAGS = -std=c++17 -g


 output: main.o utils.o filter.o
	${CXX}  ${BDIR}/main.o ${BDIR}/utils.o ${BDIR}/filter.o -o ${BDIR}/baseline ${CXXFLAGS}

 main.o: main.cpp
	${CXX} -c main.cpp -o ${BDIR}/$@ ${CXXFLAGS}

 utils.o: ${IDIR}/utils.cpp ${IDIR}/utils.hpp
	${CXX} -c ${IDIR}/utils.cpp -o ${BDIR}/$@ ${CXXFLAGS}

filter.o: ${IDIR}/filter.cpp ${IDIR}/filter.hpp
	${CXX} -c ${IDIR}/filter.cpp -o ${BDIR}/$@ ${CXXFLAGS}

#Just for testing on my local machine
run_baseline:
	./build/baseline ../../host/datasets/cpu-splitted/part_1.csv 1000 1 127.0.0.1 12000

 clean:
	rm -rf ${BDIR}

