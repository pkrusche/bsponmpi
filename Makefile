
MODE?=release
SEQUENTIAL?=0
THREADSAFE?=1

all: 
	scons -Q mode=${MODE} sequential=${SEQUENTIAL} threadsafe=${THREADSAFE} runtests=1

clean:
	scons -Q -c mode=${MODE} sequential=${SEQUENTIAL} threadsafe=${THREADSAFE} runtests=1

test:
	scons -Q mode=${MODE} sequential=${SEQUENTIAL} threadsafe=${THREADSAFE} runtests=1

all-libs:
	scons -Q mode=debug sequential=1 threadsafe=0
	scons -Q mode=debug sequential=1 threadsafe=1
	scons -Q mode=release sequential=1 threadsafe=0
	scons -Q mode=release sequential=1 threadsafe=1
	scons -Q mode=debug sequential=0 threadsafe=0
	scons -Q mode=debug sequential=0 threadsafe=1
	scons -Q mode=release sequential=0 threadsafe=0
	scons -Q mode=release sequential=0 threadsafe=1
