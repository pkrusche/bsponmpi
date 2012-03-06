
MODE=release
SEQUENTIAL=0

all: 
	scons -Q mode=${MODE} sequential=${SEQUENTIAL} configure=1

clean:
	scons -Q -c mode=${MODE} sequential=${SEQUENTIAL}
