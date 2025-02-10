#PA3 cpp compiler
.PHONY: all run clean
all: 111521051_PA3.o
	@g++ -std=c++23 111521051_PA3.o -o 111521051
 
run:
	@./111521051 $(input) $(input) $(input) $(input) $(input) $(output)
clean:
	@rm *.o
	@rm 111521051
111521051_PA3.o: 111521051_PA3.cpp
	@g++ -std=c++23 -c 111521051_PA3.cpp
  
