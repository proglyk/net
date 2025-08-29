### Purpose

A unified network driver for Cortex-M: brings up Ethernet/LwIP, runs client and server tasks, manages connections and disconnections, and provides a clean API to the application.


### Concurrency & Events

* FreeRTOS: separate tasks for server dispatcher, client dispatcher, interface input task, and one task per connection/session.

* Semaphores/IRQ: ETH interrupt triggers ```net__irq()```, driver signals semaphore ```pvSmphrInput```, input task consumes frame and passes it to LwIP.

* Link state: callback in ```net.c``` tracks previous state, updates flags, and shuts down all servers on down.


### Limits / Configuration

* Config in ```proj_conf.h```: ```CLT_NUM_MAX = 4```, ```SRV_NUM_MAX = 4```, ```RMT_CLT_MAX = 4```.

* Board parameters, MAC/IP, and target choice controlled via ```#define```s.

* Server/client slot availability determined by empty name in the table.


### How to Use (minimum steps)

1. Call ```net__init(&net)```.
2. Register servers/clients with ```net__add_srv()``` / ```net__add_clt()``` using a filled ```net_init_t``` and your own ```net_if_fn_t``` set.
3. Run ```net__run(&net)``` — dispatchers and input task will start; sessions will spawn automatically as connections are made.


### Roadmap
* Add a couple of examples (a number are available yet but not prepared)
* Add wrappers for thread calls and thus get support for various RTOS not only FreeRTOS
* Do a deep code refactoring and add support of Linux
