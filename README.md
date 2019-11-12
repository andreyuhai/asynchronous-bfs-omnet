<p align="center"> ![simulation gif](https://github.com/andreyuhai/asynchronous-bfs-omnet/blob/master/simulation_sample.gif) </p>


### 5.2.2 Asynchronous BFS Construction 

#### (From the book [Distributed Graph Algorithms for Computer Networks](https://www.amazon.com/Distributed-Algorithms-Computer-Networks-Communications/dp/1447151720) by Kayhan Erciyes)

The second algorithm to build a BFS tree of a graph G is the distributed version
of the Bellman–Ford algorithm called Update_BFS. We have a single initiator as
before, which starts the algorithm by sending the layer(l) message that contains its distance to its neighbors as unity. Any node receiving a layer(1) message compares the layer value l contained in the message with its known distance to the root, and if the new value is smaller, the sender of the layer message is labeled as the new parent, and the distance is updated to l. Since the new distance to the root will affect all neighbors and other nodes, the layer(l + 1) message containing the new distance is sent to all neighbors except the new parent as shown in Algorithm 5.2. It can be seen that this process eventually builds a BFS tree starting from the root. The termination condition would be the traversing of the longest shortest path between any two nodes, which would be the diameter of the graph G.

#### 5.2.2.1 Example

An example is shown in Fig. 5.3 with six nodes numbered 1, . . . , 6, where the layer message carries the distance, and the time frame it is delivered as layer(distance,

![Algorithm](https://raw.githubusercontent.com/andreyuhai/asynchronous-bfs-omnet/master/algorithm.png)

time). Node 6 initiates the algorithm by sending the layer(1, 1) message to its one-hop neighbors. Each neighbor node, when it receives this message, compares the distance value in the message to its known distance and assigns its parent to the sender if the new distance is smaller. It can be seen in Fig. 5.3(a) that layer message reaches node 2 via node 5 before the direct connection between nodes 6 and 2, resulting in node 2 identifying node 5 as its parent. However, this situation is corrected in (b) when the layer message from node 6 reaches node 2 in the third time frame resulting in node 2 replacing its parent node 5 with node 6 correctly. Similarly, in time frame 4, node 3 replaces its parent node 4 with node 2 to correctly construct the BFS tree rooted at node 6.
