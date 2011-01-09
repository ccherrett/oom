import Pyro.core

oom=Pyro.core.getProxyForURI('PYRONAME://:Default.oom')

print "Tempo: " + str(oom.getTempo(0))

