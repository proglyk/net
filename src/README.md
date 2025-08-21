### Key Components

<img src="http://github.com/proglyk/net/raw/main/image/diagram_NET_EN.png" width="500" height="305">

#### net.c / net.h - the top-level orchestrator

Ñontains next functions:

* ```net__init()``` — initializes LwIP (```tcpip_init```) and the network interface (```net_netif__init()``` with link callback).

* ```net__run()``` — starts the dispatchers: clients, servers, and the input task of the interface.

* ```net__add_srv()``` / ```net__add_clt()``` — register servers/clients using a ```net_init_t``` structure.

* ```net__input()``` — signals the network interface about a new frame (unblocks the input task).

* ```net__irq()``` — forwards Ethernet MAC interrupts to the low-level driver.

* ```net__inst()``` — singleton accessor for ```net_t```.

#### net_netif.c / .h - LwIP network interface layer

Main features:

* Creates a ```struct netif```, sets up ```output```/```input``` (ETHARP/ethernet_input).

* Only static ip addressing supported.

* Input task waits on a semaphore and feeds frames to the stack (```net_eth__input()```).

* Link callback notifies ```net.c```; on link changes, flags are updated and servers are closed when the link goes down.

#### net_eth.c / .h — MAC/PHY layer

Main features:

* Init/start/stop, IRQ handling, transmit (```net_eth__output```) and receive (```net_eth__input```) pbufs.

* Works with HAL DMA descriptors.

#### net_srv.c / .h — server side (TCP)

Main features:

* Dispatcher iterates over registered servers; if link is up and enabled, calls ```setup()``` (creates listening socket) and launches ```thread_pool()```.

* ```thread_pool``` loops on ```accept()``` and spawns a FreeRTOS task ```net_conn__do``` for each client (up to ```RMT_CLT_MAX```).

* API includes ```net_srv__enable/disable/is_enabled``` and ```net_srv__delete_all()```.

#### net_clt.c / .h — client side (TCP/UDP)

Main features:

* Dispatcher iterates over registered clients; if link is up and enabled, executes ```try_connect()``` (TCP connect or UDP setup).

* Once connected, starts a task ```net_conn__do``` with client context.

#### net_conn.c / .h — connection/session logic

Main features:

* Core runner net_conn__do():

    1. calls ```ppvSessInit()``` (upper layer session init, returns context),

    2. executes main loop ```pslSessDo()``` (protocol processing),

    3. runs ```pvSessDel()``` for cleanup.

* Protocol behavior is defined by callbacks in ```net_if_fn_t```.

#### net_if.h — contract with the “upper layer”

Main features:

* ```net_if_fn_t```: pointers to ```ppvSessInit```, ```pslSessDo```, ```pvSessDel``` + optional notification callbacks.

* ```net_init_t```: unified registration structure for server/client: name, port, socket type (```CLT_TCP```/```CLT_UDP```), ```bEnabled``` flag, for clients — ```pcRmt``` (IP), ```ulId```, for servers — ```pvTopPld```.