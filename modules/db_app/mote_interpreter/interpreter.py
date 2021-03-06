#!/usr/bin/python

import pysos

MSG_QUERY_REPLY=34

def result_handler(msg):
    (sid, qid, num_remaining, sensor, value) = pysos.unpack('<BHHBH', msg['data'])

    print "node id %d" %sid
    print "QueryId %d" %qid
    print "SensorId %d" %sensor
    print "Value %d" %value
    
if __name__ == "__main__":
    srv = pysos.sossrv()

#    data = pysos.pack('<HBBIHH', 1, 1, 80, 1024, 20, 0)

#    srv.post(daddr = 0, saddr = 0, did=128, sid = 128, type=33, data = data)

    data = pysos.pack('<HIHBBBB', 123, 1024, 50, 1, 0, 0, 4)

    srv.post(daddr = 1, saddr = 1, did=128, sid=128, type=33, data=data)

    srv.register_trigger(result_handler, did=128, sid=128, type=MSG_QUERY_REPLY)

    while(1):
	pass
