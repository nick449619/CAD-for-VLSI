#PA2 cpp compiler
.PHONY: all run clean
all: 111521051_PA2.o
	@g++ -std=c++23 111521051_PA2.o -o exe
 
run:
	@./exe $(input) $(output)
clean:
	@rm *.o
	@rm exe
111521051_PA2.o: 111521051_PA2.cpp
	@g++ -std=c++23 -c 111521051_PA2.cpp
  
