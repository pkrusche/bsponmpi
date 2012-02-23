
MODE=release
SEQUENTIAL=1

all: 
	scons -Q mode=${MODE} sequential=${SEQUENTIAL} configure=1

clean:
	scons -Q -c mode=${MODE} sequential=${SEQUENTIAL}
