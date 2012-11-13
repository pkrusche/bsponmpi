sequential=1

toolset = "msvc"
# toolset = "intel_windows"

if toolset == "gnu":
	tbbdir = 'C:\\Users\\peter\\Documents\\Code\\tbb40_20120201oss_src'
	MPIDIR = 'C:\\Program Files (x86)\\OpenMPI_v1.5.4-x64'
else:
	# tbbdir = 'C:\\Users\\peter\\Documents\\Code\\tbb40_20120201oss'
	tbbdir = 'C:\\Program Files (x86)\\Intel\\Parallel Studio 2011\\Composer\\tbb'

