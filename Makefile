OBJS:=Bus.o Exile.o olc6502.o Main.o
exile: $(OBJS) exile-disassembly.txt
	g++ -o $@ $^ -lX11 -pthread -lstdc++fs -lpng -lGL -march=skylake -flto
.cpp.o:
	g++ -c -o $@ $^ -O3 -march=skylake -flto
clean:
	rm -f $(OBJS) exile

exile-disassembly.txt:
	curl -sL -o exile-disassembly.txt http://www.level7.org.uk/miscellany/exile-disassembly.txt
