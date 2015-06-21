:mod:`usocket` -- socket module
===============================

.. module:: usocket
   :synopsis: socket module

This module provides access to a subset of the BSD socket interface.

.. note: Some behaviour is platform dependant, specially when it comes to error codes.

Functions
---------

.. function:: getaddrinfo(host, port)

    Translate the host/port argument into a sequence of 5-tuples that contain all the 
    necessary arguments for creating a socket connected to that service.
    The function returns a list of 5-tuples with the following structure:

    ``(family, type, proto, canonname, sockaddr)``

    - ``family`` is always an integer representing ``AF_INET``.
    - ``type`` is always an integer representing ``SOCK_STREAM``.
    - ``proto`` is always an integer representing ``IPPROTO_TCP``.
    - ``canonname`` is always ane mpty string.
    - ``sockaddr`` is the ipv4 address of the service.

.. only:: port_pyboard

    .. function:: socket(family=AF_INET, type=SOCK_STREAM, fileno=-1)

       Create a socket.

.. only:: port_wipy

    .. function:: socket(family=socket.AF_INET, type=socket.SOCK_STREAM, proto=socket.IPPROTO_TCP, fileno=None)

       Create a new socket using the given address family, socket type and protocol number.
       
       - ``family`` can only be ``AF_INET``.
       - ``type`` can be ``SOCK_STREAM`` or ``SOCK_DGRAM``.
       - ``proto`` can be ``IPPROTO_SSL``, ``IPPROTO_TCP``, ``IPPROTO_UDP``.
       - ``fileno`` is not used, it's just kept for CPython compatibility.

.. only:: port_wipy

    Constants
    ---------

    .. data:: socket.AF_INET
    
    .. data:: socket.SOCK_STREAM
    
    .. data:: socket.SOCK_DGRAM
    
    .. data:: socket.IPPROTO_SSL
    
    .. data:: socket.IPPROTO_TCP
    
    .. data:: socket.IPPROTO_UDP


class socket
============

Functions
---------

.. function:: close

.. function:: bind

.. function:: listen

.. function:: accept

.. function:: connect

.. function:: send

.. function:: recv

.. function:: sendto

.. function:: recvfrom

.. function:: setsockopt

.. function:: settimeout

.. function:: setblocking
