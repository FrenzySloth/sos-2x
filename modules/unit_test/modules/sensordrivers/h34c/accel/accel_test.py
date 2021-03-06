import pysos
import signal
import sys

# these two variables should be changed depending on the test drivers PID
# and the type of message it will be sending, If you are using the generic_test.c 
# then it is likely these two values can stay the same
TEST_MODULE = 0x80
MSG_TEST_DATA= 33
ALARM_LEN = 60

# variables holding new and old sensor values
# this can be replaces with whatever you want since this is specific to
# what the test driver expects for data
oldstate = {} 
state = {}
centroid = [507, 508, 396]

# a signal handler that will go off for an alarm
# it is highly suggested that you use this since it is the easiest way to test if your
# node has entered panic mode via the script
def panic_handler(signum, frame):
    print "it is highly likely that your node has entered panic mode"
    print "please reset the node"
    sys.exit(1)

# message handler for messages of type MSG_DATA_READY
def generic_test(msg):
    """ Small example of test driver usage. It simulates a virtual
    dice and shows which side of the dice is up.
    """
    global oldstate
    global state

    print "message recieved"
    signal.alarm(ALARM_LEN)

    #unpack the values we are expecting, in this case it is a node id, the acclerometer id,
    # and a value from the sensor
    (node_id, sensor_num, accelid, value) = pysos.unpack("<BBBH", msg['data'])

    if node_id not in state.keys():
	state[node_id] = [0, 0, 0]
	oldstate[node_id] = [0,0,0]

    # these are some simple calculations to test the sensor value we have gotten
    # this is the part which you need to fill in in order to verify that the driver is working

    accelid -= 1

    if abs(value-centroid[accelid]) > 100:
	print >> sys.stderr, "the value is to far out of range for accel id %d, the value is %d" %(accelid+1, value)
	print >> sys.stderr, "if the mote is not moving, then this is an error"
    else:
	print "the value is acceptedable, for accelid %d, the value is %d" %(accelid+1, value)

if __name__ == "__main__":
    # here we set up a connection to sossrv using the pysos module
    # and begin listening for messages
    # we also register our function above with the server so that it is called
    # when the appropriate message type is recieved
    srv = pysos.sossrv()

    srv.register_trigger(generic_test, did=TEST_MODULE, type=MSG_TEST_DATA)

    # register the signal handler and begin an alarm that will wait for 60 seconds before going off
    # other times for the alarm might be good, use your own judgement based on your test
    signal.signal(signal.SIGALRM, panic_handler)
    signal.alarm(ALARM_LEN)

    # we do this so since the test_suite application has information regarding the amount of time
    # each test should be run.  after the amount of time specified in test.lst, test_suite will 
    # end this script and move to another test
    while(1):
        continue
