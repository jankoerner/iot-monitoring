BDIR = build
CXX = g++
CXXFLAGS = -std=c++17 -O3


 output: main.o utils.o filter.o
	${CXX}  ${BDIR}/main.o ${BDIR}/utils.o ${BDIR}/filter.o -o ${BDIR}/cppfilter ${CXXFLAGS}

 main.o: main.cpp
	${CXX} -c main.cpp -o ${BDIR}/$@ ${CXXFLAGS}

 utils.o: utils.cpp utils.hpp
	${CXX} -c utils.cpp -o ${BDIR}/$@ ${CXXFLAGS}

filter.o: filter.cpp filter.hpp
	${CXX} -c filter.cpp -o ${BDIR}/$@ ${CXXFLAGS}

 clean:
	rm -rf ${BDIR}/*							

