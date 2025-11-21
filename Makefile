all: kg

kg: kilo.c
	$(CC) -o kg kilo.c -Wall -W -pedantic -std=c99

clean:
	rm -f kg
