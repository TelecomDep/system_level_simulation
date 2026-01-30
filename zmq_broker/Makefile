CXX = g++
LIBS = -lgnuradio-blocks -lgnuradio-runtime -lgnuradio-pmt -lfmt -lzmq 

%: %.cpp
	$(CXX) $< -o $@ $(LIBS)
