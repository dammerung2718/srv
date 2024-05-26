# srv

My home-made, (aspiring to be) high-performance web server.
It should be able to serve its own source file a bit faster after each commit.
At least thats the minimum bar for any changes.


## Trial #1

Accept requests on main thread, spawn and respond to each client via `pthread`.
Tests are ran with `wrk -t3 -c1000 http://localhost:8080/srv.c`.

```
Running 10s test @ http://localhost:8080/srv.c
  3 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   318.11us  294.67us  10.49ms   98.33%
    Req/Sec     2.41k     2.01k    5.53k    45.83%
  5940 requests in 10.10s, 22.59MB read
  Socket errors: connect 751, read 14624, write 22, timeout 0
Requests/sec:    588.27
Transfer/sec:      2.24MB
```

This will be the base requirement for successive attempts.
