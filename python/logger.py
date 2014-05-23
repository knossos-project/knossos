import sys
def log_to_ipython(str):
	sys.stderr.write(str)

knossos.connect('echo(QString)', log_to_ipython)	