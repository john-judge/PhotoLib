
Serial COM port usage with EDT Camera Link and AIA framegrabbers
EDT, Inc. 2014
www.edt.com

This README describes how to use a camera manufacturers' control application
with an EDT framegrabber over a virtual COM port.

The preferred method for Camera Link serial communications is to use the
serial DLL "middleware" method using the Camera Link standard clseredt.dll.
However, some camera control applications only implement the COM Port method.
It is for such cases that this method is provided.

COM port communication with EDT framegrabbers involves two elements:

1) The opensource com0com application, which sets up and connects virtual
   COM ports.

2) EDT's pdvcom application which transmits serial traffic between a
   virtual COM port and an EDT framegrabber's serial channel.

com0com is open source software, freely available under GPL license. For
our purposes, com0com is used to set up two two virtual COM ports connected
together. Then the camera control application uses one of the virtual COM
ports and the EDT device uses the other. By default, com0com sets up two
virtual port pairs, one of which is com4-com5. By default, EDT's pdvcom
application uses com5, so the camera control dialog would use com4. To use
this method with the defaults:

1) Install com0com by downloading and unzipping the com0com application from 
   a location such as http://tinyurl.com/com0comw7x64signed, and running its
   setup.exe (the provided link for the Win7/x64 version; other versions are
   available but may not be digitally signed.)

2) In com0com setup, verify that com4 and com5 are present and connected to
   each other.

3) Open a Pdv Utilities command window and run pdvcom from the command line.

4) Run the camera manufacturer's control application and choose the com4
   port for communications.

The above will work for the default case where there is only one (or you are
using the first of multiple) EDT framegrabber boards in the system, and 
virtual port pair com4 and com5 are available and not being used by another
device. For other than this default case, com0com can be used to set up other
COM port pairs, and pdvcom arguments can be used to specify a different
device, device connector, and COM port.

For example, say that com4 and com5 are being used, and your camera is conn-
ected to the upper (2nd, or channel 1) connector on the second of two EDT 
framegrabbers in a system. First invoke com0com setup and add another COM port
pair by selecting "Add Pair" and creating two new ports (e.g. com6 and com7).
Then use the -u, -c and -p arguments to pdvcom to specify the desired frame-
grabber, framegrabber connector, and COM port:

pdvcom -u 1 -c 1 -p com7

(to see all pdvcom options, run pdvcom -h)

Finally, run the camera configuration application and choose the com6 port,
which in this example is virtually connected to com7.

Note: some camera control applications only recognize up to com4. In such
cases you'll need to work within the limits set by the camera manufacturer.

