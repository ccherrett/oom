#
# Example client for OOMidi Pyro bridge (Python Remote Object)
#
import Pyro.core
import time

oom=Pyro.core.getProxyForURI('PYRONAME://:Default.oom')
print "Current position is: " + str(oom.getCPos())
oom.startPlay()
time.sleep(1) # Sleep one second
oom.stopPlay()
print "New position is: " + str(oom.getCPos())
oom.rewindStart()
print "Pos after rewind is: " + str(oom.getCPos())
print "Lpos, Rpos: " + str(oom.getLPos()) + ":" + str(oom.getRPos())


