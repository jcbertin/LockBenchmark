## Hardware Overview:

*Model Name:*	iMac  
*Model Identifier:*	iMac13,2  
*Processor Name:*	Intel Core i7  
*Processor Speed:*	3.4 GHz  
*Number of Processors:*	1  
*Total Number of Cores:*	4  
*L2 Cache (per Core):*	256 KB  
*L3 Cache:*	8 MB  
*Memory:*	16 GB

## OS X 10.11

### Spinlock:

    Thread #1: min latency = 19  
    Thread #1: max latency = 2035391  
    Thread #1: mean latency = 190.356683, count = 4983621  
    Thread #2: min latency = 19  
    Thread #2: max latency = 5032372  
    Thread #2: mean latency = 190.837564, count = 5016381  
    Result - 1.475962

### Semaphore:

    Thread #1: min latency = 19  
    Thread #1: max latency = 2544805  
    Thread #1: mean latency = 288.588579, count = 5009820  
    Thread #2: min latency = 19  
    Thread #2: max latency = 471392  
    Thread #2: mean latency = 268.325415, count = 4990182  
    Result - 3.833730

### Mutex:

    Thread #1: min latency = 21  
    Thread #1: max latency = 407322  
    Thread #1: mean latency = 6370.995614, count = 4997434  
    Thread #2: min latency = 21  
    Thread #2: max latency = 295866  
    Thread #2: mean latency = 6378.456008, count = 5002568  
    Result - 42.626932

### Adaptive #1:

    Thread #1: min latency = 22  
    Thread #1: max latency = 9963802  
    Thread #1: mean latency = 156.673405, count = 5695288  
    Thread #2: min latency = 22  
    Thread #2: max latency = 9585169  
    Thread #2: mean latency = 253.212717, count = 4304714  
    Lock: spins = 83, spins count = 8881969, locks count = 7459  
    Result - 1.677453

### Adaptive #2:

    Thread #1: min latency = 22  
    Thread #1: max latency = 9978297  
    Thread #1: mean latency = 158.928476, count = 5342999  
    Thread #2: min latency = 22  
    Thread #2: max latency = 8428217  
    Thread #2: mean latency = 221.338897, count = 4657003  
    Lock: spins = 110, spins count = 7526288, locks count = 5405  
    Result - 1.583836


## OS X 10.8

### Spinlock:

    Thread #1: min latency = 19  
    Thread #1: max latency = 1036178  
    Thread #1: mean latency = 222.629722, count = 5155813  
    Thread #2: min latency = 20  
    Thread #2: max latency = 1062551  
    Thread #2: mean latency = 245.348301, count = 4844189  
    Result - 1.921168

### Semaphore:

    Thread #1: min latency = 345  
    Thread #1: max latency = 1103199  
    Thread #1: mean latency = 1017.752916, count = 5183223  
    Thread #2: min latency = 345  
    Thread #2: max latency = 1157835  
    Thread #2: mean latency = 1103.459290, count = 4816779  
    Result - 11.697605

### Mutex:

    Thread #1: min latency = 25  
    Thread #1: max latency = 3794281  
    Thread #1: mean latency = 47134.946166, count = 4997623  
    Thread #2: min latency = 25  
    Thread #2: max latency = 6276454  
    Thread #2: mean latency = 47095.920660, count = 5002379  
    Result - 305.039503

### Adaptive #1:

    Thread #1: min latency = 26  
    Thread #1: max latency = 920708  
    Thread #1: mean latency = 272.900915, count = 4726114  
    Thread #2: min latency = 26  
    Thread #2: max latency = 977494  
    Thread #2: mean latency = 228.563486, count = 5273888  
    Lock: spins = 606, spins count = 10308144, locks count = 9658  
    Result - 2.114606

### Adaptive #2:

    Thread #1: min latency = 26  
    Thread #1: max latency = 1078730  
    Thread #1: mean latency = 301.755024, count = 4531351  
    Thread #2: min latency = 26  
    Thread #2: max latency = 926591  
    Thread #2: mean latency = 219.708379, count = 5468651  
    Lock: spins = 334, spins count = 10452431, locks count = 14688  
    Result - 2.168112
