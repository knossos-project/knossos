def log(message):
		print message

if(knossos):
	knossos.connect('echo(QString)', log)		