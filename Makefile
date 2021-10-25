.PHONY=run
mpce: main.o
	g++ -g -o $@ $^

%.o: %.cc
	g++ -std=c++17 -g -I. -c $<

clean:
	rm -f *.o

run: clean mpce
	./mpce

logisim:
	java -jar cpu/apps/logisim-evolution.jar cpu/logisim-files/cpu.circ
