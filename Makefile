run:
	./tpcpp
build:
	g++ -std=c++20 main.cpp -o tpcpp
br:
	g++ -std=c++20 main.cpp -o tpcpp && ./tpcpp
clean:
	rm tpcpp